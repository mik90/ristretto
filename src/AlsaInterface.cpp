#include "AlsaInterface.hpp"

namespace mik {

snd_pcm_t* AlsaInterface::openSoundDevice(std::string_view pcmDesc, StreamConfig streamDir) {
  snd_pcm_t* pcmHandle = nullptr;

  const int err =
      snd_pcm_open(&pcmHandle, pcmDesc.data(), static_cast<snd_pcm_stream_t>(streamDir), 0x0);
  if (err != 0) {
    logger_->error("Couldn't get capture handle, error:{}", std::strerror(err));
    return nullptr;
  }
  const std::string iface = (streamDir == StreamConfig::CAPTURE) ? "CAPTURE" : "PLAYBACK";
  logger_->info("Configured interface \'{}\' for {}", pcmDesc, iface);
  return pcmHandle;
}

Status AlsaInterface::updateConfiguration(const AlsaConfig& config) {
  config_ = config;
  return this->configureInterface();
}

Status AlsaInterface::configureInterface() {
  logger_->info("Configuring interface...");

  // Reference: https://www.linuxjournal.com/article/6735
  pcmHandle_.reset(this->openSoundDevice(config_.pcmDesc, config_.streamConfig));
  if (!pcmHandle_) {
    logger_->error("Could not get handle for the capture device: {}.", config_.pcmDesc);
    return Status::ERROR;
  }

  logger_->info("Allocating memory for parameters...");
  {
    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);
    params_ = params; // Grab the pointer we got from the ALSA macro
  }

  logger_->info("Setting sound card parameters...");
  snd_pcm_hw_params_any(pcmHandle_.get(), params_);
  snd_pcm_hw_params_set_format(pcmHandle_.get(), params_, config_.format);
  snd_pcm_hw_params_set_channels(pcmHandle_.get(), params_,
                                 static_cast<unsigned int>(config_.channelConfig));
  snd_pcm_hw_params_set_access(pcmHandle_.get(), params_, config_.accessType);

  // Set sampling rate
  int dir = 0;
  unsigned int val = config_.samplingRate_bps;
  logger_->info("Attempting to get a sampling rate of {} bits per second", val);
  snd_pcm_hw_params_set_rate_near(pcmHandle_.get(), params_, &val, &dir);
  // Check what rate we actually got
  logger_->info("Got a sampling rate of {} bits per second", val);

  // Set period size to X frames
  logger_->info("Attempting to set period size to {} frames", config_.frames);
  snd_pcm_hw_params_set_period_size_near(pcmHandle_.get(), params_, &(config_.frames), &dir);
  logger_->info("Period size was set to {} frames", config_.frames);

  // Write parameters to the driver
  int status = snd_pcm_hw_params(pcmHandle_.get(), params_);
  if (status != 0) {
    logger_->error("Could not write parameters to the sound driver!");
    logger_->error("snd_pcm_hw_params() errno:{}", std::strerror(errno));
    return Status::ERROR;
  }

  // Check how many frames we actually got
  snd_pcm_hw_params_get_period_size(params_, &(config_.frames), &dir);
  logger_->info("Retrieved period size of {} frames", config_.frames);

  // Figure out how long a period is
  snd_pcm_hw_params_get_period_time(params_, &(config_.recordingPeriod_us), &dir);
  if (config_.recordingPeriod_us == 0) {
    logger_->error("Can't divide by a recording period of 0ms!");
    std::exit(1);
  }
  logger_->info("Retrieved recording period of {} ms", config_.recordingPeriod_us);

  audioChunkSize_ = config_.frames * 4; // 2 bytes/sample, 2 channel

  logger_->info("Audio chunk size is {} bytes", audioChunkSize_);

  return Status::SUCCESS;
}

AlsaInterface::AlsaInterface(const AlsaConfig& alsaConfig) : config_(alsaConfig) {

  if (spdlog::get("AlsaLogger")) {
    // The logger exists already
    logger_ = spdlog::get("AlsaLogger");
    logger_->set_level(spdlog::level::debug);
  } else {
    try {
      logger_ = spdlog::basic_logger_mt("AlsaLogger", "logs/client.log", true);
      logger_->set_level(spdlog::level::debug);
    } catch (const spdlog::spdlog_ex& e) {
      std::cerr << "Log init failed: " << e.what() << std::endl;
    }
  }

  logger_->info("Initialized logger");

  logger_->info("Configuring interface...");
  if (this->configureInterface() == Status::SUCCESS) {
    logger_->info("Configured interface successfully");
  } else {
    logger_->error("Could not configure AlsaInterface");
    return;
  }

  logger_->info("Finished AlsaInterface construction");
  logger_->info("");
}

AlsaInterface::~AlsaInterface() {
  logger_->info("Destroying AlsaInterface...");
  this->stopRecording();
  logger_->info("Done destroying AlsaInterface");
}

} // namespace mik
