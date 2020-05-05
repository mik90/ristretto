#include <chrono>
#include <condition_variable>
#include <thread>
#include <utility>
#include <vector>

#include "AlsaInterface.hpp"

namespace mik {

Recorder::~Recorder() { stopRecording(); }

std::vector<AudioType> Recorder::consumeAudioChunk() {
  if (audioChunks_.empty()) {
    logger_->info("No chunks left to consume, returning an empty chunk");
    return {};
  }

  logger_->info("Consuming oldest audio chunk out of {} chunks", audioChunks_.size());
  const auto frontChunk = audioChunks_.front();
  audioChunks_.pop_front();
  return frontChunk;
}

void Recorder::stopRecording() {
  shouldRecord_ = false;
  if (recordingThread_.joinable()) {
    recordingThread_.join();
  }
  if (pcmHandle_) {
    snd_pcm_drop(pcmHandle_.get());
  }
  logger_->info("Recording stopped.");
}

std::unique_ptr<snd_pcm_t, SndPcmDeleter> Recorder::takePcmHandle() {
  if (shouldRecord_ == false) {
    // Do not allow this during recording
    // Replaces pcmHandle_ with nullptr and returns pcmHandle_
    return std::exchange(pcmHandle_, nullptr);
  }
  return nullptr;
}

void Recorder::startRecording() {
  // Create the thread objec which will start off the recording
  shouldRecord_ = true;
  recordingThread_ = std::thread(&Recorder::recordingThread, this);
  logger_->info("Recording started.");
}

void Recorder::recordingThread() {
  logger_->debug("recordingThread: start");
  while (shouldRecord_) {
    std::vector<AudioType> audioBuffer;
    audioBuffer.reserve(audioChunkSize_);

    const auto status = snd_pcm_readi(pcmHandle_.get(), audioBuffer.data(), config_.frames);
    if (status == -EPIPE) {
      // Overran the buffer
      logger_->error("recordingThread: Overran buffer, received EPIPE. Will continue");
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    } else if (status < 0) {
      logger_->error("recordingThread: Error reading from pcm. errno:{}",
                     std::strerror(static_cast<int>(status)));
      return;
    } else if (status != static_cast<int>(config_.frames)) {
      logger_->error("recordingThread: Should've read {} frames, only read {}.", config_.frames,
                     status);
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    }

    {
      // Access the shared list, add this chunk of audio to it
      std::scoped_lock<std::mutex> lock(audioChunkMutex_);
      audioChunks_.emplace_back(std::move(audioBuffer));
    }
  }

  logger_->debug("recordingThread: end");
}

// Capture audio until user exits
void AlsaInterface::captureAudioUntilUserExit() {
  logger_->info("Starting capture until user exits...");

  // Capture audio until user presses a key
  if (!this->isConfiguredForCapture()) {
    logger_->debug("Interface was not configured for audio capture! Re-configuring...");
    this->configureInterface(StreamConfig::CAPTURE, pcmDesc_);
  }

  // Need to have an input thread and a recording thread
}

// Capture fixed duration of audio
void AlsaInterface::captureAudio(std::ostream& outputStream) {
  logger_->info("Starting capture...");

  if (!this->isConfiguredForCapture()) {
    logger_->debug("Interface was not configured for audio capture! Re-configuring...");
    this->configureInterface(StreamConfig::CAPTURE, pcmDesc_);
  }

  logger_->info("Calculating amount of recording loops...");
  logger_->info("Recording duration is {} microseconds ({} seconds as integer division)",
                config_.recordingDuration_us,
                AlsaConfig::microsecondsToSeconds(config_.recordingDuration_us));
  logger_->info("Recording period is {} microseconds ({} seconds as integer division)",
                config_.recordingPeriod_us,
                AlsaConfig::microsecondsToSeconds(config_.recordingPeriod_us));

  int loopsLeft = config_.calculateRecordingLoops();

  auto startTime = std::chrono::steady_clock::now();

  logger_->info("Will be running {} loops", loopsLeft);
  logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

  while (loopsLeft > 0) {
    --loopsLeft;
    auto status = snd_pcm_readi(pcmHandle_.get(), buffer_.get(), config_.frames);
    if (status == -EPIPE) {
      // Overran the buffer
      logger_->error("Overran buffer, received EPIPE. Will continue");
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    } else if (status < 0) {
      logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
      return;
    } else if (status != static_cast<int>(config_.frames)) {
      logger_->error("Should've read {} frames, only read {}.", config_.frames, status);
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    }

    outputStream.write(buffer_.get(), bufferSize_);
  }

  auto endTime = std::chrono::steady_clock::now();
  const auto actualDuration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
  logger_->debug("Capture was configured to take {} milliseconds, it actually took {} ms",
                 config_.recordingDuration_us / 1000, actualDuration);

  snd_pcm_drop(pcmHandle_.get());
  logger_->info("Finished audio capture");
  return;
}

} // namespace mik
