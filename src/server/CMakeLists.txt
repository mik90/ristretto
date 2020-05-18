project(server)

find_package(kaldi CONFIG REQUIRED
    PATHS ${CMAKE_SOURCE_DIR}/../kaldi/dist/lib/cmake/kaldi
)

add_executable(RistrettoServer RistrettoServer.cpp)

target_link_libraries(RistrettoServer PRIVATE
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
