#!/bin/sh
# I can't get the cmake custom command to work so this is just a workaround
cd build
mkdir -p src

DEV_PROTOC_BIN=/home/mike/Development/usr/bin/protoc
DEV_GRPC_CPP_PLUGIN_BIN=/home/mike/Development/usr/bin/grpc_cpp_plugin

if [ -f $DEV_PROTOC_BIN ]; then
  echo "Using $DEV_PROTOC_BIN"
else
  echo "Defaulting to first \"protoc\" in path"
  DEV_PROTOC_BIN=$(which protoc)
fi


if [ -f $DEV_GRPC_CPP_PLUGIN_BIN ]; then
  echo "Using $DEV_GRPC_CPP_PLUGIN_BIN"
else
  echo "Defaulting to first \"grpc_cpp_plugin\" in path"
  # Absolute path is actually required for this
  DEV_GRPC_CPP_PLUGIN_BIN=$(which grpc_cpp_plugin)
fi

CMD="$DEV_PROTOC_BIN --grpc_out ./src --cpp_out ./src -I ../src/protos \
      --plugin=protoc-gen-grpc=$DEV_GRPC_CPP_PLUGIN_BIN ../src/protos/ristretto.proto"
echo $CMD
eval $CMD
