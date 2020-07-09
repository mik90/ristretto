#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "RistrettoServer.hpp"
namespace mik {

RistrettoServer::RistrettoServer(int argc, char* argv[]) : nnet3_(argc, argv) {
  SPDLOG_INFO("Constructed RistrettoServer");
}

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
  new CallData(&service_, cq_.get(), nnet3_.getDecoderLambda());
  void* tag;
  bool ok;
  SPDLOG_DEBUG("about to process Rpcs");

  while (true) {

    GPR_ASSERT(cq_->Next(&tag, &ok));
    GPR_ASSERT(ok);
    static_cast<CallData*>(tag)->proceed(nnet3_.getDecoderLambda());
  }
}

CallData::CallData(RistrettoProto::Decoder::AsyncService* service, grpc::ServerCompletionQueue* cq,
                   std::function<std::string(std::unique_ptr<std::string>)> decoderFunc)
    : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
  SPDLOG_DEBUG("Constructing CallData");
  proceed(std::move(decoderFunc));
}

void CallData::proceed(std::function<std::string(std::unique_ptr<std::string>)> decoderFunc) {
  SPDLOG_DEBUG("Running CallData state machine with state:{}", static_cast<int>(status_));
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestDecodeAudio(&ctx_, &audioData_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new CallData(service_, cq_, decoderFunc);

    SPDLOG_DEBUG("Constructed new CallData to replace old one");
    SPDLOG_DEBUG("Grabbing audioData...");
    // Grab the audiodata
    std::unique_ptr<std::string> audioDataPtr(audioData_.release_audio());
    // Call the decoder lambda
    SPDLOG_DEBUG("Starting decoding...");
    const auto text = decoderFunc(std::move(audioDataPtr));
    transcript_.set_text(text);

    status_ = FINISH;
    SPDLOG_DEBUG("Responding with transcript: {}", text);
    responder_.Finish(transcript_, grpc::Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}
} // namespace mik