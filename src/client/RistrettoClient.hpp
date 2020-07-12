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

namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server
// Given a stream of audio, send it over TCP

std::string filterResult(const std::string& fullResult);

class RistrettoClient {
public:
  explicit RistrettoClient(std::shared_ptr<grpc::Channel> channel);
  std::string decodeAudio(const std::vector<char>& audio);
  void processAudioInput();

private:
  std::unique_ptr<RistrettoProto::Decoder::Stub> stub_;
};

} // namespace mik