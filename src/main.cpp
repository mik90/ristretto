#include <docopt/docopt.h>
#include <fmt/core.h>

#include "AlsaInterface.hpp"


static constexpr auto Usage =
R"(AlsaInterface.

    Usage:
          AlsaInterface capture <output_filename>
          AlsaInterface playback <input_audio_file>
          AlsaInterface (-h | --help)
          AlsaInterface (-v | --version)

    Options:
          -h --help             Show this screen.
          -v --version          Show the version.
)";



int main(int argc, char** argv) {

    auto args = docopt::docopt(Usage, { std::next(argv), std::next(argv, argc) },
                                true,// show help if requested
                                "AlsaInterface 0.1");// version string
    fmt::print("--------\ndocopt output:\n");
    for(auto const& arg : args) {
        std::cout << arg.first << ": " << arg.second << std::endl;
    }
    fmt::print("--------\n");

    const auto capture = args[std::string("capture")];
    const auto playback = args[std::string("playback")];
    
    const auto outputFilename = args[std::string("<output_filename>")];
    const auto inputAudioFile = args[std::string("<input_audio_file>")];

    if (capture && outputFilename) {
        mik::AlsaInterface alsa(mik::StreamConfig::CAPTURE, mik::defaultHw);
        alsa.captureAudio(outputFilename.asString());
    }
    else if (playback && inputAudioFile) {
        mik::AlsaInterface alsa(mik::StreamConfig::PLAYBACK, mik::defaultHw);
        alsa.playbackAudio(inputAudioFile.asString());
    }

    return 0;
}
