#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "AlsaInterface.hpp"
#include "DecoderClient.hpp"

// Note: Logic currently expects the test audio files to be in the current directory

TEST(AlsaTest, CaptureAudioInBuffer) {

  mik::AlsaConfig config;
  config.recordingDuration_us = mik::AlsaConfig::secondsToMicroseconds(2);
  mik::AlsaInterface alsa(config);

  const auto audio = alsa.captureAudioUntilUserExit();

  ASSERT_GT(audio.size(), 0);
}

TEST(AlsaTest, CaptureAudioToFile) {

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

TEST(AlsaTest, PlaybackAudioFromFile) {

  const std::string inputAudio = "unittestPlayback.raw";
  mik::AlsaConfig config;
  config.recordingDuration_us = mik::AlsaConfig::secondsToMicroseconds(1);
  mik::AlsaInterface alsa(config);

  std::fstream inputStream(inputAudio, inputStream.in);
  ASSERT_TRUE(inputStream.is_open());

  alsa.playbackAudio(inputStream);
}

// This requires the tcp decoding server to be up
TEST(ClientTest, TalkToDecodeServer) {

  mik::AlsaConfig config;
  config.samplingRate_bps = 16000; // 16,000 Hz, I guess
  mik::AlsaInterface alsa(config);

  const auto audioBuffer = alsa.captureAudioUntilUserExit();
  ASSERT_FALSE(audioBuffer.empty());

  mik::DecoderClient client;
  // Could also do localhost "127.0.0.1"
  client.connect("192.168.1.165", "5050");
  client.sendAudioToServer(audioBuffer);

  const auto result = client.getResultFromServer();
  ASSERT_FALSE(result.empty());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}