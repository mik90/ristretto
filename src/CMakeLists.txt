
# Reference: https://chromium.googlesource.com/external/github.com/grpc/grpc/+/chromium-deps/2016-08-17/examples/cpp/helloworld/CMakeLists.txt
# protobuf/3.9.1/testing appends the cmake files to the module but that's not on 
# the conan repo. Gonna just do a workaround
#find_package(protobuf CONFIG REQUIRED)
#message(STATUS "Using protobuf ${protobuf_VERSION}")
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${protobuf_VERSION}")

get_target_property(gRPC_CPP_PLUGIN_EXECUTABLE gRPC::grpc_cpp_plugin
    IMPORTED_LOCATION_RELEASE
)

get_filename_component(ristretto_proto "${CMAKE_SOURCE_DIR}/src/protos/ristretto.proto" ABSOLUTE)
get_filename_component(ristretto_proto_path "${ristretto_proto}" PATH)

protobuf_generate_cpp(ristretto_proto_srcs ristretto_proto_hdrs "${ristretto_proto}")
set(ristretto_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${ristretto_proto.grpc.pb.cc}")
set(ristretto_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${ristretto_proto.grpc.pb.h}")
add_custom_command(
    OUTPUT "${ristretto_proto_srcs}" "${ristretto_proto_hdrs}"
    COMMMAND protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" -I "${ristretto_proto}"
      --plugin=protoc-gen-grpc="${gRPC_CPP_PLUGIN_EXECUTABLE}" "${ristretto_proto}"
    DEPENDS "${ristretto_proto}"
)

if (BUILD_CLIENT)
    add_subdirectory(client)
endif()

if (BUILD_SERVER)
    add_subdirectory(server)
endif()
