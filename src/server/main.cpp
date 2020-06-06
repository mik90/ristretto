
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
#include "Utils.hpp"

int main(int argc, char* argv[]) {
  mik::createLogger();
  fmt::print("Created logger\n");
  mik::RistrettoServer server(argc, argv);
  fmt::print("Created server\n");
  server.run();
  fmt::print("Server exited\n");

  // If run() returns then that's an error
  return 1;
}