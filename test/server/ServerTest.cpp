#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>

#include "KaldiInterface.hpp"
#include "TestUtils.hpp"
#include "Utils.hpp"

// @test 6 chars should be condensed into 3 int16 elements
TEST(DeserializeTest, SizeDecreased) {
  const std::string input = "123456"; // 6 chars
  ASSERT_EQ(input.length(), 6);

  // Pass in a unique copy of the input
  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  ASSERT_EQ(output.Dim(), 3);
}

// @test 5 chars should become 3 int16s, the last char will just be the bottom
// half of an int16
TEST(DeserializeTest, UnevenInputLength) {
  const std::string input = "12345"; // 5 chars
  ASSERT_EQ(input.length(), 5);

  // Pass in a unique copy of the input
  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  ASSERT_EQ(output.Dim(), 3);
}

// @test Check that integer values are preserved as expected with uppers of 0x00
TEST(DeserializeTest, ValuePreserved) {
  // Since the 'lower' chars will be 0, this input should have the
  // same integer value as the output per-element
  const std::string input = {0x01, 0x00, 0x02, 0x00};

  // Pass in a unique copy of the input
  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  // There should be half as many elements in the output
  ASSERT_EQ(input.length() / 2, output.Dim());

  // Promote both types to int32
  EXPECT_EQ(static_cast<int32_t>(input[0]) /* 0x01 */, static_cast<int32_t>(output(0)));
  EXPECT_EQ(static_cast<int32_t>(input[2]) /* 0x02 */, static_cast<int32_t>(output(1)));
}

// @test Check that values are changed when uppers are not 0x00
TEST(DeserializeTest, ValueChanged) {
  // Since the 'lower' chars will NOT be 0, they will form a different integaer
  // value when combined as an int16
  const std::string input = {0x00, 0x01, 0x00, 0x02};

  // Pass in a unique copy of the input
  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  // There should be half as many elements in the output
  ASSERT_EQ(input.length() / 2, output.Dim());

  // Promote both types to int32
  EXPECT_NE(static_cast<int32_t>(input[0]) /* 0x00 */, static_cast<int32_t>(output(0)));
  EXPECT_NE(static_cast<int32_t>(input[2]) /* 0x00 */, static_cast<int32_t>(output(1)));
}

// @test Convert 20 values into 10. Values are taken from audio data.
TEST(DeserializeTest, ConvertValidValues) {
  // These are int values
  const std::string input = {int8_t(0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, -1, -1, 0, 0, 1, 0};
  // Just a check to ensure that this is 20 values
  ASSERT_EQ(input.length(), 20);

  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  const auto expectedOutput = TestUtils::initializerListToKaldiVector(
      std::initializer_list<kaldi::BaseFloat>{0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0});

  // Just a check to ensure that this is 10 values
  ASSERT_EQ(expectedOutput.Dim(), 10);

  ASSERT_TRUE(output.ApproxEqual(expectedOutput));
}

// @test Convert 21 values into 11. Values are taken from audio data. If there's an odd number of
//       input values, it'll be appended to the end
TEST(DeserializeTest, ConvertValidValues_UnevenAmount) {
  // These are int values
  const std::string input = {int8_t(0), 0, 0, 0,  0,  0, 0, 0, 0, 0, 1,
                             0,         0, 0, -1, -1, 0, 0, 1, 0, 35};
  ASSERT_EQ(input.length(), 21);

  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  const auto expectedOutput =
      TestUtils::initializerListToKaldiVector(std::initializer_list<kaldi::BaseFloat>{
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 35.0});

  ASSERT_EQ(expectedOutput.Dim(), 11);

  ASSERT_TRUE(output.ApproxEqual(expectedOutput));
}

// @test Convert -54 lower and 0 upper to 202
TEST(DeserializeTest, ConvertValidValuesSigned) {
  // These are int values
  const std::string input = {int8_t(-54), 0};
  const auto inputLength = input.length();

  const auto output = mik::stringToKaldiVector(std::make_unique<std::string>(input));

  const auto expectedOutput =
      TestUtils::initializerListToKaldiVector(std::initializer_list<kaldi::BaseFloat>{202});

  // Output length should be half of input length
  ASSERT_EQ(inputLength / 2, static_cast<size_t>(expectedOutput.Dim()));

  for (auto i = 0; i < expectedOutput.Dim(); ++i) {
    ASSERT_EQ(expectedOutput(i), output(i));
  }
}

TEST(JsonTest, ConvertArgs) {
  std::filesystem::path serverConfig("./test/resources/testServerConfig.json");

  mik::KaldiCommandLineArgs cmdLineArgs(serverConfig);

  ASSERT_EQ(cmdLineArgs.getArgCount(), 14);

  // Should be the same as the arg count
  const auto argvAsStrings = cmdLineArgs.getUnderlyingStrings();
  ASSERT_EQ(argvAsStrings.size(), 14);

  char* const* curArgV = cmdLineArgs.getArgValues();
  auto it = argvAsStrings.cbegin();

  for (int i = 0; i < 13; ++i) {
    ASSERT_NE(curArgV, nullptr) << "curArgV[" << i << "] is null!";
    ASSERT_NE(*curArgV, nullptr) << "*curArgV[" << i << "] is null!";
    ASSERT_EQ(std::string(*curArgV), *it);
    curArgV++;
    it++;
  }
}
