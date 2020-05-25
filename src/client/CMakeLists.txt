project(client)

add_subdirectory(AlsaInterface)

add_executable(RistrettoClient
  RistrettoClient.cpp
  main.cpp
  ${ristretto_proto_srcs}
  ${ristretto_grpc_srcs}
)

target_include_directories(RistrettoClient PRIVATE
  "${CMAKE_BINARY_DIR}/src"
  AlsaInterface
)

target_link_libraries(RistrettoClient PRIVATE
    project_options
    project_warnings
    # Internal libs
    AlsaInterface
    grpcObjects
    # Conan packages
    CONAN_PKG::docopt.cpp
    # gRPC
    gRPC::grpc++_unsecure
    # System-included protobuf
    protobuf
)