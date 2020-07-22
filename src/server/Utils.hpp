#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

namespace mik {

class KaldiCommandLineArgs {
public:
  explicit KaldiCommandLineArgs(const std::filesystem::path& jsonPath);
  [[nodiscard]] int getArgCount() const noexcept { return static_cast<int>(argc_); }
  [[nodiscard]] char* const* getArgValues() noexcept { return argv_.get(); }
  std::vector<std::string> getUnderlyingStrings() { return underlyingStrings_; }

private:
  size_t argc_;
  std::unique_ptr<char*> argv_; // NOLINT
  std::vector<std::string> underlyingStrings_;
};

class Utils {
public:
  static void createLogger();
};

} // namespace mik