```bash
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/blender/blender.git
cd blender
GIT_LFS_SKIP_SMUDGE=1 git checkout a3db93c5b2595a79f65f304114c23aeef8c2139f # v5.0.1
make update
git clone https://projects.blender.org/blender/blender-benchmarks.git tests/benchmarks
rm tests/performance/tests/sculpt.py
ln -s ../sculpt.py tests/performance/tests/sculpt.py
```

Build all configurations...

```bash
patch -p1 -i ../blender.patch
cmake . -B build-vanilla -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
cmake --build build-vanilla
ninja -C build-vanilla install

patch -p1 -i ../blender.memarena.patch
cmake . -B build-minus-memarena -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
cmake --build build-minus-memarena
ninja -C build-minus-memarena install
patch -p1 -R -i ../blender.memarena.patch

patch -p1 -i ../blender.mempool.patch
cmake . -B build-minus-mempool -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
cmake --build build-minus-mempool
ninja -C build-minus-mempool install
patch -p1 -R -i ../blender.mempool.patch

patch -p1 -i ../blender.linear.patch
cmake . -B build-minus-linear -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
cmake --build build-minus-linear
ninja -C build-minus-linear install
patch -p1 -R -i ../blender.linear.patch

patch -p1 -i ../blender.memarena.patch
patch -p1 -i ../blender.mempool.patch
patch -p1 -i ../blender.linear.patch
cmake . -B build-minus-all -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
cmake --build build-minus-all
ninja -C build-minus-all install
patch -p1 -R -i ../blender.memarena.patch
patch -p1 -R -i ../blender.mempool.patch
patch -p1 -R -i ../blender.linear.patch
```

Run benchmarks:

```

```
