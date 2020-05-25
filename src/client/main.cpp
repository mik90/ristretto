#include <iostream>

#include "RistrettoClient.hpp"
#include <grpcpp/grpcpp.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  using namespace mik;

  RistrettoClient client(grpc::CreateChannel("localhost:5050", grpc::InsecureChannelCredentials()));

  std::string user("world");
  std::string reply = client.sendHello(user);
  std::cout << "Received:" << reply << std::endl;
  return 0;
}