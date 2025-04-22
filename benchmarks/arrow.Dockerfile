FROM ubuntu

RUN apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y tzdata \
    && apt install -y build-essential cmake git ninja-build \
    && apt install -y libbenchmark-dev libboost-all-dev libgflags-dev libxsimd-dev rapidjson-dev \
    && apt clean && rm -rf /var/lib/apt/lists/*

WORKDIR /root

ARG ARROW_HASH=8b56d7e9e0f82bf87b82aa5204cd5ff1312768fc # Arbitrary, latest release does not work.
RUN --mount=type=bind,source=arrow.patch,target=/root/arrow.patch \
    git clone https://github.com/apache/arrow.git \
    && cd arrow/cpp \
    && git checkout $ARROW_HASH \
    && git apply ../../arrow.patch \
    && cmake . -G Ninja -B /root/arrow-vanilla -DCMAKE_BUILD_TYPE=Release -DARROW_BUILD_BENCHMARKS=ON \
    && cmake --build /root/arrow-vanilla \
    && cd ../.. \
    && rm -rf arrow
    # && git apply ../arrow-shim.patch \
    # && cmake llvm -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
    # && cmake --build build \
    # && cmake --install build --prefix /root/arrow-shim \
    # && cd .. \
