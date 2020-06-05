#!/bin/sh
set -e

./runProtoc.sh
cd ./build
cmake .. -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja
cmake --build . --parallel $(nproc)
