# On Littering

## Experimental Platform

Data was obtained on a Thinkpad P15s Gen 2 with an Intel i7-1165G7 processor.

|          | Version |
|----------|---------|
| glibc    | 2.31    |
| jemalloc | 5.3.0   |
| mimalloc | 2.0.6   |

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
