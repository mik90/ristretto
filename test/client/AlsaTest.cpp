#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "AlsaInterface.hpp"
#include "Utils.hpp"

// Note: Logic currently expects the test audio files to be in the current directory

TEST(UtilTest, ReadInAudioFile) {
  const auto audioBuffer = mik::Utils::readInAudioFile("test/resources/ClientTestAudio8KHz.raw");
  ASSERT_EQ(audioBuffer.size(), 160000);
}

TEST(AlsaTest, CaptureAudio_Stream) {

  std::filesystem::path outputPath = "test/resources/unittestCapture.raw";
  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  std::fstream outputStream(outputPath.generic_string(), outputStream.trunc | outputStream.out);
  ASSERT_TRUE(outputStream.is_open());

  constexpr unsigned int millisecondsToRecord = 10;
  alsa.captureAudioFixedSizeMs(outputStream, millisecondsToRecord);
  outputStream.flush();

  EXPECT_GT(std::filesystem::file_size(outputPath), 0);

  // Clean up
  std::filesystem::remove(outputPath);
}

TEST(AlsaTest, CaptureAudio_Vector) {

  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  constexpr unsigned int msToRecord = 100;
  const auto audioData = alsa.captureAudioFixedSizeMs(msToRecord);

  ASSERT_GT(audioData.size(), 0);
}

TEST(AlsaTest, PlaybackAudioFromFile) {

  const std::string inputAudio = "test/resources/unittestPlayback.raw";
  mik::AlsaConfig config;
  mik::AlsaInterface alsa(config);

  std::fstream inputStream(inputAudio, inputStream.in);
  constexpr unsigned int secondsToRecord = 1;

  const auto start = inputStream.gcount();
  ASSERT_TRUE(inputStream.is_open());

  alsa.playbackAudioFixedSize(inputStream, secondsToRecord);

  const auto end = inputStream.gcount();
  ASSERT_LT(start, end) << "inputStream.gcount() should have risen";
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
  // Create large vector of default-initialized data
  const std::vector<char> internalAudioData(2048);
  mik::AlsaConfig config;
  MockAlsaInterface alsa(config);
  alsa.setAudioData(internalAudioData);

  // 10 is 2 periods (each 128 bytes) totalling 256 bytes
  const auto audioData = alsa.consumeDurationOfAudioData(10);
  ASSERT_EQ(256, audioData.size());
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
