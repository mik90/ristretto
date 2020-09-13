
if (EXISTS ${THIRD_PARTY_USR_DIR})
  # Prefer package in here, they're newer
  find_package(protobuf CONFIG REQUIRED
      PATHS ${THIRD_PARTY_USR_DIR}/lib/cmake/protobuf
  )

  find_package(gRPC CONFIG REQUIRED
      PATHS ${THIRD_PARTY_USR_DIR}/lib/cmake/grpc
  )
else()
  message(FATAL "Expected ${THIRD_PARTY_DIR} to exist!")
endif()
  
find_package(Threads REQUIRED)

add_subdirectory(AlsaInterface)

if (BUILD_KALDI_TCP_CLIENT)
    add_subdirectory(TcpClient)
endif()

add_library(protoObjects OBJECT
    ${ristretto_proto_srcs}
    ${ristretto_grpc_srcs}
)
target_link_libraries(protoObjects PUBLIC
    project_options
    protobuf::libprotobuf
    gRPC::grpc++_unsecure
    gRPC::gpr
)

add_library(RistrettoClientLib
  RistrettoClient.cpp
  Utils.cpp
)
set_target_properties(RistrettoClientLib
  PROPERTIES OUTPUT_NAME "RistrettoClient"
)

target_include_directories(RistrettoClientLib PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_BINARY_DIR}/src
  AlsaInterface
)

target_link_libraries(RistrettoClientLib PUBLIC
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
    CONAN_PKG::docopt.cpp

    # Locally built gRPC/protobuf
    #protobuf::libprotobuf
    #gRPC::grpc++_unsecure
    #gRPC::gpr
    
    # System-included uuid lib
    uuid
)

add_executable(RistrettoClient
  main.cpp
)

target_link_libraries(RistrettoClient
  RistrettoClientLib
)
