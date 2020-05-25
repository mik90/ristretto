#include <grpc++/grpc++.h>

#include "RistrettoServer.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    RistrettoServer server;
    server.run();
}