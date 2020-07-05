#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>

#include "KaldiInterface.hpp"
#include "TestUtils.hpp"

// @test 6 chars should be condensed into 3 int16 elements
TEST(DeserializeTest, SizeDecreased) {
  const std::string input = "123456"; // 6 chars
  ASSERT_EQ(input.length(), 6);

  // Pass in a unique copy of the input
  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

  ASSERT_EQ(output.Dim(), 3);
}

// @test 5 chars should become 3 int16s, the last char will just be the bottom
// half of an int16
TEST(DeserializeTest, UnevenInputLength) {
  const std::string input = "12345"; // 5 chars
  ASSERT_EQ(input.length(), 5);

  // Pass in a unique copy of the input
  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

  ASSERT_EQ(output.Dim(), 3);
}

// @test Check that integer values are preserved as expected with uppers of 0x00
TEST(DeserializeTest, ValuePreserved) {
  // Since the 'lower' chars will be 0, this input should have the
  // same integer value as the output per-element
  const std::string input = {0x01, 0x00, 0x02, 0x00};

  // Pass in a unique copy of the input
  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

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
  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

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

  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

  const auto expected_output = TestUtils::initializerListToKaldiVector(
      std::initializer_list<kaldi::BaseFloat>{0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0});

  // Just a check to ensure that this is 10 values
  ASSERT_EQ(expected_output.Dim(), 10);

  ASSERT_TRUE(output.ApproxEqual(expected_output));
}

// @test Convert 21 values into 11. Values are taken from audio data. If there's an odd number of
//       input values, it'll be appended to the end
TEST(DeserializeTest, ConvertValidValues_UnevenAmount) {
  // These are int values
  const std::string input = {int8_t(0), 0, 0, 0,  0,  0, 0, 0, 0, 0, 1,
                             0,         0, 0, -1, -1, 0, 0, 1, 0, 35};
  ASSERT_EQ(input.length(), 21);

  const auto output = kaldi::deserializeAudioData(std::make_unique<std::string>(input));

  const auto expected_output =
      TestUtils::initializerListToKaldiVector(std::initializer_list<kaldi::BaseFloat>{
          0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 35.0});

  ASSERT_EQ(expected_output.Dim(), 11);

  ASSERT_TRUE(output.ApproxEqual(expected_output));
}
