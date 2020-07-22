#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "Utils.hpp"

namespace mik {

void Utils::createLogger() {
  static std::atomic<bool> hasBeenCalled(false);
  if (hasBeenCalled.load()) {
    return;
  } else {
    hasBeenCalled.store(true);
  }

  // [log level] has color enabled
  // [D/M/YR Hour:Month:Second.ms]     [thread id] [log level] [file::func():line] message
  spdlog::set_pattern("[%D %H:%M:%S.%e] [tid %t] [%^%l%$] [%s::%!():%#] %v");
  auto logger = spdlog::basic_logger_mt("RistrettoServerLogger", "logs/ristretto-server.log", true);
  spdlog::set_default_logger(logger);
  spdlog::flush_every(std::chrono::seconds(2));
  spdlog::set_level(spdlog::level::debug);
  SPDLOG_DEBUG("Debug-level logging enabled");
}

/**
 * @brief Gives Kaldi's ParseOptions a command line arg interface while
 * storing the underlying text in std::strings. DANGER: Extra crusty.
 */
KaldiCommandLineArgs::KaldiCommandLineArgs(const std::filesystem::path& jsonPath) {
  std::fstream inputStream(jsonPath, std::ios_base::in);
  const auto jsonObj = nlohmann::json::parse(inputStream);
  const auto kaldiArgs = jsonObj["kaldiCommandLineArgs"];

  // The name of the program is normally the first arg
  argc_ = kaldiArgs.size() + 1;
  argv_.reset(new char*[argc_]);

  size_t argvIndex = argc_;

  // Technically this value shouldn't matter to Kaldi, but should be the name of the running program
  underlyingStrings_.emplace_back("RistrettoServer");
  {
    // Pointer arithmetic. Crusty!
    argv_.get()[argvIndex++] = underlyingStrings_.back().data();
  }

  // Create strings for the rest of the args and copy their pointers
  for (auto& arg : kaldiArgs) {
    underlyingStrings_.push_back(arg);
    argv_.get()[argvIndex++] = underlyingStrings_.back().data();
  }
}
} // namespace mik