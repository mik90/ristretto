#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Utils.hpp"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  mik::Utils::createLogger();
  return RUN_ALL_TESTS();
}