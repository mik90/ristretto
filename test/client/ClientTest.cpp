#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "AlsaInterface.hpp"
#include "DecoderClient.hpp"
#include "Utils.hpp"

// Note: Logic currently expects the test audio files to be in the current directory

TEST(UtilTest, ReadInAudioFile) {
  const auto audioBuffer = mik::Utils::readInAudioFile("ClientTestAudio8KHz.raw");
  ASSERT_EQ(audioBuffer.size(), 160000);
}

TEST(ClientTest, FilterResult) {
  const auto testString("this \n"
                        "this is a test \n"
                        "this is a test one \n"
                        "this is a test one two \n"
                        "this is a test one two three \n"
                        "this is a test one two three \n"
                        "\n"
                        "\n");
  const auto result = mik::DecoderClient::filterResult(testString);
  ASSERT_EQ(result, "this is a test one two three");
}

// This requires the tcp decoding server to be up
TEST(ClientTest, REQUIRES_SERVER_SendAudioFileToServer) {
  const auto audioBuffer = mik::Utils::readInAudioFile("ClientTestAudio8KHz.raw");
  ASSERT_FALSE(audioBuffer.empty());

  mik::DecoderClient client;
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

TEST(AlsaTest, USER_INPUT_CaptureAudioInBuffer) {

  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  const auto audio = alsa.captureAudioUntilUserExit();

  ASSERT_GT(audio.size(), 0);
}

TEST(AlsaTest, USER_INPUT_CaptureAudioToFile) {

  const std::string outputAudio = "unittestCapture.raw";
  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  std::fstream outputStream(outputAudio, outputStream.trunc | outputStream.out);
  ASSERT_TRUE(outputStream.is_open());

  constexpr unsigned int secondsToRecord = 5;
  alsa.captureAudioFixedSize(outputStream, secondsToRecord);

  // Clean up
  std::filesystem::path testOutput{outputAudio};
  std::filesystem::remove(testOutput);
}

TEST(AlsaTest, USER_INPUT_PlaybackAudioFromFile) {

  const std::string inputAudio = "unittestPlayback.raw";
  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  std::fstream inputStream(inputAudio, inputStream.in);
  ASSERT_TRUE(inputStream.is_open());

  constexpr unsigned int secondsToRecord = 5;
  alsa.playbackAudioFixedSize(inputStream, secondsToRecord);
}

// This requires the tcp decoding server to be up and also requires someone to press "enter"
TEST(ClientTest, REQUIRES_SERVER_USER_INPUT_CaptureUserInputAndSendToServer) {

  mik::AlsaConfig config;
  config.samplingFreq_Hz = 8000;
  mik::AlsaInterface alsa(config);

  const auto audioBuffer = alsa.captureAudioUntilUserExit();
  ASSERT_FALSE(audioBuffer.empty());

  mik::DecoderClient client;
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

class MockAlsaInterface : public mik::AlsaInterface {
public:
  MockAlsaInterface(const mik::AlsaConfig& config) : AlsaInterface(config){};
  void setAudioData(const std::vector<char>& v) { audioData_ = v; }
};

TEST(AlsaTest, consumeAllAudio) {
  // Default init 100 chars
  const std::vector<char> internalAudioData(100);
  mik::AlsaConfig config;
  MockAlsaInterface alsa(config);
  alsa.setAudioData(internalAudioData);

  const auto audioData = alsa.consumeAllAudioData();
  ASSERT_EQ(internalAudioData.size(), audioData.size());
}

TEST(AlsaTest, consumeDurationOfAudio_10ms) {
  const std::vector<char> internalAudioData(425000);
  mik::AlsaConfig config;
  MockAlsaInterface alsa(config);
  alsa.setAudioData(internalAudioData);

  // 10 milliseconds should be 320,000 bytes
  // This is found with the same calculation that i used to make it, so not the best test
  const auto audioData = alsa.consumeDurationOfAudioData(10);
  ASSERT_EQ(320000, audioData.size());
}

TEST(AlsaTest, consumeDurationOfAudio_LargeRequest) {
  const std::vector<char> internalAudioData(5000);
  mik::AlsaConfig config;
  MockAlsaInterface alsa(config);
  alsa.setAudioData(internalAudioData);

  // 1000 milliseconds is a lot, so it should just return all the data that is available
  const auto audioData = alsa.consumeDurationOfAudioData(1000);
  ASSERT_EQ(internalAudioData.size(), audioData.size());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  mik::Utils::createLogger();
  return RUN_ALL_TESTS();
}