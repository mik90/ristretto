#pragma once

#include <fmt/core.h>
#include <grpc++/grpc++.h>
#include <memory>
#include <spdlog/spdlog.h>

#include "ristretto.grpc.pb.h"

// Reference:
// https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_server.cc

namespace mik {

class CallData {
  ristretto::Greeter::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;

  ristretto::HelloRequest request_;
  ristretto::HelloReply reply_;

  grpc::ServerAsyncResponseWriter<ristretto::HelloReply> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;

public:
  CallData(ristretto::Greeter::AsyncService* service, grpc::ServerCompletionQueue* cq)
      : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
    proceed();
  }

  void proceed() {
    if (status_ == CREATE) {
      status_ = PROCESS;

      service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if (status_ == PROCESS) {
      new CallData(service_, cq_);

      // The actual processing.
      std::string prefix("Hello ");
      reply_.set_message(prefix + request_.name());

      status_ = FINISH;
      responder_.Finish(reply_, grpc::Status::OK, this);
    } else {
      GPR_ASSERT(status_ == FINISH);
      delete this;
    }
  }
};

class RistrettoServer {
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  ristretto::Greeter::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
  // Thread safe
  void handleRpcs() {
    new CallData(&service_, cq_.get());
    void* tag;
    bool ok;
    while (true) {
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->proceed();
    }
  }

public:
  ~RistrettoServer() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  void run() {
    std::string serverAddress("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    fmt::print("Server listening on {}\n", serverAddress);

    handleRpcs();
  }
};

} // namespace mik
