```bash
rm -rf blender benchmark/default/results.*
GIT_LFS_SKIP_SMUDGE=1 git clone https://github.com/blender/blender.git
cd blender
GIT_LFS_SKIP_SMUDGE=1 git checkout a3db93c5b2595a79f65f304114c23aeef8c2139f # v5.0.1
make update
git clone https://projects.blender.org/blender/blender-benchmarks.git tests/benchmarks

# Custom setup for the test files.
rm tests/performance/tests/sculpt.py
ln -s `pwd`/../sculpt.py tests/performance/tests/sculpt.py
rm tests/performance/tests/geometry_nodes.py
ln -s `pwd`/../geometry_nodes.py tests/performance/tests/geometry_nodes.py
cd tests/benchmarks/geometry_nodes \
    && rm -rf brushstroke_tools delete_geometry dual_mesh erindale_bejewelled erindale_flower_shop erindale_mouse_house extrude field many_auto_smooth_shared_mesh many_nodes_test_files mesh_island normals split_edges \
    && cd -
```

Build all configurations...

```bash
patch -p1 -i ../blender.patch

cmake . -B build-vanilla -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
ninja -C build-vanilla
ninja -C build-vanilla install

patch -p1 -i ../blender.memarena.patch
cmake . -B build-minus-memarena -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
ninja -C build-minus-memarena
ninja -C build-minus-memarena install
patch -p1 -R -i ../blender.memarena.patch

patch -p1 -i ../blender.mempool.patch
cmake . -B build-minus-mempool -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
ninja -C build-minus-mempool
ninja -C build-minus-mempool install
patch -p1 -R -i ../blender.mempool.patch

patch -p1 -i ../blender.linear.patch
cmake . -B build-minus-linear -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
ninja -C build-minus-linear
ninja -C build-minus-linear install
patch -p1 -R -i ../blender.linear.patch

patch -p1 -i ../blender.memarena.patch
patch -p1 -i ../blender.mempool.patch
patch -p1 -i ../blender.linear.patch
cmake . -B build-minus-all -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DWITH_MEM_JEMALLOC=OFF -DWITH_TBB_MALLOC_PROXY=OFF \
        -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
ninja -C build-minus-all
ninja -C build-minus-all install
patch -p1 -R -i ../blender.memarena.patch
patch -p1 -R -i ../blender.mempool.patch
patch -p1 -R -i ../blender.linear.patch
```

Run geometry_nodes benchmarks:

```bash
# Arena:
./build-vanilla/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));result = tests.geometry_nodes._run(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Malloc/free:
./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));result = tests.geometry_nodes._run(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Detector:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/libdetector_distribution.so LITTER_DATA_FILENAME=../blender.geometry_nodes.bfield.json ./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));result = tests.geometry_nodes._run(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Malloc/free with littering:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/liblitterer_distribution_standalone.so LITTER_DATA_FILENAME=../blender.geometry_nodes.bfield.json LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.8 ./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));result = tests.geometry_nodes._run(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Arena with littering:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/liblitterer_distribution_standalone.so LITTER_DATA_FILENAME=../blender.geometry_nodes.bfield.json LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.8 ./build-vanilla/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/geometry_nodes/foreach_geometry_element/foreach_zone_bfield.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.geometry_nodes;args = pickle.loads(base64.b64decode(b"gASVJwAAAAAAAAB9lIwKbG9nX3ByZWZpeJSME2ZvcmVhY2hfem9uZV9iZmllbGSUcy4="));result = tests.geometry_nodes._run(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'
```

Run sculpt benchmarks:

```bash
# Arena:
./build-vanilla/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/sculpt/brushes/base.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));result = tests.sculpt._run_brush_test(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Malloc/free:
./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/sculpt/brushes/base.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));result = tests.sculpt._run_brush_test(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'


# Detector:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/libdetector_distribution.so LITTER_DATA_FILENAME=../blender.sculpt.clay_strips.json ./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/sculpt/brushes/base.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));result = tests.sculpt._run_brush_test(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Malloc/free with littering:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/liblitterer_distribution_standalone.so LITTER_DATA_FILENAME=../blender.sculpt.clay_strips.json LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.8 ./build-minus-all/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/sculpt/brushes/base.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));result = tests.sculpt._run_brush_test(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'

# Arena with littering:
BLENDER_RESTORE_LD_PRELOAD='' LD_PRELOAD=../../build/liblitterer_distribution_standalone.so LITTER_DATA_FILENAME=../blender.sculpt.clay_strips.json LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.8 ./build-vanilla/bin/blender --factory-startup --threads 1 -noaudio --enable-autoexec --python-exit-code 1 --background tests/benchmarks/sculpt/brushes/base.blend --python-expr 'import sys, pickle, base64;sys.path.append(r"/home/nvankempen/nicovank/litterer/blender/blender/tests/performance");import tests.sculpt;args = pickle.loads(base64.b64decode(b"gASVjwAAAAAAAAB9lCiMBG1vZGWUjAx0ZXN0cy5zY3VscHSUjApTY3VscHRNb2RllJOUSwOFlFKUjApicnVzaF90eXBllGgCjAlCcnVzaFR5cGWUk5SMC0NsYXkgU3RyaXBzlIWUUpSMD3NwYXRpYWxfcmVvcmRlcpSJjARuYW1llIwTZHludG9wb19jbGF5X3N0cmlwc5R1Lg=="));result = tests.sculpt._run_brush_test(args);result = base64.b64encode(pickle.dumps(result));print("\nTEST_OUTPUT: " + result.decode() + "\n")'
```
