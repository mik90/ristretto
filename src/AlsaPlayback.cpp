#include <cstdio>

#include "AlsaInterface.hpp"

namespace mik {
void AlsaInterface::playbackAudio(std::filesystem::path inputFile) {

    //std::fstream inputStream (inputFile, inputStream.in);
    std::FILE* f = std::fopen(inputFile.c_str(), "r");

    if (!f) {
        logger_->error("Could not open {} for reading", std::string(inputFile));
        return;
    }
    
    logger_->info("Calculating amount of recording loops...");
    int loopsLeft = config_.calculateRecordingLoops();
    logger_->info("Will be running {} loops", loopsLeft);
    logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

    const auto startStatus = snd_pcm_start(pcmHandle_.get());
    if (startStatus != 0) {
        logger_->error("Could not start PCM errno:{}", std::strerror(startStatus));
        std::fclose(f);
        return;
    }
    
    logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));

    while (loopsLeft > 0) {
        --loopsLeft;

        // Read in bufferSize amount of bytes from the audio file into the buffer
        const auto bytesRead = std::fread(buffer_.get(), sizeof(char), bufferSize_, f);
        if (bytesRead == 0) {
            logger_->error("Hit unexpected EOF on audio input");
            break;
        }
        else if (bytesRead != bufferSize_) {
            logger_->error("Short read: should've read {} bytes, only read {}", bufferSize_, bytesRead);
        }

        // Write out a frame of data from the buffer into the soundcard
        const auto status = snd_pcm_writei(pcmHandle_.get(), buffer_.get(), config_.frames);
        if (status == -EPIPE) {
            // Overran the buffer
            logger_->error("Underrun occured, received EPIPE. Will continue");
            snd_pcm_prepare(pcmHandle_.get());
            snd_pcm_start(pcmHandle_.get());
            continue;
        }
        else if (status == -ESTRPIPE) {
            // Overran the buffer
            logger_->error("Received ESTRPIPE (Stream is suspended)e");
            break;
        }
        else if(status < 0) {
            logger_->error("Error reading from pcm. errno:{}", std::strerror(static_cast<int>(status)));
            break;
        }
        else if(status != static_cast<int>(config_.frames)) {
            logger_->error("Should've written {} frames, only wrote {}.", config_.frames, status);
            logger_->info("PCM State: {}", snd_pcm_state_name(snd_pcm_state(pcmHandle_.get())));
            //snd_pcm_prepare(pcmHandle_.get());
            //continue;
            break;
        }

    }

    snd_pcm_drop(pcmHandle_.get());
    logger_->info("Finished reading audio from {}", static_cast<std::string>(inputFile));
    logger_->flush();
    std::fclose(f);
    return;
}

}
