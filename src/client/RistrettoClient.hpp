#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

#include <grpc++/grpc++.h>

#include "AlsaInterface.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "ristretto.grpc.pb.h"
#pragma GCC diagnostic pop

namespace mik {

class RistrettoClient {
public:
  explicit RistrettoClient(std::shared_ptr<grpc::Channel> channel,
                           const AlsaConfig& config = AlsaConfig());
  std::string decodeAudioSync(const std::vector<char>& audio, unsigned int audioId = 0);
  void decodeMicrophoneInput();
  void setRecordingDuration(std::chrono::milliseconds milliseconds) {
    this->recordingTimeout_ = milliseconds;
  };

private:
  void recordAudioChunks();
  void renderResults();
  std::chrono::milliseconds chunkDuration_ = std::chrono::milliseconds(1000);

  /// @brief Stores captured audio in preparation for sending
  std::queue<RistrettoProto::AudioData> audioInputQ_;
  /// @brief Used for modifying the audioInputQ
  std::mutex audioInputMutex_;

  /// @brief This is thread safe according to https://github.com/grpc/grpc/issues/4486
  grpc::CompletionQueue resultCompletionQ_;

  std::unique_ptr<RistrettoProto::Decoder::Stub> stub_;

  std::atomic<bool> continueRecording_ = false;
  /// @brief Do not limit the recording duration if the timeout is 0
  std::chrono::milliseconds recordingTimeout_ = std::chrono::milliseconds(0);

  AlsaConfig config_;
  AlsaInterface alsa_;
};

struct ClientCallData {
  RistrettoProto::Transcript transcript;

  grpc::ClientContext context;

  grpc::Status status;

  std::unique_ptr<grpc::ClientAsyncResponseReader<RistrettoProto::Transcript>> responseReader;
};

std::string filterResult(const std::string& fullResult);

} // namespace mik