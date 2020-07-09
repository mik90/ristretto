#pragma once

#include <memory>

#include <fmt/core.h>
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include "KaldiInterface.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "ristretto.grpc.pb.h"
#pragma GCC diagnostic pop

// Reference:
// https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_server.cc

namespace mik {

class RistrettoServer {
public:
  RistrettoServer(int argc, char* argv[]);
  ~RistrettoServer();
  void run();

private:
  // Thread safe
  void handleRpcs();
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  RistrettoProto::Decoder::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
  kaldi::Nnet3Data nnet3_;
};

class CallData {
public:
  CallData(RistrettoProto::Decoder::AsyncService* service, grpc::ServerCompletionQueue* cq,
           std::function<std::string(std::unique_ptr<std::string>)> decoderFunc);
  void proceed(std::function<std::string(std::unique_ptr<std::string>)> decoderFunc);

private:
  RistrettoProto::Decoder::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;

  RistrettoProto::AudioData audioData_;
  RistrettoProto::Transcript transcript_;

  grpc::ServerAsyncResponseWriter<RistrettoProto::Transcript> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;
};

} // namespace mik
