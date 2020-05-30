
#include <fmt/core.h>
#include <fmt/locale.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
#include "Utils.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
  mik::createLogger();
  mik::RistrettoServer server;
  server.run();
  return 0;
}