#include <docopt/docopt.h>
#include <fmt/core.h>

#include "AlsaInterface.hpp"


static constexpr auto Usage =
  R"(AlsaInterface.

    Usage:
          AlsaInterface capture [--file=<outputFile>]
          AlsaInterface playback [--file=<inputFile>]
    Options:
          --file=<board_file>   Input a file that contains a board state.
          -h --help             Show this screen.
          -v --version          Show the version.
)";



int main(int argc, char** argv) {

    mik::AlsaInterface alsa(mik::defaultHw);
    auto args = docopt::docopt(Usage, { std::next(argv), std::next(argv, argc) },
                                true,// show help if requested
                                "AlsaInterface 0.1");// version string

    const auto capture = args[std::string("capture")];
    const auto playback = args[std::string("playback")];
    
    const auto file = args[std::string("--file")];

    if (!file) {
        fmt::print("Did not see \'--file\' as an argument");
        fmt::print("Usage: {}", Usage);
    }

    if (capture) {
        alsa.captureAudio(file.asString());
    }
    else if (playback) {
        alsa.playbackAudio(file.asString());
    }

    return 0;
}
