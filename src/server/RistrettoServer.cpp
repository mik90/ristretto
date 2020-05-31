#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include "RistrettoServer.hpp"
namespace mik {

RistrettoServer::~RistrettoServer() {
  server_->Shutdown();
  cq_->Shutdown();
}

void RistrettoServer::run() {
  SPDLOG_DEBUG("run() start");
  std::string serverAddress("0.0.0.0:5050");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  fmt::print("Server listening on {}\n", serverAddress);

  SPDLOG_DEBUG("run() end");
  handleRpcs();
}
void RistrettoServer::handleRpcs() {
  new CallData(&service_, cq_.get());
  void* tag;
  bool ok;
  SPDLOG_DEBUG("about to process Rpcs");
  while (true) {
    GPR_ASSERT(cq_->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallData*>(tag)->proceed();
  }
}

CallData::CallData(ristretto::Greeter::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
  SPDLOG_DEBUG("Constructing CallData");
  proceed();
}
void CallData::proceed() {
  SPDLOG_DEBUG("Running CallData state machine with state:{}", static_cast<int>(status_));
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
} // namespace mik