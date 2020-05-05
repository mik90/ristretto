#ifndef MIK_ALSA_CONFIG_HPP_
#define MIK_ALSA_CONFIG_HPP_

#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <thread>

#include "KaldiClient.hpp"

namespace mik {

class Recorder {
public:
  // TODO There has to be a more portable way to calculate audioChunkSize, but that would be a lot
  // of effort for something that I'd never use This done since S16 is 2 bytes and the default is
  // STEREO (2 channel)
  Recorder(std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle, const AlsaConfig& config)
      : shouldRecord_(false), recordingThread_(), audioChunkSize_(config_.frames * 4),
        pcmHandle_(std::move(pcmHandle)), config_(config), logger_(spdlog::get("AlsaLogger")),
        audioChunkMutex_(), audioChunks_(){};
  ~Recorder();
  void stopRecording();
  void startRecording();
  std::vector<AudioType> consumeAudioChunk();
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> takePcmHandle();

private:
  void recordingThread();

  std::atomic<bool> shouldRecord_;
  std::thread recordingThread_;
  size_t audioChunkSize_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  AlsaConfig config_;
  std::shared_ptr<spdlog::logger> logger_;
  std::mutex audioChunkMutex_;
  // These chunks will not be tiny and making them contiguous is a waste of effort
  // so a list makes most sense to avoid trying to find a large chunk of continguous memory
  std::list<std::vector<AudioType>> audioChunks_;
};

} // namespace mik

#endif