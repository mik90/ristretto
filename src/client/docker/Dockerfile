FROM ubuntu:bionic as buildenv

# Set up the ristretto build environment
ENV force_color_prompt=yes

RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  gcc g++ ccache \
  git \
  python3 python3-distutils \
  zlib1g-dev \
  make ninja-build \
  curl \
  libprotobuf-dev protobuf-compiler \
  libgrpc++-dev libgrpc++1 libgrpc-dev libgrpc3 protobuf-compiler-grpc \
  libasound2-dev \
  vim


# Get the newest cmake and GCC 9
RUN apt install -y --no-install-recommends \
  apt-transport-https ca-certificates gnupg wget \
  software-properties-common \
  && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
  | gpg --dearmor - \
  | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null  \
  && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
  && apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main' \
  && apt update \
  && apt install -y --no-install-recommends \
  kitware-archive-keyring

RUN rm /etc/apt/trusted.gpg.d/kitware.gpg
RUN apt install -y --no-install-recommends \
  gcc-9 g++-9 cmake

# Set gcc-9 as the default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60  \
  --slave /usr/bin/g++ g++ /usr/bin/g++-9

RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3 get-pip.py && rm get-pip.py
RUN pip3 install conan

# The extra size is fine for now
#RUN rm -rf /var/lib/apt/lists/*

FROM buildenv as runenv
# Be root i guess, setting up a user and avoiding things being installed in /home is super annoying
#RUN groupadd -r wheel
#RUN useradd --no-log-init -r -g wheel client
#RUN chown client:wheel /opt/ristretto/build && chown client:wheel /opt/ristretto/.vscode
#USER client:wheel
WORKDIR /opt/ristretto
RUN ./runProtoc.sh
ENTRYPOINT ["cmake", "..", "-DBUILD_SERVER=ON", "-DBUILD_CLIENT=OFF", "-DWARNINGS_AS_ERRORS=OFF", "-DCMAKE_BUILD_TYPE=Debug", "-G", "Ninja"]
