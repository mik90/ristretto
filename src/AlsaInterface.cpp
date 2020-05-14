#include "AlsaInterface.hpp"
#include "Utils.hpp"

namespace mik {

AlsaInterface::AlsaInterface(const AlsaConfig& alsaConfig) : config_(alsaConfig) {
  SPDLOG_INFO("Configuring AlsaInterface...");
  if (this->configureInterface() == Status::SUCCESS) {
    SPDLOG_INFO("Configured AlsaInterface successfully");
  } else {
    SPDLOG_ERROR("Could not configure AlsaInterface");
    return;
  }

  SPDLOG_INFO("--------------------------------------------");
  SPDLOG_INFO("AlsaInterface created.");
  SPDLOG_INFO("--------------------------------------------");
}

AlsaInterface::~AlsaInterface() {
  SPDLOG_INFO("Destroying AlsaInterface...");
  this->stopRecording();
  SPDLOG_INFO("Done destroying AlsaInterface");
}

snd_pcm_t* AlsaInterface::openSoundDevice(std::string_view pcmDesc, StreamConfig streamDir) {
  snd_pcm_t* pcmHandle = nullptr;

  const int err =
      snd_pcm_open(&pcmHandle, pcmDesc.data(), static_cast<snd_pcm_stream_t>(streamDir), 0x0);
  if (err != 0) {
    SPDLOG_ERROR("Couldn't get capture handle, error:{}", std::strerror(err));
    return nullptr;
  }
  const std::string iface = (streamDir == StreamConfig::CAPTURE) ? "CAPTURE" : "PLAYBACK";
  SPDLOG_INFO("Configured interface \'{}\' for {}", pcmDesc, iface);
  return pcmHandle;
}

Status AlsaInterface::updateConfiguration(const AlsaConfig& config) {
  config_ = config;
  return this->configureInterface();
}

Status AlsaInterface::configureInterface() {
  SPDLOG_INFO("Configuring interface...");

  // Reference: https://www.linuxjournal.com/article/6735
  pcmHandle_.reset(this->openSoundDevice(config_.pcmDesc, config_.streamConfig));
  if (!pcmHandle_) {
    SPDLOG_ERROR("Could not get handle for the capture device: {}.", config_.pcmDesc);
    return Status::ERROR;
  }

  SPDLOG_INFO("Allocating memory for parameters...");

  // TODO make an RAII wrapper, although it's a bit difficult as snd_pcm_hw_params is opaque from
  // this level of abstraction
  snd_pcm_hw_params_t* params = nullptr;
  // This macro allocates the memory
  snd_pcm_hw_params_alloca(&params);

  SPDLOG_INFO("Setting sound card parameters...");
  snd_pcm_hw_params_any(pcmHandle_.get(), params);
  snd_pcm_hw_params_set_format(pcmHandle_.get(), params, config_.format);
  snd_pcm_hw_params_set_channels(pcmHandle_.get(), params,
                                 static_cast<unsigned int>(config_.channelConfig));
  snd_pcm_hw_params_set_access(pcmHandle_.get(), params, config_.accessType);

  int dir = 0;
  // Set sampling rate
  unsigned int val = config_.samplingFreq_Hz;
  SPDLOG_INFO("Attempting to get a sampling freq of {} Hz", val);
  snd_pcm_hw_params_set_rate_near(pcmHandle_.get(), params, &val, &dir);
  // Check what rate we actually got
  SPDLOG_INFO("Got a sampling freq of {} Hz", val);

  // Set period size to X frames
  SPDLOG_INFO("Attempting to set period size to {} frames", config_.frames);
  snd_pcm_hw_params_set_period_size_near(pcmHandle_.get(), params, &(config_.frames), &dir);
  SPDLOG_INFO("Period size was set to {} frames", config_.frames);

  // Write parameters to the driver
  int status = snd_pcm_hw_params(pcmHandle_.get(), params);
  if (status != 0) {
    SPDLOG_ERROR("Could not write parameters to the sound driver!");
    SPDLOG_ERROR("snd_pcm_hw_params() errno:{}", std::strerror(errno));
    snd_pcm_hw_params_free(params);
    return Status::ERROR;
  }

  // Check how many frames we actually got
  snd_pcm_hw_params_get_period_size(params, &(config_.frames), &dir);
  SPDLOG_INFO("Retrieved period size of {} frames", config_.frames);

  // Figure out how long a period is
  snd_pcm_hw_params_get_period_time(params, &(config_.periodDuration_us), &dir);
  if (config_.periodDuration_us == 0) {
    SPDLOG_ERROR("Can't divide by a recording period of 0us!");
    snd_pcm_hw_params_free(params);
    return Status::ERROR;
  }
  SPDLOG_INFO("Retrieved period duration of {} us", config_.periodDuration_us);
  snd_pcm_hw_params_free(params);
  return Status::SUCCESS;
}

std::vector<char> AlsaInterface::consumeDurationOfAudioData(unsigned int milliseconds) {
  std::scoped_lock<std::mutex> lock(audioChunkMutex_);
  const auto duration_us = Utils::millisecondsToMicroseconds(milliseconds);

  const auto periodCount = duration_us / config_.periodDuration_us;
  const auto bytesToGet = periodCount * config_.periodSizeBytes;

  if (bytesToGet > audioData_.size()) {
    // If too many bytes are requested, just give it all that is available
    return std::exchange(audioData_, std::vector<char>{});
  } else if (bytesToGet == 0) {
    return std::vector<char>{};
  }

  // Split this vector into two. The lower indices wil be returned, the rest will be kept
  const auto startOfConsumedAudio = std::cbegin(audioData_);
  const auto endOfConsumedAudio =
      std::next(startOfConsumedAudio,
                static_cast<decltype(startOfConsumedAudio)::difference_type>(bytesToGet));

  const auto startOfRemainingAudio = std::next(endOfConsumedAudio);
  const auto endOfRemainingAudio = std::cend(audioData_);

  const std::vector<char> consumedAudio(startOfConsumedAudio, endOfConsumedAudio);

  std::vector<char> remainingAudio(startOfRemainingAudio, endOfRemainingAudio);
  audioData_.swap(remainingAudio);

  return consumedAudio;
}

} // namespace mik
