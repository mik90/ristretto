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
    : stub_(ristretto::Greeter::NewStub(channel)) {
  SPDLOG_INFO("Constructed RistrettoClient");
  fmt::print("Constructed RistrettoClient\n");
}

std::string RistrettoClient::sendHello(const std::string& user) {

  SPDLOG_INFO("sendHello start");
  ristretto::HelloRequest request;
  request.set_name(user);

  ristretto::HelloReply reply;
  grpc::ClientContext context;
  grpc::CompletionQueue cq;
  grpc::Status status;

  std::unique_ptr<grpc::ClientAsyncResponseReader<ristretto::HelloReply>> rpc(
      stub_->AsyncSayHello(&context, request, &cq));

  rpc->Finish(&reply, &status, reinterpret_cast<void*>(1));
  void* got_tag;
  bool ok = false;

  GPR_ASSERT(cq.Next(&got_tag, &ok));
  GPR_ASSERT(got_tag == reinterpret_cast<void*>(1));
  GPR_ASSERT(ok);

  SPDLOG_INFO("sendHello end");
  if (status.ok()) {
    return reply.message();
  } else {
    return "RPC failed.";
  }
}

// Strip all the newlines until we get to text
// use all that text until we get to the newline
std::string RistrettoClient::filterResult(const std::string& fullResult) {
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
