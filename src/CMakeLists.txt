get_filename_component(ristretto_proto "${CMAKE_CURRENT_LIST_DIR}/protos/ristretto.proto" ABSOLUTE)
get_filename_component(ristretto_proto_path "${ristretto_proto}" PATH)

set(ristretto_proto_srcs "${CMAKE_BINARY_DIR}/src/ristretto.pb.cc")
set(ristretto_proto_hdrs "${CMAKE_BINARY_DIR}/src/ristretto.pb.h")
set(ristretto_grpc_srcs "${CMAKE_BINARY_DIR}/src/ristretto.grpc.pb.cc")
set(ristretto_grpc_hdrs "${CMAKE_BINARY_DIR}/src/ristretto.grpc.pb.h")

# TODO get this command working
add_custom_command(
    OUTPUT "${ristretto_grpc_srcs}" "${ristretto_grpc_hdrs}"
    COMMMAND protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}" --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
       -I "${ristretto_proto_path}" --plugin=protoc-gen-grpc="/usr/bin/grpc_cpp_plugin"
       "${ristretto_proto}"
    DEPENDS "${ristretto_proto}"
)

if (BUILD_CLIENT)
    add_subdirectory(client)
endif()

if (BUILD_SERVER)
    add_subdirectory(server)
endif()
