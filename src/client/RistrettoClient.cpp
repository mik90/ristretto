#include <algorithm>
#include <fstream>
#include <iostream>

#include <fmt/core.h>
#include <fmt/locale.h>
#include <grpc/support/log.h>
#include <spdlog/spdlog.h>

#include "RistrettoClient.hpp"
#include "Utils.hpp"

namespace mik {

RistrettoClient::RistrettoClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(ristretto::Decoder::NewStub(channel)) {
  SPDLOG_INFO("Constructed RistrettoClient");
}

std::string RistrettoClient::decodeAudio(const std::vector<char>& audio) {

  SPDLOG_DEBUG("decodeAudio start");
  ristretto::AudioData audioData;
  SPDLOG_DEBUG("audio vector size: {}", audio.size());
  audioData.set_audio(audio.data(), audio.size());
  SPDLOG_DEBUG("audioData bytes: {}", audioData.ByteSizeLong());
  if (audioData.ByteSizeLong() == 0) {
    SPDLOG_ERROR("audioData is empty, abandoning RPC");
    return {};
  }

  grpc::ClientContext context;
  grpc::CompletionQueue cq;
  grpc::Status status;

  std::unique_ptr<grpc::ClientAsyncResponseReader<ristretto::Transcript>> rpc(
      stub_->AsyncDecodeAudio(&context, audioData, &cq));

  ristretto::Transcript transcipt;
  rpc->Finish(&transcipt, &status, reinterpret_cast<void*>(1));
  void* got_tag;
  bool ok = false;
  GPR_ASSERT(cq.Next(&got_tag, &ok));
  GPR_ASSERT(got_tag == reinterpret_cast<void*>(1));
  GPR_ASSERT(ok);

  SPDLOG_DEBUG("decodeAudio end");
  if (status.ok()) {
    return transcipt.text();
  } else {
    SPDLOG_ERROR("Error with RPC: Error code:{}, details:{}", status.error_code(),
                 status.error_message());
    return {};
  }
}

// Strip all the newlines until we get to text
// use all that text until we get to the newline
std::string filterResult(const std::string& fullResult) {
  SPDLOG_INFO("Initial:{}", fullResult);
  // Strip the temporary transcript to get the final result
  auto endOfText = fullResult.crbegin();
  while (*endOfText == '\n' || *endOfText == ' ') {
    // Keep on skipping while the char is a newline or whitespace
    ++endOfText;
  }

  auto startOfText = endOfText;
  while (*startOfText != '\n') {
    // Keep on skipping until we hit another newline
    ++startOfText;
  }

  // Use these as forward iterators
  const auto textSize = endOfText.base() - startOfText.base();
  if (textSize <= 0) {
    SPDLOG_ERROR("textSize is {} which is not positive!", textSize);
    return std::string{};
  }

  const std::string filteredText(startOfText.base(), endOfText.base());
  SPDLOG_INFO("Filtered:{}", filteredText);
  return filteredText;
}

} // namespace mik
