FROM ubuntu

RUN apt update \
    && apt install -y build-essential cmake git libdigest-sha3-perl ninja-build python3 unzip wget \
    && apt clean && rm -rf /var/lib/apt/lists/*

WORKDIR /root

ARG LLVM_HASH=58df0ef89dd64126512e4ee27b4ac3fd8ddf6247 # llvmorg-20.1.2
RUN --mount=type=bind,source=benchmarks/llvm-project-shim.patch,target=/root/llvm-project-shim.patch \
    git clone https://github.com/llvm/llvm-project.git \
    && cd llvm-project \
    && git checkout $LLVM_HASH \
    && cmake llvm -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" \
    && cmake --build build \
    && cmake --install build --prefix /root/llvm-project-vanilla \
    && git apply ../llvm-project-shim.patch \
    && cmake llvm -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" \
    && cmake --build build \
    && cmake --install build --prefix /root/llvm-project-shim \
    && cd .. \
    && rm -rf llvm-project

RUN wget https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip \
    && echo "e7eb4cfb2d95626e782cfa748f534c74482f2c3c93f13ee828b9187ce05b2da7  sqlite-amalgamation-3490100.zip" | sha3sum -a 256 -c - \
    && unzip -p sqlite-amalgamation-3490100.zip sqlite-amalgamation-3490100/sqlite3.c > /root/sqlite3.c \
    && rm sqlite-amalgamation-3490100.zip

ENTRYPOINT ["/bin/bash"]
