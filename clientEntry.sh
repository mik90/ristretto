#!/bin/sh
set -e

run_protoc()
{
    echo "Running clientEntry.sh::run_protoc()"
<<<<<<< HEAD
=======
    cd /opt/ristretto
>>>>>>> master
    ./runProtoc.sh
}

run_build()
{
    echo "Running clientEntry.sh::run_build()"
<<<<<<< HEAD
    cd build
    cmake .. -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --target RistrettoClient ClientTest --parallel $(nproc)
=======
    cd /opt/ristretto/build
    cmake .. -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --target RistrettoClient --parallel $(nproc)
>>>>>>> master
    cd -
}

run_tests()
{
    echo "Running clientEntry.sh::run_tests()"
    # Could use ctest binary, but the executable itself is more verbose on CLI
    #ctest . -E "USER_INPUT|REQUIRES_SERVER"

    # Test resources are copied into bin
<<<<<<< HEAD
=======
    cd /opt/ristretto
>>>>>>> master
    TEST_BINARY="./build/bin/ClientTest"
    GTEST_ARGS=" --gtest_filter=-*USER_INPUT*:*REQUIRES_SERVER* "
    test -f $TEST_BINARY || { echo "$TEST_BINARY does not exist, have you built it yet?"; exit 1; }
    CMD="$TEST_BINARY $GTEST_ARGS"
    eval $CMD
<<<<<<< HEAD
=======
    cd -
>>>>>>> master
}

start_client()
{
    echo "Running clientEntry.sh::start_client()"
    ./build/bin/RistrettoClient --file ./test/resources/ClientTestAudio8KHz.raw
}

main()
{
<<<<<<< HEAD
    if [ "$(basename $PWD)" != "ristretto" ]; then
        echo "Expected to be in the ristretto dir, exiting..."
        exit 1
    fi

=======
    cd /opt/ristretto
    if [ "$SKIP_PROTOC" != "YES" ]; then
        run_protoc
    fi
>>>>>>> master
    if [ "$SKIP_BUILD" != "YES" ]; then
        run_protoc
        run_build
        run_tests
    fi

    if [ "$SKIP_RUN" != "YES" ]; then
        start_client
    fi
}

main