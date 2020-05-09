#ifndef MIK_KALDI_CLIENT_HPP_
#define MIK_KALDI_CLIENT_HPP_

#include <alsa/asoundlib.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>
#include <string_view>
#include <thread>

// This is the consumer-facing header

namespace mik {

constexpr std::string_view defaultHw("default");
constexpr std::string_view intelPch("hw:PCH");
constexpr std::string_view speechMike("hw:III");

// Was unsure if this needed to be signed or unsigned, just making it easy to switch
using uint8_t = uint8_t;

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
  // Is this samples per second (Hz)? I can't quite figure this out.
  // 44,100 Hz is CD-quality and too high for ASR
  // 16,000 Hz would be best
  unsigned int samplingRate_bps = 44100;
  // Not necessary if recording is indefinite
  unsigned int recordingDuration_us = 5000000;
  // Not necessary if recording is indefinite
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
  ~AlsaInterface();

  void captureAudio(std::ostream& outputStream);
  std::vector<uint8_t> captureAudioUntilUserExit();
  void playbackAudio(std::istream& inputStream);
  Status updateConfiguration(const AlsaConfig& alsaConfig);

  Status configureInterface();
  const AlsaConfig& getConfiguration() { return config_; }
  inline bool isConfiguredForPlayback() const noexcept {
    return config_.streamConfig == StreamConfig::PLAYBACK;
  }
  inline bool isConfiguredForCapture() const noexcept {
    return config_.streamConfig == StreamConfig::CAPTURE;
  }
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> takePcmHandle() {
    return std::exchange(pcmHandle_, nullptr);
  }
  void stopRecording();
  void startRecording();
  std::vector<uint8_t> getAudioData() { return audioData_; }

private:
  snd_pcm_t* openSoundDevice(std::string_view pcmDesc, StreamConfig streamConfig);
  void record();

  AlsaConfig config_;

  std::atomic<bool> shouldRecord_;
  std::thread recordingThread_;

  std::mutex audioChunkMutex_;
  std::vector<uint8_t> audioData_;

  std::unique_ptr<char> inputBuffer_;
  // Have to make params a raw pointer since the underlying type is opaque (apparently)
  snd_pcm_hw_params_t* params_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
  // Amount of bytes that will be written to/read form each cycle
  size_t audioChunkSize_;
  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mik
#endif