project(client)

add_subdirectory(AlsaInterface)

add_library(protoObjects OBJECT
    ${ristretto_proto_srcs}
    ${ristretto_grpc_srcs}
)
target_link_libraries(protoObjects PRIVATE
    project_options
)

add_executable(RistrettoClient
  RistrettoClient.cpp
  Utils.cpp
  main.cpp
)

target_include_directories(RistrettoClient PRIVATE
  "${CMAKE_BINARY_DIR}/src"
  AlsaInterface
)

target_link_libraries(RistrettoClient PRIVATE
    # Generated files trigger warnings, compiled them separately
    protoObjects
    # Project settings
    project_options
    project_warnings
    # Internal libs
    AlsaInterface
    # Conan packages
    CONAN_PKG::docopt.cpp
    # System-included gRPC/protobuf
    protobuf
    grpc++_unsecure
    gpr
)