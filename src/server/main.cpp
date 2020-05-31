
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
#include "Utils.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  mik::createLogger();
  fmt::print("Created logger\n");
  mik::RistrettoServer server;
  fmt::print("Created server\n");
  server.run();
  return 0;
}