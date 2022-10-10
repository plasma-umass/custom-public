# On Littering

## Littering

Littering is a method developped to measure the benefits of custom allocators without unfairly favoritizing the shimmed
version of the program. It works by running a short pre-conditioning step before starting the program, allocating and
freeing objects successively to artifically fragment the heap. This simulates real conditions of long-running programs,
where fragmentation is inevitable.

More precisely, our version littering works in two steps.
 1. `LD_PRELOAD=libdetector.so <program>`. This keeps track of every allocated object's size, binning sizes to obtain a
    rough size distribution of the objects used by the program. It also keeps track of a few statistics we can calculate
    online, such as mean, min/max, and most importantly `MaxLiveAllocations`.
 2. `LD_PRELOAD=liblitterer.so <program>`. This is when we actually run the litterering step, along with the program
    when done. This works by allocating `LITTER_MULTIPLIER * MaxLiveAllocations` objects following the recorded size
    distribution, (optionally) shuffling the addresses, and freeing a fraction of `1 - LITTER_OCCUPANCY` of them. We
    use the same `malloc` as the program will, hence the program will start with existing "litter" on the heap. There
    are a few tunable parameters.
     -  `LITTER_SEED`: Seed to be used for the random number generator. Random by default.
     -  `LITTER_OCCUPANCY`: Fraction of objects to _keep_ on the heap. Value must be between 0 and 1.
     -  `LITTER_NO_SHUFFLE`: Set to 1 to disable shuffling, and free from the last allocated objects.
     -  `LITTER_SLEEP`: Sleep _x_ seconds after littering, but before starting the program. Default is disabled.
     -  `LITTER_MULTIPLIER`: Multiplier of number of objects to allocate. Default is 20.

## Experimental Platform

Data was obtained on a Thinkpad P15s Gen 2 with an Intel i7-1165G7 processor.

|          | Version |
|----------|---------|
| glibc    | 2.31    |
| jemalloc | 5.3.0   |
| mimalloc | 2.0.6   |

Results below were obtained with `LITTER_MULTIPLIER = 1`, varying the occupancy number.

## Graphs

First, some `197.parser` specific graphs, using the Linux default allocator, jemalloc, and mimalloc.
The elapsed time graph is normalized by the elapsed time of the program running with the custom allocator, no littering.

![TimeByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/TimeByOccupancy.197.parser.png)
![LLCMissRateByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/LLCMissRateByOccupancy.197.parser.png)
![InstructionsByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/InstructionsByOccupancy.197.parser.png)

We also generate graphs separating events counted with `perf`. We assign events in/out malloc based on which DSO the
sample comes from.

![SeparatedCyclesByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/SeparatedCyclesByOccupancy.197.parser.png)
![SeparatedLLCMissesByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/SeparatedLLCMissesByOccupancy.197.parser.png)
![SeparatedInstructionsByOccupancy.197.parser](https://github.com/plasma-umass/custom-public/raw/master/graphs/SeparatedInstructionsByOccupancy.197.parser.png)


## References

[1] Emery D. Berger, Benjamin G. Zorn, and Kathryn S. McKinley. 2002. Reconsidering custom memory allocation. In Proceedings of the 17th ACM SIGPLAN conference on Object-oriented programming, systems, languages, and applications (OOPSLA '02). Association for Computing Machinery, New York, NY, USA, 1–12. https://doi.org/10.1145/582419.582421
