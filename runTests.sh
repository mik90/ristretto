#!/bin/bash

TEST_BINARY="$PWD/build/bin/ClientTest"
GTEST_ARGS=" --gtest_filter=-*USER_INPUT* "
test -f $TEST_BINARY || { echo "$TEST_BINARY does not exist, have you built it yet?"; exit 1; }
CMD="$TEST_BINARY $GTEST_ARGS"

eval $CMD