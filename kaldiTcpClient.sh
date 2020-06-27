#!/bin/bash
set -e

run_build()
{
    echo "Running clientEntry.sh::run_build()"
    cd /opt/ristretto/build
    cmake .. -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON -DBUILD_KALDI_TCP_CLIENT=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
    cmake --build . --target TcpClient --parallel $(nproc)
    cd -
}

start_client()
{
    echo "Running kaldiTcpClient.sh::start_client()"
    cd /opt/ristretto
    ./build/bin/TcpClient
    cd -
}

main()
{
    cd /opt/ristretto
    if [ "$SKIP_RUN" != "YES" ]; then
        start_client
    fi
}

main