FROM ubuntu:latest AS build

RUN apt update -y \
    && apt install -y build-essential cmake git \
    && git clone https://github.com/abseil/abseil-cpp.git /root/abseil-src \
    && cmake /root/abseil-src -B /root/abseil-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/root/abseil-install \
    && cmake --build /root/abseil-build --parallel \
    && cmake --install /root/abseil-build

FROM scratch AS export
COPY --from=build /root/abseil-install /abseil-install
