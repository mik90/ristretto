#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "AlsaInterface.hpp"
#include "DecoderInterface.hpp"

// Note: Logic currently expects the test audio files to be in the current directory

TEST(ClientTest, CaptureAudioInBuffer) {

  mik::AlsaConfig config;
  config.recordingDuration_us = mik::AlsaConfig::secondsToMicroseconds(2);
  mik::AlsaInterface alsa(config);

  const auto audio = alsa.captureAudioUntilUserExit();

  ASSERT_GT(audio.size(), 0);
}

TEST(ClientTest, CaptureAudioToFile) {

  const std::string outputAudio = "unittestCapture.raw";
  mik::AlsaConfig config;
  config.recordingDuration_us = mik::AlsaConfig::secondsToMicroseconds(2);
  mik::AlsaInterface alsa(config);

  std::fstream outputStream(outputAudio, outputStream.trunc | outputStream.out);
  ASSERT_TRUE(outputStream.is_open());

  alsa.captureAudio(outputStream);

  // Clean up
  std::filesystem::path testOutput{outputAudio};
  std::filesystem::remove(testOutput);
}

TEST(ClientTest, PlaybackAudioFromFile) {

  const std::string inputAudio = "unittestPlayback.raw";
  mik::AlsaConfig config;
  config.recordingDuration_us = mik::AlsaConfig::secondsToMicroseconds(1);
  mik::AlsaInterface alsa(config);

  std::fstream inputStream(inputAudio, inputStream.in);
  ASSERT_TRUE(inputStream.is_open());

  alsa.playbackAudio(inputStream);
}

// This requires the tcp decoding server to be up
TEST(ClientTest, ConnectToDecodeServer) {

  const std::string inputAudio = "unittestDecodeServer.raw";

  mik::DecoderInterface iface;
  iface.connect("127.0.0.1", "5050");

  // Note: this should not be const
  auto audioBuffer = mik::Utils::readInAudiofile(inputAudio);
  ASSERT_FALSE(audioBuffer.empty());

  iface.sendAudio(audioBuffer);

  const auto result = iface.getResult();
  ASSERT_FALSE(result.empty());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}