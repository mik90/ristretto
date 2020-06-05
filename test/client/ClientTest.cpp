#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "AlsaInterface.hpp"
#include "RistrettoClient.hpp"
#include "Utils.hpp"

TEST(ClientTest, FilterResult) {
  const auto testString("this \n"
                        "this is a test \n"
                        "this is a test one \n"
                        "this is a test one two \n"
                        "this is a test one two three \n"
                        "this is a test one two three \n"
                        "\n"
                        "\n");
  const auto result = mik::RistrettoClient::filterResult(testString);
  ASSERT_EQ(result, "this is a test one two three");
}

// This requires the tcp decoding server to be up
TEST(ClientTest, REQUIRES_SERVER_SendAudioFileToServer) {
  const auto audioBuffer = mik::Utils::readInAudioFile("ClientTestAudio8KHz.raw");
  ASSERT_FALSE(audioBuffer.empty());

  mik::RistrettoClient client;
  // client.connect("192.168.1.165", "5050");
  try {
    client.connect("127.0.0.1", "5050");
  } catch (const boost::system::system_error& e) {
    FAIL() << "Exception:" << e.what();
  } catch (const std::exception& e) {
    FAIL() << "Exception:" << e.what();
  }
  client.sendAudioToServer(audioBuffer);

  const auto result = client.getResultFromServer();
  ASSERT_FALSE(result.empty());
  ASSERT_NE(result.find("this is a test one two three"), std::string::npos);
}

// This requires the tcp decoding server to be up and also requires someone to press "enter"
TEST(ClientTest, REQUIRES_SERVER_USER_INPUT_CaptureUserInputAndSendToServer) {

  mik::AlsaConfig config;
  config.samplingFreq_Hz = 8000;
  mik::AlsaInterface alsa(config);

  const auto audioBuffer = alsa.captureAudioUntilUserExit();
  ASSERT_FALSE(audioBuffer.empty());

  mik::RistrettoClient client;
  // Could also do localhost "127.0.0.1"
  // client.connect("192.168.1.165", "5050");
  try {
    client.connect("127.0.0.1", "5050");
  } catch (const boost::system::system_error& e) {
    FAIL() << "Exception:" << e.what();
  } catch (const std::exception& e) {
    FAIL() << "Exception:" << e.what();
  }
  client.sendAudioToServer(audioBuffer);

  const auto result = client.getResultFromServer();
  ASSERT_FALSE(result.empty());
}
