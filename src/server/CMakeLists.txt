find_package(kaldi CONFIG REQUIRED
    PATHS ${CMAKE_SOURCE_DIR}/../kaldi/dist/lib/cmake/kaldi
)

add_library(protoObjects OBJECT
    ${ristretto_proto_srcs}
    ${ristretto_grpc_srcs}
)
target_link_libraries(protoObjects PRIVATE
    project_options
)

add_library(RistrettoServerLib
    Utils.cpp
    KaldiInterface.cpp
    RistrettoServer.cpp
)

set_target_properties(RistrettoServerLib
    PROPERTIES OUTPUT_NAME "RistrettoServer"
)

# Turn spdlog onto the DEBUG level
target_compile_definitions(RistrettoServerLib PUBLIC SPDLOG_ACTIVE_LEVEL=1)

# These keep disappearing in my cache unless i add them in here, unsure why??
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Og -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")

target_include_directories(RistrettoServerLib PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_BINARY_DIR}/src
)

target_link_libraries(RistrettoServerLib PUBLIC
    # Generated files throw warnings so compile them separately
    protoObjects
    # Project settings
    project_options
    project_warnings

    # system gRPC/Protobuf
    protobuf
    grpc++_unsecure
    gpr

    # Libs from conan
    CONAN_PKG::fmt
    CONAN_PKG::spdlog

    # Kaldi and OpenFST
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
    RistrettoServerLib
)
