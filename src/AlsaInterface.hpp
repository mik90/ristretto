#ifndef MIK_ALSA_CONFIG_HPP_
#define MIK_ALSA_CONFIG_HPP_

#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cstring>

#include <alsa/asoundlib.h>
#include <spdlog/spdlog.h> 
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/core.h>

#include <string>
#include <memory>
#include <string_view>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace mik {

const std::string_view defaultHw("default");
const std::string_view intelPch("hw:PCH");
const std::string_view speechMike("hw:III");

enum class ChannelConfig : unsigned int {MONO = 1, STEREO = 2};

constexpr bool deleteOldLog = true; 
static auto logFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/client.log", deleteOldLog);

struct SndPcmDeleter {
    void operator()(snd_pcm_t* p) {
        int err = snd_pcm_close(p);
        if (err != 0) {
            std::cerr << "Could not close snd_pcm_t*, error:" << std::strerror(err) << std::endl;
        }
        p = nullptr;
    }
};

struct AlsaConfig {
    unsigned int samplingRate_bps = 44100;
    unsigned int recordingTime_us = 5000000;
    unsigned int recordingPeriod_us = 735;

    snd_pcm_uframes_t frames = 32;
    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
    snd_pcm_access_t accessType = SND_PCM_ACCESS_RW_INTERLEAVED;
    ChannelConfig channelConfig = ChannelConfig::STEREO; // Stereo
        
    int calculateRecordingLoops() {
        return static_cast<int>(recordingTime_us / recordingPeriod_us);
    }

};

class AlsaPlayback {
    public:
        AlsaPlayback(const AlsaConfig& config = AlsaConfig(), std::optional<std::filesystem::path> inputFile = {});
    private:
        std::unique_ptr<char> inputBuffer_;
        AlsaConfig config_;
        std::optional<std::filesystem::path> inputFile_ = {};
        std::unique_ptr<spdlog::logger> logger_;
};

class AlsaCapture {
    public:
        AlsaCapture(std::string_view captureDeviceName, const AlsaConfig& config = AlsaConfig());
        void captureAudio();
    private:
        AlsaConfig  config_;
        std::string captureDeviceName_;
        // Have to make params a raw pointer since the underlying type is opaque (apparently)
        snd_pcm_hw_params_t* params_;
        std::unique_ptr<snd_pcm_t, SndPcmDeleter> handle_;
        std::unique_ptr<char> buffer_;
        size_t bufferSize_;
       
        std::filesystem::path outputFile_;
        std::unique_ptr<spdlog::logger> logger_;

        // TODO
        // Write to file (.raw or maybe wav)
        // Record x seconds of audio
        // Use AlsaConfig
        snd_pcm_t* getSoundDeviceHandle(std::string_view captureDevice);
};

}
#endif