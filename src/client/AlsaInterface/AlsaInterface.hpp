#ifndef MIK_KALDI_CLIENT_HPP_
#define MIK_KALDI_CLIENT_HPP_

#include <alsa/asoundlib.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

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
  // 44,100 Hz is CD-quality and too high for ASR
  // 16,000 Hz would be best, although the fisher-english corpus is recorded with 8,000 Hz
  // and that's what I'm using right now
  unsigned int samplingFreq_Hz = 8000;
  unsigned int periodDuration_us = 735;

  ChannelConfig channelConfig = ChannelConfig::STEREO;
  snd_pcm_uframes_t frames = 32;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  // 2 bytes/sample (since signed 16-bit), and 2 channel (since it's Stereo);
  // Could this be done programatically instead of hard-coded? Yes
  // Will I write a function for that? No, I probably won't even adjust these settings.
  size_t periodSizeBytes = frames * 4;
  snd_pcm_access_t accessType = SND_PCM_ACCESS_RW_INTERLEAVED;
  StreamConfig streamConfig = StreamConfig::CAPTURE;
  std::string pcmDesc = static_cast<std::string>(defaultHw);

  inline int calculateRecordingLoops(unsigned int recordingDuration_us) {
    return static_cast<int>(recordingDuration_us / periodDuration_us);
  }

  // Isn't there a way to automatically generate this?
  inline bool operator==(const AlsaConfig& rhs) const noexcept {
    return samplingFreq_Hz == rhs.samplingFreq_Hz && periodDuration_us == rhs.periodDuration_us &&
           frames == rhs.frames && format == rhs.format && accessType == rhs.accessType &&
           channelConfig == rhs.channelConfig && streamConfig == rhs.streamConfig &&
           pcmDesc == rhs.pcmDesc;
  }
  inline bool operator!=(const AlsaConfig& rhs) const noexcept { return !(*this == rhs); }
};

class AlsaInterface {
public:
  AlsaInterface(const AlsaConfig& alsaConfig);
  virtual ~AlsaInterface();

  void captureAudioFixedSizeMs(std::ostream& outputStream, unsigned int milliseconds);
  std::vector<char> captureAudioFixedSizeMs(unsigned int milliseconds);
  std::vector<char> captureAudioUntilUserExit();
  void playbackAudioFixedSize(std::istream& inputStream, unsigned int seconds);
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
  std::vector<char> consumeAllAudioData();
  std::vector<char> consumeDurationOfAudioData(unsigned int milliseconds);

protected:
  snd_pcm_t* openSoundDevice(std::string_view pcmDesc, StreamConfig streamConfig);
  void record();

  AlsaConfig config_;

  std::atomic<bool> shouldRecord_;
  std::thread recordingThread_;

  std::mutex audioChunkMutex_;
  std::vector<char> audioData_;
  std::unique_ptr<snd_pcm_t, SndPcmDeleter> pcmHandle_;
};

} // namespace mik
#endif