#pragma once

#include <list>
#include <memory>
#include <thread>

#include "AlsaInterface.hpp"

namespace mik {

class Recorder {
public:
  // TODO There has to be a more portable way to calculate audioChunkSize, but that would be a lot
  // of effort for something that I'd never use This done since S16 is 2 bytes and the default is
  // STEREO (2 channel)
  Recorder(std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle, const AlsaConfig& config)
      : shouldRecord_(false), recordingThread_(), audioChunkMutex_(), audioData_(),
        audioChunkSize_(config.frames * 4), pcmHandle_(std::move(pcmHandle)), config_(config),
        logger_(spdlog::get("AlsaLogger")){};

  void stopRecording();
  void startRecording();
  inline ~Recorder() { stopRecording(); };
  inline std::vector<AudioType> getAudioData() { return audioData_; }
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> takePcmHandle();

private:
  void record();

  std::atomic<bool> shouldRecord_;
  std::thread recordingThread_;

  std::mutex audioChunkMutex_;
  std::vector<AudioType> audioData_;
  size_t audioChunkSize_;

  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  AlsaConfig config_;

  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mik