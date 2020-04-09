#include "AlsaInterface.hpp"

namespace mik {
void AlsaCapture::captureAudio() {

    std::fstream outputFile (outputFile_, outputFile.trunc | outputFile.out);

    if (!outputFile.is_open()) {
        std::cerr << "Could not open" << outputFile_ << std::endl;
        return;
    }

    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = config_.calculateRecordingLoops();
    logger_->info("Will be running {} loops", loopsLeft);
    while (loopsLeft > 0) {
        --loopsLeft;
        auto status = snd_pcm_readi(handle_.get(), buffer_.get(), config_.frames);
        if (status == -EPIPE) {
            // Overran the buffer
            logger_->error("Overran buffer, received EPIPE. Will continue");
            snd_pcm_prepare(handle_.get());
            continue;
        }
        else if(status < 0) {
            logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
            return;
        }
        else if(status != static_cast<int>(config_.frames)) {
            logger_->error("Should've read 32 frames, only read {}.", config_.frames);
            snd_pcm_prepare(handle_.get());
            continue;
        }

        outputFile.write(buffer_.get(), static_cast<std::streamsize>(bufferSize_));
    }

    logger_->info("Wrote audio data to {}", static_cast<std::string>(outputFile_));
    //snd_pcm_drain(handle_.get());
    return;
}

}
