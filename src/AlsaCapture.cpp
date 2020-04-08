#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::captureAudio() {
    std::fstream out (outputFile_);
    if (out.is_open()) {
        std::cerr << "Could not open" << outputFile_ << std::endl;
        return;
    }

    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = calculateRecordingLoops();
    logger_->info("Will be running {} loops", loopsLeft);
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

}
