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
#include <memory>
#include <string>
#include <string_view>

namespace mik {

const std::string_view defaultHw("default");
const std::string_view intelPch("hw:PCH");
const std::string_view speechMike("hw:III");

enum class Status { SUCCESS, ERROR };

enum class StreamConfig : unsigned int { PLAYBACK = SND_PCM_STREAM_PLAYBACK, CAPTURE = SND_PCM_STREAM_CAPTURE, UNKNOWN = 99 };
enum class ChannelConfig : unsigned int { MONO = 1, STEREO = 2 };

struct SndPcmDeleter {
  void operator()(snd_pcm_t *p) {
    int err = snd_pcm_close(p);
    if (err != 0) {
      std::cerr << "Could not close snd_pcm_t*, error:" << std::strerror(err) << std::endl;
    }
    p = nullptr;
  }
};

struct AlsaConfig {
  unsigned int samplingRate_bps = 44100;
  unsigned int recordingTime_us = 5000000;
  unsigned int recordingPeriod_us = 735;

  snd_pcm_uframes_t frames = 32;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  snd_pcm_access_t accessType = SND_PCM_ACCESS_RW_INTERLEAVED;
  ChannelConfig channelConfig = ChannelConfig::STEREO; // Stereo

  int calculateRecordingLoops() { return static_cast<int>(recordingTime_us / recordingPeriod_us); }
};

class AlsaInterface {
public:
  AlsaInterface(StreamConfig streamConfig, std::string_view pcmDesc = defaultHw);
  void captureAudio(std::ostream &outputStream);
  void playbackAudio(std::istream &inputStream);

private:
  Status configureInterface(StreamConfig streamConfig, std::string_view pcmDesc);
  inline bool isConfiguredForPlayback() const noexcept { return streamConfig_ == StreamConfig::PLAYBACK; }
  inline bool isConfiguredForCapture() const noexcept { return streamConfig_ == StreamConfig::CAPTURE; }
  snd_pcm_t *openSoundDevice(std::string_view pcmDesc, StreamConfig streamConfig);
  std::unique_ptr<char> inputBuffer_;
  AlsaConfig config_;
  std::shared_ptr<spdlog::logger> logger_;

  // Have to make params a raw pointer since the underlying type is opaque (apparently)
  snd_pcm_hw_params_t *params_;
  StreamConfig streamConfig_;
  std::string pcmDesc_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  std::unique_ptr<char> buffer_;
  std::streamsize bufferSize_;
};

} // namespace mik
#endif