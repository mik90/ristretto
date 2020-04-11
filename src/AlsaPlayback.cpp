#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::playbackAudio(std::filesystem::path inputFile) {

    std::fstream inputStream (inputFile, inputStream.in);

    if (!inputStream.is_open()) {
        logger_->error("Could not open {} for reading", std::string(inputFile));
        return;
    }

    // Read in bufferSize amount of bytes from the audio file into the buffer
    inputStream.read(buffer_.get(), static_cast<std::streamsize>(bufferSize_));

    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = config_.calculateRecordingLoops();
    logger_->info("Will be running {} loops", loopsLeft);

    while (loopsLeft > 0) {
        --loopsLeft;
        // Write out a frame of data from the file into the soundcard
        auto status = snd_pcm_writei(pcmHandle_.get(), buffer_.get(), config_.frames);
        if (status == -EPIPE) {
            // Overran the buffer
            logger_->error("Underrun occured, received EPIPE. Will continue");
            snd_pcm_prepare(pcmHandle_.get());
            continue;
        }
        else if(status < 0) {
            logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
            return;
        }
        else if(status != static_cast<int>(config_.frames)) {
            logger_->error("Should've written 32 frames, only wrote {}.", config_.frames);
            snd_pcm_prepare(pcmHandle_.get());
            continue;
        }

    }

    snd_pcm_drain(pcmHandle_.get());
    logger_->info("Finished reading audio from {}", static_cast<std::string>(inputFile));
    return;
}

}
