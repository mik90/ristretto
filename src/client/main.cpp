#include <thread>
#include <cstdio>

#include "RistrettoClient.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    using namespace mik;

    std::string_view hostAndPort = "localhost:5050";
    RistrettoClient client(grpc::CreateChannel(hostAndPort.c_str()), grpc::InsecureChannelCredentials());

    // Indefinite loop
    std::thread t = std::thread(^RistrettoClient::asyncCompleteRpc, &client);

    for (int i = 0; i < 100; i++) {
        std::string user("world " + std::to_string(i));
        client.sayHello(user);
    }

    std::printf("Press ctrl-c to quit\n");
    t.join();
    return 0;
}