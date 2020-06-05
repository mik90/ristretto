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

run_build