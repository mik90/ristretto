FROM kaldiasr/kaldi:gpu-latest as buildenv

# Set up the build environment
ENV force_color_prompt=yes

RUN apt update \
  && apt install -y --no-install-recommends \
  software-properties-common \
  && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
  | gpg --dearmor - \
  | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null  \
  && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ xenial main' \
  && apt update \
  && apt install -y --no-install-recommends \ 
  gcc-9 g++-9 cmake ninja curl clang-format libopenblas-dev
  

# When building Kaldi with CMake:
# - May want to pull in libopenblas-dev as the Kaldi cmake targets look for that
# - Use:
# - Use:-DCMAKE_INSTALL_PREFIX=../dist
# - When using vscode, can add a kaldi/.vscode/settings.json that updates the cmake.configureSettings
# - Kaldi will not build with GCC-9 requires use with 5.4
#   - Are GCC 9 and 5.4 even ABI compatible?
# - Need to run "make install" after building
WORKDIR /opt/kaldi
RUN mkdir build
WORKDIR /opt/kaldi/build
RUN cmake .. -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs \
             -DCMAKE_INSTALL_PREFIX=../dist \
             -DCMAKE_BUILD_TYPE=Release
RUN make install

# After building kaldi, set gcc-9 as the default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60  \
  --slave /usr/bin/g++ g++ /usr/bin/g++-9

# The extra size is fine for now
#RUN rm -rf /var/lib/apt/lists/*
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3 get-pip.py && rm get-pip.py
RUN pip3 install conan

FROM buildenv as runenv
# Set up the model

# Need to unzip the trained aspire model with HCLG into the egs/aspire/s5/exp dir
# Don't want to store this in the repo but i also don't want to download it again since it's 793 MB
# Link: http://kaldi-asr.org/models/1/0001_aspire_chain_model_with_hclg.tar.bz2
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