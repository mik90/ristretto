#!/bin/sh
set -e

run_build()
{
    echo "Running buildClient.sh::run_build()"
    echo "PWD:$PWD"
    ./runProtoc.sh
    cd ./build
    echo "PWD:$PWD"
    cmake .. -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --parallel $(nproc)
}

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

# Main
run_build
run_tests