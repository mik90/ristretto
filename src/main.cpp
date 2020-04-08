#include "AlsaInterface.hpp"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv) {
    mik::AlsaInterface alsa(mik::defaultHw);
    alsa.captureAudio();

    return 0;
}
