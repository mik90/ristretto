#pragma once

#include <grpc++/grpc++.h>
#include <memory>
#include <spdlog/spdlog.h>

#include "ristretto.grpc.pb.h"

// Reference:
// https://chromium.googlesource.com/external/github.com/grpc/grpc/+/chromium-deps/2016-08-17/examples/cpp/helloworld/greeter_async_server.cc

namespace mik {

class RequestProcessor {
public:
  RequestProcessor(ristretto::AsyncService* service, grpc::ServerCompletionQueue* compQueue)
      : service_(service), compQueue_(compQueue), responder_(&context_), status_(CREATE) {
    start();
  }

  void proceed() {
    if (status_ == CREATE) {
      status_ = PROCESS;
      service_->RequesySayHello(&context_, &request_, &responder_, compQueue_, compQueue_, this);
    } else if (status_ == PROCESS) {
      new RequestProcessor(service_, compQueue_);

      std::string prefix("Hello ");
      reply_.set_message(prefix + request_.name());

      status_ = FINISH;
      responder_.Finish(reply_, grpc::Status::OK, this);
    } else {
      delete this;
    }
  }

private:
    ristretto::AsyncService* service_;
    grpc::ServerCompletionQueue* compQueue_;
    grpc::ServerContext context_;
    ristretto::HelloRequest_;
    ristretto::HelloReply_;
    // For talking to the client
    grpc::ServerAsyncResponseWriter<HelloReply> responder_;

    enum CallStatus { CREATE, PROCESS, FINISH };
    CallStatus status_;

};

class RisrettoServer {
public:
  RisrettoServer(std::string_view port);
  ~RisrettoServer();
  void run();

private:
  void handleRpcs() {
    new RequestProcessor(&service_, compQueue_.get());
    void* tag;
    bool ok;
    while (true) {
      GPR_ASSERT(compQueue_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<RequestProcessor*>(tag)->proceed();
    }
  }

  std::unique_ptr<grpc::ServerCompletionQueue> compQueue_;
  ristretto::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;
};

} // namespace mik
