#include <cstdio>

#include <spdlog/spdlog.h>

#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::playbackAudio(std::istream& inputStream) {
  SPDLOG_INFO("Starting playback...");

  if (!this->isConfiguredForPlayback()) {
    SPDLOG_DEBUG("Interface was not configured for audio playback! Re-configuring...");
    config_.streamConfig = StreamConfig::PLAYBACK;
    // Re-configure the interface with the new config
    this->configureInterface();
  }

  SPDLOG_INFO("Calculating amount of recording loops...");
  int loopsLeft = config_.calculateRecordingLoops();
  SPDLOG_INFO("Will be running {} loops", loopsLeft);
  SPDLOG_INFO("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

  const auto startStatus = snd_pcm_start(pcmHandle_.get());
  if (startStatus != 0) {
    SPDLOG_ERROR("Could not start PCM errno:{}", std::strerror(startStatus));
    return;
  }

  auto cBuffer = std::make_unique<char[]>(audioChunkSize_);
  SPDLOG_INFO("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

  while (loopsLeft > 0) {
    --loopsLeft;

    // Read in audioChunkSize_ amount of bytes from the audio file into the buffer
    inputStream.read(cBuffer.get(), static_cast<std::streamsize>(audioChunkSize_));
    const auto bytesRead = inputStream.gcount();
    if (bytesRead == 0) {
      SPDLOG_ERROR("Hit unexpected EOF on audio input");
      break;
    } else if (bytesRead != static_cast<std::streamsize>(audioChunkSize_)) {
      SPDLOG_ERROR("Short read: should've read {} bytes, only read {}", audioChunkSize_, bytesRead);
    }

    // Write out a frame of data from the buffer into the soundcard
    const auto status = snd_pcm_writei(pcmHandle_.get(), cBuffer.get(), config_.frames);
    if (status == -EPIPE) {
      // Overran the buffer
      SPDLOG_WARN("Underrun occured, received EPIPE. Will continue");
      snd_pcm_prepare(pcmHandle_.get());
      snd_pcm_start(pcmHandle_.get());
      continue;
    } else if (status == -ESTRPIPE) {
      // Overran the buffer
      SPDLOG_ERROR("Received ESTRPIPE (Stream is suspended)e");
      break;
    } else if (status < 0) {
      SPDLOG_ERROR("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
      break;
    } else if (status != static_cast<int>(config_.frames)) {
      SPDLOG_WARN("Should've written {} frames, only wrote {}.", config_.frames, status);
      SPDLOG_INFO("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    }
  }

  snd_pcm_drop(pcmHandle_.get());
  SPDLOG_INFO("Finished audio playback");
  return;
}

} // namespace mik
