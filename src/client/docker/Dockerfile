FROM alpine:latest as buildenv

RUN apk add --update \
  bash \
  gcc g++ ccache clang cmake \
  git \
  python3 \
  zlib-dev \
  cmake make ninja \
  curl \
  grpc-dev protobuf-dev \
  alsa-lib-dev \
  vim

# The extra size is fine for now
#RUN rm -rf /var/cache/apk/*

RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
RUN python3 get-pip.py && rm get-pip.py
RUN pip3 install conan


FROM buildenv as runenv
# Be root i guess, setting up a user and avoiding things being installed in /home is super annoying
#RUN groupadd -r wheel
#RUN useradd --no-log-init -r -g wheel client
#RUN chown client:wheel /opt/ristretto/build && chown client:wheel /opt/ristretto/.vscode
#USER client:wheel
WORKDIR /opt/ristretto
# There's a little bug in Alpine, work around it to get color prompts
RUN mv /etc/profile.d/color_prompt /etc/profile.d/color_prompt.sh
ENTRYPOINT ["./runProtoc.sh &&", "cmake", "..", "-DBUILD_SERVER=ON", "-DBUILD_CLIENT=OFF", "-DCMAKE_BUILD_TYPE=Debug", "-G", "Ninja"]
