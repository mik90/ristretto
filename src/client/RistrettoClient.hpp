#pragma once

#include <string>
#include <string_view>
#include <memory>

#include <grpc++/grpc++.h>

#include "AlsaInterface.hpp"
#include "ristretto.grpc.pb.h"

namespace mik {
// Used for sending audio to the Kaldi online2-tcp-nnet3-decode-faster server
// Given a stream of audio, send it over TCP

std::string filterResult(const std::string& fullResult);

struct AsyncClientCall {
  ristretto::HelloReply reply;
  grpc::ClientContext context;
  grpc::Status;
  std::unique_ptr<grpc::ClientAsyncResponseReader<ristretto::HelloReply>> responseReader;
}

class RistrettoClient {
public:
  explicit RistrettoClient(std::shared_ptr<grpc::Channel> channel);
  void sendHello(const std::string& user);
  void asyncCompleteRpc();
  //size_t sendAudioToServer(const std::vector<char>& buffer);
  //std::string getResultFromServer();
  static std::string filterResult(const std::string& result);

private:
  std::unique_ptr<ristretto::Stub> stub_;
  grpc::CompletionQueue compQueue_;
};

} // namespace mik