#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <grpc++/grpc++.h>

#include "AlsaInterface.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "ristretto.grpc.pb.h"
#pragma GCC diagnostic pop

using ristretto::Greeter;
namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server
// Given a stream of audio, send it over TCP

std::string filterResult(const std::string& fullResult);

class RistrettoClient {
public:
  explicit RistrettoClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(Greeter::NewStub(channel)){};
  std::string sendHello(const std::string& user);
  static std::string filterResult(const std::string& result);

private:
  std::unique_ptr<Greeter::Stub> stub_;
};

} // namespace mik