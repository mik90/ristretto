
#include "AlsaInterface.hpp"

namespace mik {

void AlsaInterface::captureAudio() {
    std::cout << "captureAudio entry\n";
    std::fstream out (outputFile_);
    if (out.is_open()) {
        std::cerr << "Could not open" << outputFile_ << std::endl;
        return;
    }

    int loopsLeft = calculateRecordingLoops();
    while (loopsLeft > 0) {
        --loopsLeft;
        auto status = snd_pcm_readi(handle_.get(), buffer_.get(), frames_);
        if (status == -EPIPE) {
            // Overran the buffer
            std::cerr << "EPIPE Overran buffer, hit EPIPE\n";
            snd_pcm_prepare(handle_.get());
            continue;
        }
        else if(status < 0) {
            logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
            return;
        }
        else if(status != static_cast<int>(frames_)) {
            logger_->error("Should've read 32 frames, only read {}.", frames_);
            snd_pcm_prepare(handle_.get());
            continue;
        }

        out.write(buffer_.get(), static_cast<std::streamsize>(bufferSize_));
    }

    snd_pcm_drain(handle_.get());
    return;
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
    unsigned int val = samplingRate_kHz_;
    snd_pcm_hw_params_set_rate_near(handle_.get(), params_, &val, &dir);

    // Set period size to X frames
    logger_->info("Attempting to set period size to {} frames", frames_);
    snd_pcm_hw_params_set_period_size_near(handle_.get(), params_, &frames_, &dir);
    logger_->info("Period size was sest to {} frames", frames_);

    // Write parameters to the driver
    int status = snd_pcm_hw_params(handle_.get(), params_);
    if (status != 0) {
        logger_->error("snd_pcm_hw_params() errno:{}", std::strerror(errno));
        return;
    }

    // Create a buffer to hold a period
    snd_pcm_hw_params_get_period_size(params_, &frames_, &dir);
    size_t bufferSize = frames_ * 4; // 2 bytes/sample, 2 channel
    buffer_.reset(static_cast<char*>(std::malloc(bufferSize)));
    bufferSize_ = bufferSize;

}

int AlsaInterface::calculateRecordingLoops() {
    unsigned int periodTimeMs = 0;
    int dir = 0;
    // How long to loop for
    int err = snd_pcm_hw_params_get_period_time(params_, &periodTimeMs, &dir);
    if (err != 0) {
        logger_->error("Couldn't get period time, error:{}", std::strerror(err));
        return 0;
    } 
    else if (periodTimeMs == 0) {
        std::cerr << "Don't divide by 0!\n";
        logger_->error("Can't divide by a period size of 0!");
        return 0;
    }
    return static_cast<int>(recordingTimeMs_ / periodTimeMs);
}


inline snd_pcm_t* AlsaInterface::getSoundDeviceHandle(std::string_view captureDevice) {
    snd_pcm_t* pcmHandle = nullptr;
    const int err = snd_pcm_open(&pcmHandle, captureDevice.data(), SND_PCM_STREAM_CAPTURE, 0x0);
    if (err != 0) {
        logger_->error("Couldn't get capture handle, error:{}", std::strerror(err));
        return nullptr;
    }
    return pcmHandle;
}

}
