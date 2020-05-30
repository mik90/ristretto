#pragma once

#include <memory>

#include <fmt/core.h>
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "ristretto.grpc.pb.h"
#pragma GCC diagnostic pop

// Reference:
// https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_server.cc

namespace mik {

class RistrettoServer {
public:
  ~RistrettoServer();
  void run();

private:
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  ristretto::Greeter::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
  // Thread safe
  void handleRpcs();
};

class CallData {
public:
  CallData(ristretto::Greeter::AsyncService* service, grpc::ServerCompletionQueue* cq);
  void proceed();

private:
  ristretto::Greeter::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;

  ristretto::HelloRequest request_;
  ristretto::HelloReply reply_;

  grpc::ServerAsyncResponseWriter<ristretto::HelloReply> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;
};

} // namespace mik
