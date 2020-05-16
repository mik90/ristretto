#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mik {

class Utils {
public:
  static void createLogger(); // This needs to be called early on to configure the logger correctly

  static std::vector<char> readInAudioFile(std::string_view filename);

  // C libraries don't hvae std::chrono so conversion to primatives is needed
  // This is a bit less verbose than doing the conversoin each time
  static unsigned int secondsToMicroseconds(unsigned int seconds);
  static unsigned int millisecondsToMicroseconds(unsigned int milliseconds);

  static unsigned int microsecondsToSeconds(unsigned int microseconds);
};

} // namespace mik