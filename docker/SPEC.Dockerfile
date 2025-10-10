FROM ubuntu:latest

RUN apt update -y \
    # Debug.
    && apt install -y less \
    # General dependencies.
    && DEBIAN_FRONTEND=noninterative apt install -y tzdata \
    && apt install -y cmake git ninja-build python3 sudo \
    # LLVM/Clang.
    && apt install -y curl lsb-release wget software-properties-common gnupg \
    && curl -sSf https://apt.llvm.org/llvm.sh | sudo bash -s -- 21 all \
    && rm -rf /var/lib/apt/lists/*

ARG SPEC_ISO_MNT

RUN --mount=type=bind,readonly,source=$SPEC_ISO_MNT,target=/mnt \
    /mnt/install.sh -f -d /cpu2017
WORKDIR /cpu2017
RUN echo 'source shrc' >> /etc/bash.bashrc

COPY docker/config /cpu2017/config

# C/C++ benchmarks only.
# https://www.spec.org/cpu2017/Docs/overview.html#benchmarks.
ENV INTRATE="500 502 505 520 523 525 531 541 557"
ENV INTSPEED="600 602 605 620 623 625 631 641 657"
ENV FPRATE="508 510 511 519 526 538 544"
ENV FPSPEED="619 638 644"

RUN --mount=type=bind,readonly,source=.,target=/root/litterer-src \
    cmake /root/litterer-src -B /root/litterer-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21 \
    && cmake --build /root/litterer-build --parallel \
    && cmake --install /root/litterer-build --prefix /usr \
    && rm -r /root/litterer-build
