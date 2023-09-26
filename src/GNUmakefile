all: malloc-counter.dylib microbenchmark-pages-new

malloc-counter.dylib: GNUmakefile malloc-counter.cpp
	clang++ --std=c++20 -compatibility_version 1 -current_version 1 -dynamiclib malloc-counter.cpp printf.cpp -o malloc-counter.dylib

microbenchmark-pages-new: GNUmakefile microbenchmark-pages-new.cpp
	clang++ --std=c++20 -DNDEBUG -O3 -DOBJECT_SIZE=32 -DOBJECT_DISTANCE=4096 -std=c++20 microbenchmark-pages-new.cpp printf.cpp -o microbenchmark-pages-new
