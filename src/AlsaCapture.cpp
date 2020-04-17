#include <chrono>

#include "AlsaInterface.hpp"

namespace mik {

void AlsaInterface::captureAudio(std::ostream& outputStream) {

    logger_->info("Starting capture...");

    if (this->isConfiguredForCapture()) {
        this->configureInterface(StreamConfig::CAPTURE, pcmDesc_);
    }

    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = config_.calculateRecordingLoops();

    auto startTime = std::chrono::steady_clock::now();

    logger_->info("Will be running {} loops", loopsLeft);
    logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

    while (loopsLeft > 0) {
        --loopsLeft;
        auto status = snd_pcm_readi(pcmHandle_.get(), buffer_.get(), config_.frames);
        if (status == -EPIPE) {
            // Overran the buffer
            logger_->error("Overran buffer, received EPIPE. Will continue");
            snd_pcm_prepare(pcmHandle_.get());
            continue;
        }
        else if(status < 0) {
            logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
            return;
        }
        else if(status != static_cast<int>(config_.frames)) {
            logger_->error("Should've read {} frames, only read {}.", config_.frames, status);
            snd_pcm_prepare(pcmHandle_.get());
            continue;
        }
        
        outputStream.write(buffer_.get(), bufferSize_);
    }

    auto endTime = std::chrono::steady_clock::now();
    const auto actualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    logger_->info("Capture was configured to take {} milliseconds, it actually took {} ms",
                   config_.recordingTime_us / 1000, actualDuration);

    logger_->info("Capture done, running snd_pcm_drop...");
    logger_->flush();
    const auto status = snd_pcm_drop(pcmHandle_.get());
    if (status != 0) {
        logger_->error("snd_pcm_drop failed with error: {}", std::strerror(status));
    }
    logger_->info("Drop done.");

    /*
    logger_->info("Capture done, running snd_pcm_drain...");
    logger_->flush(); // it may hang on drain
    const auto status = snd_pcm_drain(pcmHandle_.get());
    if (status == -ESTRPIPE) {
        logger_->error("snd_pcm_drain was suspended during drain");
    }
    else if (status != 0) {
        logger_->error("snd_pcm_drain failed with error: {}", std::strerror(status));
    }
    logger_->info("Drain done.");
    */

    return;
}

}
