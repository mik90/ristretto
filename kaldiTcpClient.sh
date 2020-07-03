#!/bin/sh
set -e

run_build()
{
    echo "Running clientEntry.sh::run_build()"
    cd ./build
    cmake .. -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON -DBUILD_KALDI_TCP_CLIENT=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --target TcpClient --parallel $(nproc)
    cd -
}

start_client()
{
    echo "Running kaldiTcpClient.sh::start_client()"
    ./build/bin/TcpClient
}

main()
{
    if [ "$SKIP_BUILD" != "YES" ]; then
        run_build
    fi
    if [ "$SKIP_RUN" != "YES" ]; then
        start_client
    fi
}

main
