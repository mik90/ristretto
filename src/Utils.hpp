#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mik {

class Utils {
public:
  static std::vector<char> readInAudioFile(std::string_view filename);
  static void createLogger();
};

} // namespace mik