#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include <memory>

#include "RistrettoServer.hpp"
namespace mik {

/// @brief Placeholder name for the first decoding session
const std::string firstSessionToken = "firstSession";

/**
 * RistrettoServer::RistrettoServer
 */
// NOLINTNEXTLINE: Passing command line args to Kaldi
RistrettoServer::RistrettoServer(std::filesystem::path configPath, int argc, const char** argv)
    : sessionMapMutex_(), sessionMap_(), configFile_(std::move(configPath)), argc_(argc),
      argv_(argv) {

  SPDLOG_INFO("Inserting firstSessionToken placeholder");
  // Create an Nnet3Data in anticipation of a session
  sessionMap_.emplace(
      std::piecewise_construct, std::forward_as_tuple(firstSessionToken),
      std::forward_as_tuple(argc_, argv_)); // NOLINT: Passing command line args to Kaldi

  SPDLOG_INFO("Constructed RistrettoServer");
}

std::string RistrettoServer::decodeAudio(const std::string& sessionToken, uint32_t audioId,
                                         std::unique_ptr<std::string> audioDataPtr) {

  std::lock_guard<std::mutex> lock(sessionMapMutex_);

  // If the first session token is available, use that

  const auto firstSessionTokenPresent = sessionMap_.find(firstSessionToken) != sessionMap_.end();
  const auto currentSessionFound = sessionMap_.find(sessionToken) != sessionMap_.end();

  if (firstSessionTokenPresent) {
    // New session!
    // We're able to use the first slot in the map!
    auto mapNode = sessionMap_.extract(firstSessionToken);
    // Overwrite the firstSession key
    mapNode.key() = sessionToken;
    sessionMap_.insert(std::move(mapNode));

  } else if (currentSessionFound) {
    // Session is found in the map
    SPDLOG_INFO("Found session token {} in sessionMap_!", sessionToken);

  } else if (!currentSessionFound) {
    // Session is not found, add it
    SPDLOG_INFO("First appearance of session token {}", sessionToken);
    sessionMap_.emplace(std::piecewise_construct, std::forward_as_tuple(sessionToken),
                        std::forward_as_tuple(argc_, argv_));
  }

  // Pre-condition checks are over, now decode
  // Note: I tried just deferencing with sessionMap_[sessionToken].decodeAudio() but that somehow
  // compiled into a copy
  auto sessionIt = sessionMap_.find(sessionToken);
  return sessionIt->second.decodeAudio(sessionToken, audioId, std::move(audioDataPtr));
}
/**
 * RistrettoServer::~RistrettoServer
 */
RistrettoServer::~RistrettoServer() {
  server_->Shutdown();
  completionQueue_->Shutdown();
}

/**
 * RistrettoServer::run
 */
void RistrettoServer::run() {
  SPDLOG_DEBUG("run() start");
  /// @todo Read this value from configFile using nlohmann/json
  const std::string serverAddress("0.0.0.0:5050");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  completionQueue_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  fmt::print("Server listening on {}\n", serverAddress);

  handleRpcs();
}

/**
 * RistrettoServer::handleRpcs
 */
void RistrettoServer::handleRpcs() {
  new AsyncCallData(&service_, completionQueue_.get(), getServerReference());
  void* tag;
  bool ok;
  SPDLOG_DEBUG("about to process Rpcs");

  while (true) {

    GPR_ASSERT(completionQueue_->Next(&tag, &ok));
    GPR_ASSERT(ok);

    static_cast<AsyncCallData*>(tag)->proceed();
  }
}

/**
 * AsyncCallData::AsyncCallData
 */
AsyncCallData::AsyncCallData(RistrettoProto::Decoder::AsyncService* service,
                             grpc::ServerCompletionQueue* cq, RistrettoServer& serverRef)
    : service_(service), completionQueue_(cq), responder_(&ctx_), status_(CREATE),
      serverRef_(serverRef) {
  SPDLOG_DEBUG("Constructing AsyncCallData");
  proceed();
}

/**
 * AsyncCallData::proceed
 */
void AsyncCallData::proceed() {
  SPDLOG_DEBUG("Running AsyncCallData state machine with state:{}", static_cast<int>(status_));
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestDecodeAudio(&ctx_, &audioData_, &responder_, completionQueue_,
                                 completionQueue_, this);
  } else if (status_ == PROCESS) {
    new AsyncCallData(service_, completionQueue_, serverRef_);

    SPDLOG_DEBUG("Starting decoding...");
    const auto text =
        serverRef_.decodeAudio(audioData_.sessiontoken(), audioData_.audioid(),
                               std::unique_ptr<std::string>(audioData_.release_audio()));
    transcript_.set_text(text);
    transcript_.set_audioid(audioData_.audioid());
    transcript_.set_sessiontoken(audioData_.sessiontoken());

    status_ = FINISH;
    SPDLOG_DEBUG("Responding with transcript: {}", text);
    responder_.Finish(transcript_, grpc::Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}

} // namespace mik