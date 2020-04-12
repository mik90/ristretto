#include <chrono>

#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::captureAudio(std::filesystem::path outputFile) {

    std::fstream outputStream (outputFile, outputStream.trunc | outputStream.out);

    if (!outputStream.is_open()) {
        logger_->error("Could not open {} for creating/writing", std::string(outputFile));
        return;
    }

    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = config_.calculateRecordingLoops();

    auto startTime = std::chrono::steady_clock::now();

    logger_->info("Will be running {} loops", loopsLeft);
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
            logger_->error("Should've read 32 frames, only read {}.", config_.frames);
            snd_pcm_prepare(pcmHandle_.get());
            continue;
        }

        outputStream.write(buffer_.get(), static_cast<std::streamsize>(bufferSize_));
    }

    auto endTime = std::chrono::steady_clock::now();
    const auto actualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    logger_->info("Capture was configured to take {} milliseconds, it actually took {} ms",
                   config_.recordingTime_us / 1000, actualDuration);

    logger_->info("Capture done, running snd_pcm_drain...");
    logger_->flush(); // it may hang on drain
    snd_pcm_drain(pcmHandle_.get());
    logger_->info("Drain done.");
    logger_->flush();
    return;
}

}
