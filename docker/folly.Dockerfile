FROM ubuntu:latest AS build

RUN apt update -y \
    && DEBIAN_FRONTEND=noninterative apt install -y tzdata \
    && apt install -y git libssl-dev python3 python3-pip python3-venv sudo \
    && git clone https://github.com/facebook/folly.git /root/folly-src \
    && cd /root/folly-src \
    && python3 -m venv env \
    && . env/bin/activate \
    && python3 ./build/fbcode_builder/getdeps.py install-system-deps --recursive \
    && python3 ./build/fbcode_builder/getdeps.py --allow-system-packages build --install-dir=/root/folly-install

FROM scratch AS export
COPY --from=build /root/folly-install /folly-install
