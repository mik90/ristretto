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
RistrettoClient::RistrettoClient(const std::shared_ptr<grpc::Channel>& channel, AlsaConfig config)
    : stub_(RistrettoProto::Decoder::NewStub(channel)), config_(std::move(config)), alsa_(config_) {
  SPDLOG_INFO("Constructed RistrettoClient");

  sessionToken_ = Utils::generateSessionToken();
  SPDLOG_INFO("Created session token \"{}\"", sessionToken_);
}

/**
 * RistrettoClient::~RistrettoClient
 */
RistrettoClient::~RistrettoClient() {
  SPDLOG_INFO("Destroying RistrettoClient");
  stub_.reset();
}

/**
 * RistrettoClient::recordAudioChunks
 * @brief Starts recording and saves the audio in the input queue as protobuf objects
 */
void RistrettoClient::recordAudioChunks() {

  unsigned int audioId = 0;
  alsa_.startRecording();
  while (continueRecording_) {
    if (alsa_.audioDataAvailableMilliseconds() < chunkDuration_) {
      // It'd be smarter to use condition variables or something else but this'll work for now
      std::this_thread::sleep_for(chunkDuration_);
    }

    const auto audioData = alsa_.consumeAllAudioData();
    if (audioData.empty()) {
      SPDLOG_ERROR("No audio data was available for consumption!");
      return;
    }

    RistrettoProto::AudioData audioDataProto;
    audioDataProto.set_audio(audioData.data(), audioData.size());
    audioDataProto.set_audioid(audioId++);
    audioDataProto.set_sessiontoken(sessionToken_);

    std::lock_guard<std::mutex> lock(audioInputMutex_);
    // Add the audio data to the queue
    audioInputQ_.emplace(std::move(audioDataProto));

  } // end of while loop

  alsa_.stopRecording();
}

/**
 * RistrettoClient::renderResults
 * @brief Consumes result data from the gRPC completion queue and writes it to the terminal
 * TODO: Ensure that the audioId values are in order
 */
void RistrettoClient::renderResults() {

  SPDLOG_INFO("renderResults starting...");
  void* recieved_tag;
  bool queueIsOk = false;
  fmt::print("Results will be displayed below.\n");

  // TODO: It may be best to empty out the queue before exiting the loop
  //       due to continueRecording_ being false
  while (continueRecording_ && resultCompletionQ_.Next(&recieved_tag, &queueIsOk)) {
    // The tag identifies the ClientCallData* on the completion queue, so dereference it
    std::unique_ptr<ClientCallData> callData(static_cast<ClientCallData*>(recieved_tag));

    if (!queueIsOk) {
      SPDLOG_ERROR("Could not process RPC with tag:{}, skipping this RPC call", recieved_tag);
      continue;
    }

    if (callData->status.ok()) {
      // Render results
      SPDLOG_DEBUG("Rendering audioId {} with text \"{}\"", callData->transcript.audioid(),
                   callData->transcript.text());
      fmt::print("{}", callData->transcript.text());
    } else {
      SPDLOG_ERROR("gRPC error:{}", callData->status.error_message());
      continue;
    }
  }
  SPDLOG_INFO("renderResults exiting...");
}

/**
 * RistrettoClient::decodeMicrophoneInput
 * @brief Consumes audio chunks from another thread, sends it thru gRPC for decoding, and
 *        renders it on screen in a second thread
 */
void RistrettoClient::decodeMicrophoneInput() {

  SPDLOG_DEBUG("Starting audio processing loop");
  continueRecording_.store(true);

  // Print the results on-screen on another thread
  auto renderingThread = std::thread(&RistrettoClient::renderResults, this);
  SPDLOG_INFO("Started result rendering thread");

  // Record audio in another thread
  auto recordingThread = std::thread(&RistrettoClient::recordAudioChunks, this);
  SPDLOG_INFO("Started recording thread");

  // If configured, Stop the recording after a while
  std::thread timeoutThread;
  if (recordingTimeout_.count() > 0) {
    timeoutThread = std::thread(
        [&recordingTimeout_ = recordingTimeout_, &continueRecording_ = continueRecording_] {
          SPDLOG_INFO("Recording will end after {} milliseconds", recordingTimeout_.count());
          std::this_thread::sleep_for(recordingTimeout_);
          SPDLOG_INFO("Timeout hit, ending recording...");
          continueRecording_.store(false);
        });
    SPDLOG_INFO("Started recording timeout thread");
  }

  while (continueRecording_) {

    // Consume audio from the queue
    RistrettoProto::AudioData audioData;
    {
      while (audioInputQ_.empty()) {
        // It's possible that there's no audio to send yet, so wait for some to be recorded
        const auto timeToWait = chunkDuration_ * 3;
        SPDLOG_WARN("Audio input queue is empty, waiting for {} ms before checking it again",
                    timeToWait.count());
        std::this_thread::sleep_for(timeToWait);
      }
      std::lock_guard<std::mutex> lock(audioInputMutex_);
      std::swap(audioData, audioInputQ_.back());
      audioInputQ_.pop();
    }

    // This will be deallocated by the completion queue handler (RistrettoClient::renderResults)
    auto call = new ClientCallData;

    call->responseReader =
        stub_->PrepareAsyncDecodeAudio(&call->context, audioData, &resultCompletionQ_);
    call->responseReader->StartCall();
    call->responseReader->Finish(&call->transcript, &call->status, reinterpret_cast<void*>(call));

    SPDLOG_DEBUG("Sent {} bytes of audio, audioId:{}", audioData.ByteSizeLong(),
                 audioData.audioid());
  }
  SPDLOG_INFO("Recording ended.");

  recordingThread.join();
  renderingThread.join();
  timeoutThread.join();

  SPDLOG_DEBUG("Exiting...");
  return;
}

/**
 * RistrettoClient::decodeAudioSync
 * @brief Simple function for decoding audio in a synchronous fashion
 */
std::string RistrettoClient::decodeAudioSync(const std::vector<char>& audio, unsigned int audioId) {

  RistrettoProto::AudioData audioDataProto;
  audioDataProto.set_audio(audio.data(), audio.size());
  audioDataProto.set_audioid(audioId);
  audioDataProto.set_sessiontoken(sessionToken_);
  SPDLOG_INFO("Sending {} bytes of audio", audioDataProto.ByteSizeLong());
  if (audioDataProto.ByteSizeLong() == 0) {
    SPDLOG_ERROR("audioDataProto is empty, abandoning RPC");
    return {};
  }

  grpc::ClientContext context;
  grpc::CompletionQueue resultCompletionQ;
  grpc::Status status;

  std::unique_ptr<grpc::ClientAsyncResponseReader<RistrettoProto::Transcript>> rpc(
      stub_->AsyncDecodeAudio(&context, audioDataProto, &resultCompletionQ));

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
 *        until we get to the newline. Mostly used for the sake of debugging Kaldi's TCP server.
 *        Not really needed with the Ristretto server.
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

  std::string filteredText(startOfText.base(), endOfText.base());
  SPDLOG_INFO("Filtered: {}", filteredText);
  return filteredText;
}

} // namespace mik
