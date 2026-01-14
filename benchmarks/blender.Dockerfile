FROM ubuntu

RUN apt update \
    && DEBIAN_FRONTEND=noninteractive apt install -y tzdata \
    && apt install -y git python3 sudo wget \
    && apt install -y build-essential git git-lfs cmake \
    && apt install -y ninja-build cmake-curses-gui cmake-gui patch \
    && apt install -y libx11-dev libxxf86vm-dev libxcursor-dev libxi-dev libxrandr-dev libxinerama-dev libxkbcommon-dev libwayland-dev libdecor-0-dev wayland-protocols libdbus-1-dev libgl-dev libegl-dev \
    && apt clean && rm -rf /var/lib/apt/lists/*

ARG BLENDER_HASH=a3db93c5b2595a79f65f304114c23aeef8c2139f # v5.0.1
RUN --mount=type=bind,source=benchmarks/blender-shim.patch,target=/root/blender-shim.patch \
    git clone https://projects.blender.org/blender/blender.git /root/blender \
    && cd /root/blender \
    && git checkout $BLENDER_HASH \
    && /root/blender/build_files/utils/make_update.py \
    && cmake /root/blender -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DWITH_MEM_JEMALLOC=OFF \
    && cmake --build /root/blender/build \
    && cmake --install /root/blender/build --prefix /root/blender-vanilla \
    # && git apply /root/blender-shim.patch \
    # && cmake --build /root/blender/build \
    # && cmake --install /root/blender/build --prefix /root/blender-shim \
    && rm -rf /root/blender

WORKDIR /root
RUN for scene in barbershop_interior bmw27 classroom fishy_cat junkshop koro monster pavillon_barcelona victor; do \
        wget --no-verbose https://opendata.blender.org/cdn/BlenderBenchmark2.0/scenes/$scene.tar.bz2 -O - | tar xj; \
    done

RUN --mount=type=bind,source=.,target=/root/litterer-src \
    cmake /root/litterer-src -B /root/litterer-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build /root/litterer-build \
    && cmake --install /root/litterer-build --prefix /usr \
    && rm -r /root/litterer-build

# LD_PRELOAD=libdetector_distribution.so ../blender-shim/blender --background --factory-startup -noaudio --debug-cycles --enable-autoexec --engine CYCLES junkshop.blend --python main.py --threads 1 -- --device-type CPU

# env LD_PRELOAD=liblitterer_distribution_standalone.so ../blender-shim/blender --background --factory-startup -noaudio --debug-cycles --enable-autoexec --engine CYCLES junkshop.blend --python main.py --threads 1 -- --device-type CPU
# env LD_PRELOAD=liblitterer_distribution_standalone.so ../blender-vanilla/blender --background --factory-startup -noaudio --debug-cycles --enable-autoexec --engine CYCLES junkshop.blend --python main.py --threads 1 -- --device-type CPU


# ./blender-vanilla/blender -b classroom/main.blend -E CYCLES -f 1 --threads 1
