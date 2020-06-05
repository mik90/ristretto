#!/bin/sh

set -e

run_tests()
{
    echo "Running buildClient.sh::run_tests()"
    echo "PWD:$PWD"
    # Could use ctest binary, but the executable itself is more verbose on CLI
    #ctest . -E "USER_INPUT|REQUIRES_SERVER"

    cd ./bin
    TEST_BINARY="./ClientTest"
    GTEST_ARGS=" --gtest_filter=-*USER_INPUT*:*REQUIRES_SERVER* "
    test -f $TEST_BINARY || { echo "$TEST_BINARY does not exist, have you built it yet?"; exit 1; }
    CMD="$TEST_BINARY $GTEST_ARGS"
    eval $CMD
}

run_tests