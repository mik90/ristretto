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
  vim && \
  rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/python2.7 /usr/bin/python 

RUN git clone --depth 1 https://github.com/kaldi-asr/kaldi.git /opt/kaldi && \
  cd /opt/kaldi && \
  cd /opt/kaldi/tools && \
  ./extras/install_mkl.sh && \
  make -j $(nproc) && \
  cd /opt/kaldi/src && \
  ./configure --shared --use-cuda && \
  make depend -j $(nproc) && \
  make -j $(nproc)

WORKDIR /opt/kaldi/


# Build kaldi with cmake
# - When using vscode, can add a kaldi/.vscode/settings.json that updates the cmake.configureSettings
# - Kaldi will not build with GCC-9 requires use with 5.4
#   - Are GCC 9 and 5.4 even ABI compatible?
# - Kaldi defaults to OpenBLAS in CMake, so that requires an additiona package
RUN apt-get install -y --no-install-recommends \
  cmake libopenblas-dev
WORKDIR /opt/kaldi
RUN mkdir build
WORKDIR /opt/kaldi/build
RUN cmake .. -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs \
  -DCMAKE_INSTALL_PREFIX=../dist \
  -DCMAKE_BUILD_TYPE=Release
RUN make install

FROM kaldi-ubuntu18.04 as buildenv

# Set up the ristretto build environment
ENV force_color_prompt=yes

RUN apt update \
  && apt install -y --no-install-recommends \
  software-properties-common \
  && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
  | gpg --dearmor - \
  | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null  \
  && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
  && apt update \
  && apt install -y --no-install-recommends \ 
  gcc-9 g++-9 cmake ccache ninja curl libopenblas-dev \
  python3 python3-distutils

# After building kaldi, set gcc-9 as the default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60  \
  --slave /usr/bin/g++ g++ /usr/bin/g++-9

# The extra size is fine for now
#RUN rm -rf /var/lib/apt/lists/*
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3 get-pip.py && rm get-pip.py
RUN pip3 install conan

FROM buildenv as model-setup
# Set up the model

# Need to unzip the trained aspire model with HCLG into the egs/aspire/s5/exp dir
# Don't want to store this in the repo but i also don't want to download it again since it's 793 MB
# Could do something like
# wget http://kaldi-asr.org/models/1/0001_aspire_chain_model_with_hclg.tar.bz2
COPY 0001_aspire_chain_model_with_hclg.tar.bz2 /opt/kaldi
RUN tar -jxvf /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2 -C /opt/kaldi/egs/aspire/s5/
RUN rm /opt/kaldi/0001_aspire_chain_model_with_hclg.tar.bz2

WORKDIR /opt/kaldi/egs/aspire/s5
# The README.txt in this directory gives some useful info
RUN steps/online/nnet3/prepare_online_decoding.sh \
  --mfcc-config conf/mfcc_hires.conf data/lang_chain exp/nnet3/extractor exp/chain/tdnn_7b exp/tdnn_7b_chain_online
RUN utils/mkgraph.sh --self-loop-scale 1.0 data/lang_pp_test exp/tdnn_7b_chain_online exp/tdnn_7b_chain_online/graph_pp
# I've been having problems with either not recording in 8 KHz or Kaldi not recognizing my sampling rate
# I'd assume it's an issue on my end. Enabling downsampling will 'fix' it
RUN echo "--allow_downsample=true" >> /opt/kaldi/egs/aspire/s5/exp/tdnn_7b_chain_online/conf/mfcc.conf

FROM model-setup as grpc-build
# Reference: https://github.com/npclaudiu/grpc-cpp-docker/blob/master/Dockerfile
RUN apt update \
  && apt install -y --no-install-recommends \
  autoconf automake \
  build-essential libtool \
  pkg-config unzip
ENV GRPC_RELEASE_TAG v1.128.2

RUN git clone -b ${GRPC_RELEASE_TAG} https://github.com/grpc/grpc /opt/grpc \
  && /opt/grpc \
  && git submodule update --init --recursive

RUN cd /opt/grpc/third_party/protobuf \
  && ./autogen.sh && ./configure --enable-shared \
  && make -j && make -j check && make install && make clean \
  && ldconfig

RUN cd /opt/grpc && make -j && make install && make clean \
  && ldconfig

FROM grpc-build as runenv
# Be root i guess, setting up a user and avoiding things being installed in /home is super annoying
#RUN groupadd -r wheel && useradd --no-log-init -r -g wheel server
#RUN chown server:wheel /opt/ristretto/build
#RUN chown server:wheel /opt/ristretto/.vscode
#USER server:wheel
WORKDIR /opt/ristretto
CMD ["cmake", "..", "-DBUILD_SERVER=ON", "-DBUILD_CLIENT=OFF", "-DCMAKE_BUILD_TYPE=Debug"]
