# Osmium

## Cache sizes

These numbers are either per-core or per-processor.
LLC size is 60MiB per-processor.

```
> getconf LEVEL1_DCACHE_SIZE
49152

> getconf LEVEL2_CACHE_SIZE
2097152

> getconf LEVEL3_CACHE_SIZE
62914560
```

Intel® 64 and IA-32 Architectures Optimization
^^^ TLB information.

## llvm-project

```
> docker build -f benchmarks/llvm-project.Dockerfile .

> hyperfine \
    './llvm-project-vanilla/bin/clang -O3 -c sqlite3.c' \
    './llvm-project-shim/bin/clang -O3 -c sqlite3.c'

Benchmark 1: ./llvm-project-vanilla/bin/clang -O3 -c sqlite3.c
  Time (mean ± σ):     16.751 s ±  0.111 s    [User: 16.650 s, System: 0.100 s]
  Range (min … max):   16.605 s … 16.908 s    10 runs

Benchmark 2: ./llvm-project-shim/bin/clang -O3 -c sqlite3.c
  Time (mean ± σ):     17.873 s ±  0.209 s    [User: 17.744 s, System: 0.128 s]
  Range (min … max):   17.669 s … 18.280 s    10 runs

Summary
  ./llvm-project-vanilla/bin/clang -O3 -c sqlite3.c ran
    1.07 ± 0.01 times faster than ./llvm-project-shim/bin/clang -O3 -c sqlite3.c


> for i in {1..10}; do time ./llvm-project-vanilla/bin/clang -O3 -c sqlite3.c; done
> for i in {1..10}; do time ./llvm-project-shim/bin/clang -O3 -c sqlite3.c; done

> LD_PRELOAD=libdetector_distribution.so ./llvm-project-shim/bin/clang -O3 -c sqlite3.c

> export LITTER_MULTIPLIER=1
> export LITTER_OCCUPANCY=0.4

> for i in {1..10}; do env LD_PRELOAD=liblitterer_distribution_standalone.so ./llvm-project-vanilla/bin/clang -O3 -c sqlite3.c |& grep "Time elapsed"; done
> for i in {1..10}; do env LD_PRELOAD=liblitterer_distribution_standalone.so ./llvm-project-shim/bin/clang -O3 -c sqlite3.c |& grep "Time elapsed"; done
```

The four commands for reuse distance:

```bash
LITTER_MULTIPLIER=0 LITTER_OCCUPANCY=0.4 ./pin-tools/pin-external-4.1-99687-gd9b8f822c-gcc-linux/pin -t ./pin-tools/reuse/obj-intel64/reuse.so -o llvm-vanilla.reuse.json -attach-on-getpid -- ./llvm-litter-vanilla/bin/clang -c -O3 sqlite3.c

LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.4 ./pin-tools/pin-external-4.1-99687-gd9b8f822c-gcc-linux/pin -t ./pin-tools/reuse/obj-intel64/reuse.so -o llvm-litter-vanilla.reuse.json -attach-on-getpid -- ./llvm-litter-vanilla/bin/clang -c -O3 sqlite3.c

LITTER_MULTIPLIER=0 LITTER_OCCUPANCY=0.4 ./pin-tools/pin-external-4.1-99687-gd9b8f822c-gcc-linux/pin -t ./pin-tools/reuse/obj-intel64/reuse.so -o llvm-shim.reuse.json -attach-on-getpid -- ./llvm-litter-shim/bin/clang -c -O3 sqlite3.c

LITTER_MULTIPLIER=10 LITTER_OCCUPANCY=0.4 ./pin-tools/pin-external-4.1-99687-gd9b8f822c-gcc-linux/pin -t ./pin-tools/reuse/obj-intel64/reuse.so -o llvm-litter-shim.reuse.json -attach-on-getpid -- ./llvm-litter-shim/bin/clang -c -O3 sqlite3.c
```
