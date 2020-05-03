#ifndef MIK_ALSA_CONFIG_HPP_
#define MIK_ALSA_CONFIG_HPP_

#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include <alsa/asoundlib.h>
#include <fmt/core.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

namespace mik {

// TODO Separate out the headers into an general API header and internal headers

const std::string_view defaultHw("default");
const std::string_view intelPch("hw:PCH");
const std::string_view speechMike("hw:III");

enum class Status { SUCCESS, ERROR };

using AudioType = char;

enum class StreamConfig : unsigned int {
  PLAYBACK = SND_PCM_STREAM_PLAYBACK,
  CAPTURE = SND_PCM_STREAM_CAPTURE,
  UNKNOWN = 99
};
enum class ChannelConfig : unsigned int { MONO = 1, STEREO = 2 };

struct SndPcmDeleter {
  void operator()(snd_pcm_t* p) {
    int err = snd_pcm_close(p);
    if (err != 0) {
      std::cerr << "Could not close snd_pcm_t*, error:" << std::strerror(err) << std::endl;
    }
    p = nullptr;
  }
};

struct AlsaConfig {
  unsigned int samplingRate_bps = 44100;
  unsigned int recordingDuration_us = 5000000;
  unsigned int recordingPeriod_us = 735;

  snd_pcm_uframes_t frames = 32;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  snd_pcm_access_t accessType = SND_PCM_ACCESS_RW_INTERLEAVED;
  ChannelConfig channelConfig = ChannelConfig::STEREO; // Stereo

  static unsigned int microsecondsToSeconds(unsigned int microseconds) {
    return static_cast<unsigned int>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::microseconds(microseconds))
            .count());
  };
  static unsigned int secondsToMicroseconds(unsigned int seconds) {
    return static_cast<unsigned int>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(seconds))
            .count());
  };

  int calculateRecordingLoops() {
    return static_cast<int>(recordingDuration_us / recordingPeriod_us);
  }

  // Isn't there a way to automatically generate this?
  inline bool operator==(const AlsaConfig& rhs) const noexcept {
    return samplingRate_bps == rhs.samplingRate_bps &&
           recordingDuration_us == rhs.recordingDuration_us &&
           recordingPeriod_us == rhs.recordingPeriod_us && frames == rhs.frames &&
           format == rhs.format && accessType == rhs.accessType &&
           channelConfig == rhs.channelConfig;
  }
  inline bool operator!=(const AlsaConfig& rhs) const noexcept { return !(*this == rhs); }
};

class AlsaInterface {
public:
  AlsaInterface(const StreamConfig& streamConfig, std::string_view pcmDesc,
                const AlsaConfig& alsaConfig);
  void captureAudio(std::ostream& outputStream);
  void captureAudioUntilUserExit();
  void playbackAudio(std::istream& inputStream);
  // TODO Implemenet some way to update the configuration of an already-running AlsaInterface
private:
  Status configureInterface(const StreamConfig& streamConfig, std::string_view pcmDesc);
  inline bool isConfiguredForPlayback() const noexcept {
    return streamConfig_ == StreamConfig::PLAYBACK;
  }
  inline bool isConfiguredForCapture() const noexcept {
    return streamConfig_ == StreamConfig::CAPTURE;
  }
  snd_pcm_t* openSoundDevice(std::string_view pcmDesc, StreamConfig streamConfig);
  std::unique_ptr<char> inputBuffer_;
  AlsaConfig config_;
  std::shared_ptr<spdlog::logger> logger_;

  // Have to make params a raw pointer since the underlying type is opaque (apparently)
  snd_pcm_hw_params_t* params_;
  StreamConfig streamConfig_;
  std::string pcmDesc_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  std::unique_ptr<char> buffer_;
  std::streamsize bufferSize_;
};

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