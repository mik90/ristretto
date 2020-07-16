#include <algorithm>
#include <fstream>
#include <future>
#include <iostream>

#include <fmt/core.h>
#include <fmt/locale.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include "AlsaInterface.hpp"
#include "RistrettoClient.hpp"
#include "Utils.hpp"

namespace mik {

/**
 * RistrettoClient::RistrettoClient
 */
RistrettoClient::RistrettoClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(RistrettoProto::Decoder::NewStub(channel)), config_(), alsa_(config_) {
  SPDLOG_INFO("Constructed RistrettoClient");
}

/**
 * RistrettoClient::recordAudioChunks
 */
void RistrettoClient::recordAudioChunks(unsigned int captureDurationMilliseconds) {

  unsigned int audioId = 0;
  while (continueRecording_) {
    const auto audioData = alsa_.recordForDuration(captureDurationMilliseconds);
    if (audioData.empty()) {
      SPDLOG_ERROR("Could not capture audio!");
      continueRecording_.store(false);
      return;
    }
    RistrettoProto::AudioData audioDataProto;
    SPDLOG_DEBUG("Captured audio chunk with audioId:{}", audioId);
    audioDataProto.set_audio(audioData.data(), audioData.size());
    audioDataProto.set_audioid(audioId++);

    if (audioDataProto.ByteSizeLong() == 0) {
      SPDLOG_ERROR("audioData is empty, could not process chunk of audio");
      continue;
    }

    // Finished creating the AudioData proto
    std::lock_guard<std::mutex> lock(audioInputMutex_);
    audioInputQ_.emplace(std::move(audioDataProto));
  }
  alsa_.stopRecording();
}

/**
 * RistrettoClient::waitForTimeout
 * @brief Waits for a given duration and then disables recording
 */
void RistrettoClient::waitForTimeout() {
  SPDLOG_INFO("Recording will end after {} milliseconds", recordingTimeout_.count());
  std::this_thread::sleep_for(recordingTimeout_);
  SPDLOG_INFO("Timeout hit, ending recording...");
  continueRecording_.store(false);
}

/**
 * RistrettoClient::renderResults
 * @brief Consumes result data from the gRPC completion queue and writes it to the terminal
 * TODO: Ensure that the audioId values are in order
 */
void RistrettoClient::renderResults() {

  void* recieved_tag;
  bool ok = false;
  fmt::print("Results will be displayed below.\n");

  while (continueRecording_ && resultCompletionQ_.Next(&recieved_tag, &ok)) {
    // The tag identifies the ClientCallData* on the completion queue, so dereference it
    std::unique_ptr<ClientCallData> call(static_cast<ClientCallData*>(recieved_tag));

    if (!ok) {
      SPDLOG_ERROR("Could not process RPC with tag:{}, skipping this RPC call", recieved_tag);
      continue;
    }

    if (call->status.ok()) {
      // Render results
      fmt::print("{}", call->transcript.text());
    } else {
      SPDLOG_ERROR("gRPC error:{}", call->status.error_message());
      continue;
    }
  }
  SPDLOG_INFO("renderResults exiting...");
}

/**
 * RistrettoClient::processMicrophoneInput
 * @brief Consumes audio chunks from another thread, sends it thru gRPC for decoding, and
 *        renders it on screen in a second thread
 */
void RistrettoClient::processMicrophoneInput() {

  SPDLOG_DEBUG("Starting audio processing loop");
  continueRecording_.store(true);

  // Print the results on-screen on another thread
  auto renderingThread = std::thread(&RistrettoClient::renderResults, this);
  SPDLOG_INFO("Started result rendering thread");

  constexpr unsigned int captureChunkMs = 1000;
  // Record audio in another thread
  auto recordingThread = std::thread(&RistrettoClient::recordAudioChunks, this, captureChunkMs);
  SPDLOG_INFO("Started recording thread");

  // If configured, Stop the recording after a while
  std::thread timeoutThread;
  if (recordingTimeout_.count() > 0) {
    timeoutThread = std::thread(&RistrettoClient::waitForTimeout, this);
    SPDLOG_INFO("Started recording timeout thread");
  }

  while (continueRecording_) {

    // Consume audio from the queue
    RistrettoProto::AudioData audioData;
    {
      while (audioInputQ_.empty()) {
        // It's possible that there's no audio to send yet, so wait for some to be recorded
        SPDLOG_WARN("Audio input queue is empty, waiting before checking it again");
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
      }
      std::lock_guard<std::mutex> lock(audioInputMutex_);
      std::swap(audioData, audioInputQ_.back());
      audioInputQ_.pop();
    }

    // This will be deallocated by the completion queue processor (RistrettoClient::renderResults)
    ClientCallData* call = new ClientCallData;

    call->responseReader =
        stub_->PrepareAsyncDecodeAudio(&call->context, audioData, &resultCompletionQ_);
    call->responseReader->StartCall();
    call->responseReader->Finish(&call->transcript, &call->status, reinterpret_cast<void*>(call));

    SPDLOG_DEBUG("Sent {} bytes of audio, audioId:{}", audioData.ByteSizeLong(),
                 audioData.audioid());
  }
  SPDLOG_INFO("Recording ended.");

  if (recordingThread.joinable()) {
    recordingThread.join();
  }
  if (renderingThread.joinable()) {
    renderingThread.join();
  }
  if (timeoutThread.joinable()) {
    timeoutThread.join();
  }
  SPDLOG_DEBUG("Exiting...");
  return;
}

/**
 * RistrettoClient::decodeAudioSync
 * @brief Simple function for decoding audio in a synchronous fashion
 */
std::string RistrettoClient::decodeAudioSync(const std::vector<char>& audio, unsigned int audioId) {

  RistrettoProto::AudioData audioData;
  audioData.set_audio(audio.data(), audio.size());
  audioData.set_audioid(audioId);
  SPDLOG_INFO("Sending {} bytes of audio", audioData.ByteSizeLong());
  if (audioData.ByteSizeLong() == 0) {
    SPDLOG_ERROR("audioData is empty, abandoning RPC");
    return {};
  }

  grpc::ClientContext context;
  grpc::CompletionQueue resultCompletionQ;
  grpc::Status status;

  std::unique_ptr<grpc::ClientAsyncResponseReader<RistrettoProto::Transcript>> rpc(
      stub_->AsyncDecodeAudio(&context, audioData, &resultCompletionQ));

  RistrettoProto::Transcript transcipt;
  auto tag = reinterpret_cast<void*>(1);
  rpc->Finish(&transcipt, &status, tag);
  void* recieved_tag;
  bool ok = false;
  GPR_ASSERT(resultCompletionQ.Next(&recieved_tag, &ok));
  GPR_ASSERT(recieved_tag == tag);
  GPR_ASSERT(ok);

  if (status.ok()) {
    return transcipt.text();
  } else {
    SPDLOG_ERROR("Error with RPC: Error code:{}, details:{}", status.error_code(),
                 status.error_message());
    return {};
  }
}

/**
 * filterResult
 * @brief Strip all the newlines until we get to text and then use all of that text
 *        until we get to the newline
 * @param[in] fullResult String with undesired newlines
 * @return String without the undesired newlines
 */
std::string filterResult(const std::string& fullResult) {
  SPDLOG_INFO("Initial:{}", fullResult);
  // Strip the temporary transcript to get the final result
  auto endOfText = fullResult.crbegin();
  while (*endOfText == '\n' || *endOfText == ' ') {
    // Keep on skipping while the char is a newline or whitespace
    ++endOfText;
  }

  auto startOfText = endOfText;
  while (*startOfText != '\n') {
    // Keep on skipping until we hit another newline
    ++startOfText;
  }

  // Use these as forward iterators
  const auto textSize = endOfText.base() - startOfText.base();
  if (textSize <= 0) {
    SPDLOG_ERROR("textSize is {} which is not positive!", textSize);
    return std::string{};
  }

  const std::string filteredText(startOfText.base(), endOfText.base());
  SPDLOG_INFO("Filtered:{}", filteredText);
  return filteredText;
}

} // namespace mik
