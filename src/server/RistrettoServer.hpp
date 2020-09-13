#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include <fmt/core.h>
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include "KaldiInterface.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast" // NOLINT: Clang-tidy is not aware of the warning,
#include "ristretto.grpc.pb.h"                  // it is a gcc warning after all
#pragma GCC diagnostic pop

// Reference:
// https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_server.cc

namespace mik {

/**
 * RistrettoServer
 * @brief Top level class that's to be instantiated in main() and ran
 * @todo Be const-correct with the server reference and decodeAudio. They're meant to be called by
 * multiple threads
 */
class RistrettoServer {
public:
  // NOLINTNEXTLINE: Passing command line args to Kaldi
  explicit RistrettoServer(int argc, const char** argv);
  // There should only be one, ensure that this is not copied
  RistrettoServer(const RistrettoServer&) = delete;
  RistrettoServer(RistrettoServer&&) = delete;
  virtual ~RistrettoServer();

  void run();

  // Give AsyncCallData objects the ability to use the single server instance
  [[nodiscard]] RistrettoServer& getServerReference() { return *this; }

  // Any async call should be able to call this function
  [[nodiscard]] std::string decodeAudio(const std::string& sessionToken, uint32_t audioId,
                                        std::unique_ptr<std::string> audioDataPtr);

private:
  static constexpr std::string_view DefaultServerAddress = "0.0.0.0:5050";
  void updateSessionMap(const std::string& sessionToken);

  void handleRpcs();
  std::unique_ptr<grpc::ServerCompletionQueue> completionQueue_;
  RistrettoProto::Decoder::AsyncService service_;
  std::unique_ptr<grpc::Server> server_;

  std::mutex sessionMapMutex_;
  /// @brief SessionToken mapped to Nnet3Data
  std::map<std::string, mik::Nnet3Data> sessionMap_;

  // Kaldi cmd line args
  const int argc_;    // NOLINT: Passing command line args to Kaldi
  const char** argv_; // NOLINT: Passing command line args to Kaldi
};

class AsyncCallData {
public:
  AsyncCallData(RistrettoProto::Decoder::AsyncService* service, grpc::ServerCompletionQueue* cq,
                RistrettoServer& serverRef);
  void proceed();

private:
  RistrettoProto::Decoder::AsyncService* service_;
  grpc::ServerCompletionQueue* completionQueue_;
  grpc::ServerContext ctx_;

  RistrettoProto::AudioData audioData_;
  RistrettoProto::Transcript transcript_;

  grpc::ServerAsyncResponseWriter<RistrettoProto::Transcript> responder_;

  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;

  RistrettoServer& serverRef_;
};

} // namespace mik
