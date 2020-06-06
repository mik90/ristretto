FROM alpine:latest as buildenv

RUN mkdir -pv /opt/ristretto
WORKDIR /opt/ristretto

RUN apk add --update \
  gcc g++ ccache clang cmake \
  git \
  python3 \
  zlib-dev \
  cmake make ninja \
  curl \
  grpc-dev protobuf-dev \
  alsa-lib-dev \
  musl-locales \
  && rm -rf /var/cache/apk/*

# Install conan
RUN curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py \
  && python3 get-pip.py && rm get-pip.py \
  && pip3 install conan

# -------------------------------------------------------
FROM buildenv as runenv
# Be root i guess, setting up a user and avoiding things being installed in /home is super annoying
#RUN groupadd -r wheel
#RUN useradd --no-log-init -r -g wheel client
#RUN chown client:wheel /opt/ristretto/build && chown client:wheel /opt/ristretto/.vscode
#USER client:wheel
# There's a little bug in Alpine, work around it to get color prompts
RUN mv /etc/profile.d/color_prompt /etc/profile.d/color_prompt.sh
WORKDIR /opt/ristretto
RUN mkdir -pv build
ENTRYPOINT "./clientEntry.sh"
