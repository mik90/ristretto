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
  {
    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);
    params_ = params; // Grab the pointer we got from the ALSA macro
  }

  SPDLOG_INFO("Setting sound card parameters...");
  snd_pcm_hw_params_any(pcmHandle_.get(), params_);
  snd_pcm_hw_params_set_format(pcmHandle_.get(), params_, config_.format);
  snd_pcm_hw_params_set_channels(pcmHandle_.get(), params_,
                                 static_cast<unsigned int>(config_.channelConfig));
  snd_pcm_hw_params_set_access(pcmHandle_.get(), params_, config_.accessType);

  // Set sampling rate
  int dir = 0;
  unsigned int val = config_.samplingFreq_Hz;
  SPDLOG_INFO("Attempting to get a sampling freq of {} Hz", val);
  snd_pcm_hw_params_set_rate_near(pcmHandle_.get(), params_, &val, &dir);
  // Check what rate we actually got
  SPDLOG_INFO("Got a sampling freq of {} Hz", val);

  // Set period size to X frames
  SPDLOG_INFO("Attempting to set period size to {} frames", config_.frames);
  snd_pcm_hw_params_set_period_size_near(pcmHandle_.get(), params_, &(config_.frames), &dir);
  SPDLOG_INFO("Period size was set to {} frames", config_.frames);

  // Write parameters to the driver
  int status = snd_pcm_hw_params(pcmHandle_.get(), params_);
  if (status != 0) {
    SPDLOG_ERROR("Could not write parameters to the sound driver!");
    SPDLOG_ERROR("snd_pcm_hw_params() errno:{}", std::strerror(errno));
    return Status::ERROR;
  }

  // Check how many frames we actually got
  snd_pcm_hw_params_get_period_size(params_, &(config_.frames), &dir);
  SPDLOG_INFO("Retrieved period size of {} frames", config_.frames);

  // Figure out how long a period is
  snd_pcm_hw_params_get_period_time(params_, &(config_.recordingPeriod_us), &dir);
  if (config_.recordingPeriod_us == 0) {
    SPDLOG_ERROR("Can't divide by a recording period of 0ms!");
    std::exit(1);
  }
  SPDLOG_INFO("Retrieved recording period of {} ms", config_.recordingPeriod_us);

  audioChunkSize_ = config_.frames * 4; // 2 bytes/sample, 2 channel

  SPDLOG_INFO("Audio chunk size is {} bytes", audioChunkSize_);

  return Status::SUCCESS;
}
} // namespace mik
