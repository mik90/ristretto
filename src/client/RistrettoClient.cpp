#include <algorithm>
#include <fstream>
#include <iostream>

#include <fmt/core.h>
#include <fmt/locale.h>
#include <spdlog/spdlog.h>

#include "RistrettoClient.hpp"
#include "Utils.hpp"

namespace mik {

RistrettoClient::RistrettoClient() : stub_(ristretto::NewStub(channel)) {};
  SPDLOG_INFO("--------------------------------------------");
  SPDLOG_INFO("RistrettoClient created.");
  SPDLOG_INFO("--------------------------------------------");
}

void RistrettoClient::sendHello(const std::string& user) {
  Greeter::HelloRequest request;
  request.set_name(user);

  AsyncClientCall* call = new AsyncClientCall;

  call->responseReader = stub_->sayHello(&call->context, request, &compQueue_);
  
  call->responseReader->Finish(&call->reply, &call->status, static_cast<void*>(call));
}

void RistrettoClient::asyncCompleteRpc() {
  void* got_tag;
  bool ok = false;

  while (compQueue_.Next(&got_tag, &ok)) {
    AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);
    GPR_ASSERT(ok);
    if (call->status.ok()) {
      SPDLOG_INFO("Greeter received: {}", call->reply.message());
    }
    else {
      SPDLOG_ERROR("RPC failed.");
    }
    delete call;
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
