#include <cstdio>

#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::playbackAudio(std::istream &inputStream) {

  logger_->info("Starting playback...");

  if (this->isConfiguredForPlayback()) {
    this->configureInterface(StreamConfig::PLAYBACK, pcmDesc_);
  }

  logger_->info("Calculating amount of recording loops...");
  int loopsLeft = config_.calculateRecordingLoops();
  logger_->info("Will be running {} loops", loopsLeft);
  logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

  const auto startStatus = snd_pcm_start(pcmHandle_.get());
  if (startStatus != 0) {
    logger_->error("Could not start PCM errno:{}", std::strerror(startStatus));
    snd_pcm_drop(pcmHandle_.get());
    return;
  }

  logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

  while (loopsLeft > 0) {
    --loopsLeft;

    // Read in bufferSize amount of bytes from the audio file into the buffer
    inputStream.read(buffer_.get(), bufferSize_);
    const auto bytesRead = inputStream.gcount();
    if (bytesRead == 0) {
      logger_->error("Hit unexpected EOF on audio input");
      break;
    } else if (bytesRead != bufferSize_) {
      logger_->error("Short read: should've read {} bytes, only read {}", bufferSize_, bytesRead);
    }

    // Write out a frame of data from the buffer into the soundcard
    const auto status = snd_pcm_writei(pcmHandle_.get(), buffer_.get(), config_.frames);
    if (status == -EPIPE) {
      // Overran the buffer
      logger_->error("Underrun occured, received EPIPE. Will continue");
      snd_pcm_prepare(pcmHandle_.get());
      snd_pcm_start(pcmHandle_.get());
      continue;
    } else if (status == -ESTRPIPE) {
      // Overran the buffer
      logger_->error("Received ESTRPIPE (Stream is suspended)e");
      break;
    } else if (status < 0) {
      logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
      break;
    } else if (status != static_cast<int>(config_.frames)) {
      logger_->error("Should've written {} frames, only wrote {}.", config_.frames, status);
      logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));
      snd_pcm_prepare(pcmHandle_.get());
      continue;
    }
  }

  snd_pcm_drop(pcmHandle_.get());
  logger_->info("Finished audio playback");
  return;
}

} // namespace mik
