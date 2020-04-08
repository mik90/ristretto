#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <cstring>

#include <alsa/asoundlib.h>

#include <vector>
#include <memory>
#include <optional>
#include <iostream>
#include <string_view>

#include "AlsaInterface.hpp"


int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv) {
    mik::AlsaInterface alsa(mik::defaultHw);
    alsa.captureAudio();

    return 0;
}

