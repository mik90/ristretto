#ifndef MIK_KALDI_CLIENT_HPP_
#define MIK_KALDI_CLIENT_HPP_

#include <alsa/asoundlib.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>
#include <string_view>

// This is the consumer-facing header

namespace mik {

constexpr std::string_view defaultHw("default");
constexpr std::string_view intelPch("hw:PCH");
constexpr std::string_view speechMike("hw:III");

// Was unsure if this needed to be signed or unsigned, just making it easy to switch
using AudioType = uint8_t;

enum class Status { SUCCESS, ERROR };

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
  StreamConfig streamConfig = StreamConfig::CAPTURE;
  std::string pcmDesc = static_cast<std::string>(defaultHw);

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
           channelConfig == rhs.channelConfig && streamConfig == rhs.streamConfig &&
           pcmDesc == rhs.pcmDesc;
  }
  inline bool operator!=(const AlsaConfig& rhs) const noexcept { return !(*this == rhs); }
};

class AlsaInterface {
public:
  AlsaInterface(const AlsaConfig& alsaConfig);
  void captureAudio(std::ostream& outputStream);
  std::vector<AudioType> captureAudioUntilUserExit();
  void playbackAudio(std::istream& inputStream);

private:
  Status configureInterface();
  Status updateConfiguration(const AlsaConfig& alsaConfig);
  inline bool isConfiguredForPlayback() const noexcept {
    return config_.streamConfig == StreamConfig::PLAYBACK;
  }
  inline bool isConfiguredForCapture() const noexcept {
    return config_.streamConfig == StreamConfig::CAPTURE;
  }
  snd_pcm_t* openSoundDevice(std::string_view pcmDesc, StreamConfig streamConfig);

  AlsaConfig config_;

  std::unique_ptr<char> inputBuffer_;
  // Have to make params a raw pointer since the underlying type is opaque (apparently)
  snd_pcm_hw_params_t* params_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  std::unique_ptr<char> buffer_;
  std::streamsize bufferSize_;
  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mik
#endif