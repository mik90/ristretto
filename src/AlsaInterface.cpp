
#include "AlsaInterface.hpp"

namespace mik {


inline snd_pcm_t* AlsaInterface::getSoundDeviceHandle(std::string_view captureDevice) {
    snd_pcm_t* pcmHandle = nullptr;
    const int err = snd_pcm_open(&pcmHandle, captureDevice.data(), SND_PCM_STREAM_CAPTURE, 0x0);
    if (err != 0) {
        logger_->error("Couldn't get capture handle, error:{}", std::strerror(err));
        return nullptr;
    }
    return pcmHandle;
}


AlsaInterface::AlsaInterface(std::string_view captureDeviceName) : captureDeviceName_(captureDeviceName),
                                                                   handle_(nullptr, SndPcmDeleter{}),
                                                                   outputFile_("./output.raw") {


    try {
        constexpr bool deleteOldLog = true; 
        logger_ = spdlog::basic_logger_mt("logger", "logs/client.log", deleteOldLog); 
    } catch (const spdlog::spdlog_ex& ex) { 
        std::cerr << "Could not init spdlog: " << ex.what() << std::endl; 
        std::exit(1); 
    } 
    logger_->info("Initialized logger");

    // Reference: https://www.linuxjournal.com/article/6735
    handle_.reset(getSoundDeviceHandle(captureDeviceName_));
    if (!handle_) {
        logger_->error("Could not get handle for the capture device: {}.", captureDeviceName_);
        return;
    }

    logger_->info("Allocating memory for parameters...");
    {
        snd_pcm_hw_params_t* params = nullptr;
        snd_pcm_hw_params_alloca(&params);
        params_ = params; // Grab the pointer we got from the ALSA macro
    }

    logger_->info("Setting sound card parameters...");
    snd_pcm_hw_params_any(handle_.get(), params_);
    snd_pcm_hw_params_set_format(handle_.get(), params_, format_);
    snd_pcm_hw_params_set_channels(handle_.get(), params_, static_cast<unsigned int>(channelConfig_));
    snd_pcm_hw_params_set_access(handle_.get(), params_, accessType_);

    // Set sampling rate
    int dir = 0;
    unsigned int val = samplingRate_bps_;
    logger_->info("Attempting to get a sampling rate of {} bits per second", val);
    snd_pcm_hw_params_set_rate_near(handle_.get(), params_, &val, &dir);
    logger_->info("Got a sampling rate of {} bits per second", val);

    // Set period size to X frames
    logger_->info("Attempting to set period size to {} frames", frames_);
    snd_pcm_hw_params_set_period_size_near(handle_.get(), params_, &frames_, &dir);
    logger_->info("Period size was set to {} frames", frames_);

    // Write parameters to the driver
    int status = snd_pcm_hw_params(handle_.get(), params_);
    if (status != 0) {
        logger_->error("Could not write parameters to the sound driver!");
        logger_->error("snd_pcm_hw_params() errno:{}", std::strerror(errno));
        return;
    }

    // Create a buffer to hold a period
    snd_pcm_hw_params_get_period_size(params_, &frames_, &dir);
    logger_->info("Retrieved period size of {} frames", frames_);
    
    // Figure out how long a period is
    snd_pcm_hw_params_get_period_time(params_, &recordingPeriodMs_, &dir);
    if (recordingPeriodMs_ == 0) {
        logger_->error("Can't divide by a recording period of 0ms!");
        std::exit(1);
    }
    logger_->info("Retrieved recording period of {} ms", recordingPeriodMs_);

    size_t bufferSize = frames_ * 4; // 2 bytes/sample, 2 channel
    logger_->info("Buffer size is {} bytes", bufferSize);
    buffer_.reset(static_cast<char*>(std::malloc(bufferSize)));
    bufferSize_ = bufferSize;

}

int AlsaInterface::calculateRecordingLoops() {
    return static_cast<int>(recordingTimeMs_ / recordingPeriodMs_);
}

}
