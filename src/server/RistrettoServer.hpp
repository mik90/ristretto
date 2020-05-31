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
  RistrettoServer(int argc, char* argv[]) : nnet3Data_(argc, argv) {
    SPDLOG_INFO("Created RistrettoServer");
  };
  ~RistrettoServer();
  void run();

private:
  void handleRpcs();
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  ristretto::Decoder::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
  kaldi::Nnet3Data nnet3Data_;
  // Thread safe
};

class CallData {
public:
  CallData(ristretto::Decoder::AsyncService* service, grpc::ServerCompletionQueue* cq);
  void proceed();

private:
  ristretto::Decoder::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;

  ristretto::AudioData audioData_;
  ristretto::Transcript transcript_;

  grpc::ServerAsyncResponseWriter<ristretto::Transcript> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;
};

} // namespace mik
