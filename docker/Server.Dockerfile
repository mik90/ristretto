FROM nvidia/cuda:10.2-cudnn7-devel-ubuntu18.04 as kaldi-ubuntu18.04

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  g++ \
  make \
  automake \
  autoconf \
  bzip2 \
  unzip \
  wget \
  sox \
  libtool \
  git \
  subversion \
  python2.7 \
  python3 \
  zlib1g-dev \
  gfortran \
  ca-certificates \
  patch \
  ffmpeg \
  libopenblas-dev \
  software-properties-common \
  && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
  | gpg --dearmor - \
  | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null  \
  && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
  && apt update \
  && apt install -y --no-install-recommends \ 
  gcc-9 g++-9 cmake ccache ninja-build curl clang-format-9 \
  python3 python3-distutils \
  build-essential \
  pkg-config  \
  && ln -sv /usr/bin/clang-format-9 /usr/bin/clang-format \
  && ln -s /usr/bin/python2.7 /usr/bin/python \
  && rm -rf /var/lib/apt/lists/*

# Build kaldi with cmake
# - When using vscode, can add a kaldi/.vscode/settings.json that updates the cmake.configureSettings
# - Kaldi will not build with GCC-9 requires use with 5.4
#   - Are GCC 9 and 5.4 even ABI compatible?
# - Kaldi defaults to OpenBLAS in CMake, so that requires an additiona package
# Build Kaldi with the newer cmake
# Install to /opt/kaldi/dist
# TODO Append /opt/kaldi/dist/bin to PATH so that the kaldi scripts can find it
#      I should be able to avoid building with the makefiles once that's done
RUN git clone --depth 1 https://github.com/kaldi-asr/kaldi.git /opt/kaldi && \
  cd /opt/kaldi && \
  cd /opt/kaldi/tools && \
  ./extras/install_mkl.sh && \
  make -j $(nproc) && \
  cd /opt/kaldi/src && \
  ./configure --shared --use-cuda && \
  make depend -j $(nproc) && \
  make -j $(nproc) && \
  mkdir /opt/kaldi/build && \
  cd /opt/kaldi/build && \
  cmake .. -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs \
  -DCMAKE_INSTALL_PREFIX=../dist \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  && cmake --build . --parallel $(nproc) --target install

# Clean up the cache
RUN rm -rf /var/lib/apt/lists/*

# -------------------------------------------------------
FROM kaldi-ubuntu18.04 as buildenv
WORKDIR /opt/ristretto

# Set up the ristretto build environment
ENV force_color_prompt=yes

# After building kaldi, set gcc-9 as the default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60  \
  --slave /usr/bin/g++ g++ /usr/bin/g++-9

# Install conan
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py \
  && python3 get-pip.py && rm get-pip.py \
  && pip3 install conan

# Clean up the cache
RUN rm -rf /var/lib/apt/lists/*

# -------------------------------------------------------
FROM buildenv as grpc-build 

# Reference: https://github.com/npclaudiu/grpc-cpp-docker/blob/master/Dockerfile
ENV GRPC_RELEASE_TAG v1.29.1

RUN git clone --depth 1 -b ${GRPC_RELEASE_TAG} https://github.com/grpc/grpc /opt/grpc \
  && cd /opt/grpc \
  && git submodule update --init --recursive --depth 1

# TODO Just build grpc and protobuf for C++
RUN cd /opt/grpc/third_party/protobuf \
  && ./autogen.sh && ./configure --enable-shared \
  && make -j $(nproc) && make install && make clean \
  && ldconfig

RUN cd /opt/grpc && make -j $(nproc) && make install && make clean \
  && ldconfig

# -------------------------------------------------------
FROM grpc-build as model-setup
# Set up the model

# Need to unzip the trained aspire model with HCLG into the egs/aspire/s5/exp dir
# Don't want to store this in the repo but i also don't want to download it again since it's 793 MB
# Could do something like
# wget http://kaldi-asr.org/models/1/0001_aspire_chain_model_with_hclg.tar.bz2
COPY 0001_aspire_chain_model_with_hclg.tar.bz2 /opt/kaldi
RUN tar -jxvf /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2 -C /opt/kaldi/egs/aspire/s5/ \
  && rm /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2

# Prepare the data as described in the README.txt here
WORKDIR /opt/kaldi/egs/aspire/s5

# I've been having problems with either not recording in 8 KHz or Kaldi not recognizing my sampling rate
# I'd assume it's an issue on my end. Enabling downsampling will 'fix' it
RUN steps/online/nnet3/prepare_online_decoding.sh \
  --mfcc-config conf/mfcc_hires.conf data/lang_chain exp/nnet3/extractor exp/chain/tdnn_7b exp/tdnn_7b_chain_online \
  && utils/mkgraph.sh --self-loop-scale 1.0 data/lang_pp_test exp/tdnn_7b_chain_online exp/tdnn_7b_chain_online/graph_pp \
  && echo "--allow_downsample=true" >> /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/mfcc.conf

# -------------------------------------------------------
FROM model-setup as runenv
# Be root i guess, setting up a user and avoiding things being installed in /home is super annoying
#RUN groupadd -r wheel && useradd --no-log-init -r -g wheel server
#RUN chown server:wheel /opt/ristretto/build
#RUN chown server:wheel /opt/ristretto/.vscode
#USER server:wheel
WORKDIR /opt/ristretto
RUN mkdir -p build
ENTRYPOINT "./serverEntry.sh"
