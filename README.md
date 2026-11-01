# Reconsidering "Reconsidering Custom Memory Allocation"

[Nicolas van Kempen](https://nvankempen.com/), [Emery D. Berger](https://emeryberger.com/)

[Read the paper!](https://nvankempen.com/public/publications/reconsidering2.pdf)

Programmers using native languages such as C, C++, or Rust can implement custom memory allocation strategies to improve execution time.
This work revisits the influential ["Reconsidering Custom Memory Allocation" paper](https://dl.acm.org/doi/10.1145/582419.582421) almost 25 years later,
on a modern hardware platform with modern general-purpose allocators.
Our results support and extend the original conclusions, demonstrating the locality advantages of region-based custom memory allocators.

## Adversarial allocation / Littering

We initially named "littering" the technique introduced in the paper as "adversarial allocation". Therefore, you will find references to both
in the text and code present in this repository.

To build the litterer, run the following commands.

```bash
cmake . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

You can then use it like so.

```bash
# First, run the detector to get a distribution of size classes.
LD_PRELOAD=./build/libdetector_distribution.so <PROGRAM>

# Then run the program.
env LD_PRELOAD=./build/liblitterer_distribution_standalone.so LITTER_MULTIPLIER=1 LITTER_OCCUPANCY=0.4 <PROGRAM>

# Compare performance by running without the litterer, or setting the multiplier to zero.
env LD_PRELOAD=./build/liblitterer_distribution_standalone.so LITTER_MULTIPLIER=0 <PROGRAM>
```

The following litterer settings are available:
 1. `LITTER_DATA_FILENAME`: for both the detector and litterer. Contains the name of the data file where to write/read the size class distribution. (default: `distribution.json`)
 2. `LITTER_DETECTOR_APPEND`: for the detector. Whether to append data to the given size class distribution file, or overwrite it. (default: no)
 3. `LITTER_LOG_FILENAME`: for the litterer. File where debug and result information will be logged. (default: stderr)
 4. `LITTER_SEED`: for the litterer. Seed to use during the random generation and disposal of allocated objects. (default: random seed)
 5. `LITTER_OCCUPANCY`: for the litterer. Fraction (between 0 and 1) of objects to **keep** on the heap after random allocation. (default: 0.95)
 6. `LITTER_SHUFFLE`: for the litterer. Whether to shuffle allocated objects before freeing a fraction of them. (default: yes)
 7. `LITTER_SORT`: for the litterer. Whether to sort allocated objects before freeing a fraction of them. (default: no)
 8. `LITTER_SLEEP`: for the litterer. Number of seconds to sleep after littering, but before starting the program. This is useful to attach a profiler for example. (default: 0)
 9. `LITTER_MULTIPLIER`: for the litterer. This number, multiplied by the maximum number of live allocations as measured by the detector, is the number of allocated objects during the littering process. (default: 20)

## Benchmarks

The following benchmarks are "documented" in this repo:
 1. **197.parser, 176.gcc, 175.vpr**: See sample commands in [SPEC2000.md](SPEC2000.md). You will need a copy of the SPEC benchmarks.
 2. **boxed-sim, mudlle**: See sample commands in [SPEC2000.md](SPEC2000.md).
 4. **blender**: See sample commands in [blender/BLENDER.md](blender/BLENDER.md).
 5. **rustc**: See sample commands in [rust/RUST.md](rust/RUST.md).
 3. **llvm/clang**: Use [docker/llvm.Dockerfile](docker/llvm.Dockerfile).

The [graphs/run.py](graphs/run.py) script may provide some additional documentation by example. It assumes that sources are present in the
directory and built.