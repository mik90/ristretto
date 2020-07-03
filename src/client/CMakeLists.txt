add_subdirectory(AlsaInterface)

if (BUILD_KALDI_TCP_CLIENT)
    add_subdirectory(TcpClient)
endif()

add_library(protoObjects OBJECT
    ${ristretto_proto_srcs}
    ${ristretto_grpc_srcs}
)
target_link_libraries(protoObjects PRIVATE
    project_options
)

add_library(RistrettoClientCore
  RistrettoClient.cpp
  Utils.cpp
)

target_include_directories(RistrettoClientCore PUBLIC
  "${CMAKE_BINARY_DIR}/src"
  AlsaInterface
)

target_link_libraries(RistrettoClientCore PUBLIC
    # Generated files trigger warnings, compiled them separately
    protoObjects
    # Project settings
    project_options
    project_warnings
    # Internal libs
    AlsaInterface
    # Conan packages
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    # System-included gRPC/protobuf
    protobuf
    grpc++_unsecure
    gpr
)

add_executable(RistrettoClient
  main.cpp
)

target_link_libraries(RistrettoClient
  RistrettoClientCore
)
