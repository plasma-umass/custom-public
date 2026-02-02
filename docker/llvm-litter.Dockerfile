FROM ubuntu:latest AS build

RUN apt update \
    && apt install -y cmake curl git libdigest-sha3-perl ninja-build python3 unzip wget \
    && apt install -y lsb-release wget software-properties-common gnupg \
    && curl -sSf https://apt.llvm.org/llvm.sh | bash -s -- 21 all \
    && apt clean && rm -rf /var/lib/apt/lists/*

ARG LLVM_HASH=2078da43e25a4623cab2d0d60decddf709aaea28 # llvmorg-21.1.8
RUN --mount=type=bind,source=.,target=/root/litterer \
    # TODO: git 2.49: use the --revision flag.
    mkdir /root/llvm \
    && cd /root/llvm \
    && git init \
    && git remote add origin https://github.com/llvm/llvm-project.git \
    && git fetch origin $LLVM_HASH --depth=1 \
    && git checkout FETCH_HEAD \
    # Differences from llvm.Dockerfile start here...
    && cp /root/litterer/src/distribution/*.hpp clang/tools/driver/ \
    && git apply /root/litterer/docker/llvm-litter.patch \
    # ... and end here. Plus some naming differences below.
    && cmake llvm -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_BUILD_TOOLS=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21 \
        -DLLVM_ENABLE_LTO=Thin -DLLVM_TARGETS_TO_BUILD="X86" \
    && cmake --build build \
    && cmake --install build --prefix /root/llvm-litter-vanilla \
    && git apply /root/litterer/docker/llvm-shim.patch \
    && cmake llvm -G Ninja -B build -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_BUILD_TOOLS=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21 \
        -DLLVM_ENABLE_LTO=Thin -DLLVM_TARGETS_TO_BUILD="X86" \
    && cmake --build build \
    && cmake --install build --prefix /root/llvm-litter-shim \
    && cd .. \
    && rm -rf llvm

RUN wget https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip \
    && echo "e7eb4cfb2d95626e782cfa748f534c74482f2c3c93f13ee828b9187ce05b2da7  sqlite-amalgamation-3490100.zip" | sha3sum -a 256 -c - \
    && unzip -p sqlite-amalgamation-3490100.zip sqlite-amalgamation-3490100/sqlite3.c > /root/sqlite3.c \
    && rm sqlite-amalgamation-3490100.zip

ENTRYPOINT ["/bin/bash"]

FROM scratch AS export
COPY --from=build /root/llvm-litter-vanilla /llvm-litter-vanilla
COPY --from=build /root/llvm-litter-shim /llvm-litter-shim
COPY --from=build /root/sqlite3.c /sqlite3.c
