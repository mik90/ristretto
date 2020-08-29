#include <filesystem>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
#include "Utils.hpp"

int main(int argc, const char** argv) {

  mik::Utils::createLogger();
  fmt::print("Created logger\n");

  mik::RistrettoServer server(argc, argv);
  fmt::print("Created server\n");

  server.run();

  fmt::print("Server exited unexpectedly\n");
  return 1;
}