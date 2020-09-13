#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Utils.hpp"

TEST(UuidTest, testTokenGen) {
  const auto token = mik::Utils::generateSessionToken();

  ASSERT_FALSE(token.empty());
}
