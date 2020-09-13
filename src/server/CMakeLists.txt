find_package(kaldi CONFIG REQUIRED
    PATHS /usr/lib/cmake/kaldi
)

find_package(protobuf CONFIG REQUIRED
    PATHS /usr/lib/cmake/protobuf
)

find_package(gRPC CONFIG REQUIRED
    PATHS /usr/lib/cmake/grpc
)


add_library(protoObjects OBJECT
    ${ristretto_proto_srcs}
    ${ristretto_grpc_srcs}
)
target_link_libraries(protoObjects PRIVATE
    project_options
)

<<<<<<< HEAD
add_library(RistrettoServerLib
=======
add_library(RistrettoServerCore
>>>>>>> master
    Utils.cpp
    KaldiInterface.cpp
    RistrettoServer.cpp
)

set_target_properties(RistrettoServerLib
    PROPERTIES OUTPUT_NAME "RistrettoServer"
)

# Turn spdlog onto the DEBUG level
<<<<<<< HEAD
target_compile_definitions(RistrettoServerLib PUBLIC SPDLOG_ACTIVE_LEVEL=1)

# These keep disappearing in my cache unless i add them in here, unsure why??
=======
target_compile_definitions(RistrettoServerCore PUBLIC SPDLOG_ACTIVE_LEVEL=1)

# These keep disappearing in my cache, unsure why??
>>>>>>> master
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Og -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

<<<<<<< HEAD
target_include_directories(RistrettoServerLib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_BINARY_DIR}/src
)

target_link_libraries(RistrettoServerLib PUBLIC
=======
target_include_directories(RistrettoServerCore PUBLIC
    "${CMAKE_BINARY_DIR}/src"
    "."
)

target_link_libraries(RistrettoServerCore PUBLIC
>>>>>>> master
    # Generated files throw warnings so compile them separately
    protoObjects
    # Project settings
    project_options
    project_warnings

    # system gRPC/Protobuf
    protobuf::libprotobuf
    gRPC::grpc++_unsecure
    gRPC::gpr
    gRPC::absl_base

    # Libs from conan
    CONAN_PKG::fmt
    CONAN_PKG::spdlog
    CONAN_PKG::nlohmann_json

    # Kaldi and OpenFST
    # These targets are not exposed under a namespace
    fst
    kaldi-base
    kaldi-matrix
    kaldi-cudamatrix
    kaldi-util
    kaldi-feat
    kaldi-tree
    kaldi-gmm
    kaldi-transform
    kaldi-sgmm2
    kaldi-fstext
    kaldi-hmm
    kaldi-lm
    kaldi-decoder
    kaldi-lat
    kaldi-nnet
    kaldi-nnet2
    kaldi-nnet3
    kaldi-rnnlm
    kaldi-chain
    kaldi-ivector
    kaldi-online
    kaldi-online2
    kaldi-kws
    kaldi-cudafeat
    kaldi-cudadecoder
)

add_executable(RistrettoServer 
    main.cpp
)

target_link_libraries(RistrettoServer PRIVATE
<<<<<<< HEAD
    RistrettoServerLib
=======
    RistrettoServerCore
>>>>>>> master
)
