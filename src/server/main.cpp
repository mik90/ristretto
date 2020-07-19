#include <filesystem>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
#include "Utils.hpp"

int main(int argc, const char** argv) {

  auto configPath = std::filesystem::path("./serverConfig.json");
  fmt::print("Pass in the path to serverConfig.json to override the default path of {}\n",
             configPath.string());
  if (argc > 1) {
    // Called with an arg
    configPath = std::filesystem::path(argv[1]);
    fmt::print("Using config file at {}\n", configPath.string());
  }

  mik::Utils::createLogger();
  fmt::print("Created logger\n");

  mik::RistrettoServer server(std::move(configPath), argc, argv);
  fmt::print("Created server\n");

  server.run();

  fmt::print("Server exited unexpectedly\n");
  return 1;
}