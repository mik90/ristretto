get_filename_component(ristretto_proto "${CMAKE_CURRENT_LIST_DIR}/protos/ristretto.proto" ABSOLUTE)
get_filename_component(ristretto_proto_path "${ristretto_proto}" PATH)

set(ristretto_proto_srcs "${CMAKE_BINARY_DIR}/src/ristretto.pb.cc")
set(ristretto_proto_hdrs "${CMAKE_BINARY_DIR}/src/ristretto.pb.h")
set(ristretto_grpc_srcs "${CMAKE_BINARY_DIR}/src/ristretto.grpc.pb.cc")
set(ristretto_grpc_hdrs "${CMAKE_BINARY_DIR}/src/ristretto.grpc.pb.h")

find_program(grpc_cpp_plugin_exec
    grpc_cpp_plugin
    PATHS /usr/bin /usr/local/bin
)

# I tried to use add_custom_command but kept having issues, so this works
execute_process(
    COMMAND protoc --grpc_out "${CMAKE_BINARY_DIR}/src" --cpp_out "${CMAKE_BINARY_DIR}/src" 
            -I "${ristretto_proto_path}" --plugin=protoc-gen-grpc=${grpc_cpp_plugin_exec} "${ristretto_proto}"
)

if (BUILD_CLIENT)
    add_subdirectory(client)
endif()

if (BUILD_SERVER)
    add_subdirectory(server)
endif()
