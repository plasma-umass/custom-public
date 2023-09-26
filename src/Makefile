all: microbenchmark-pages-new

microbenchmark-pages-new: Makefile microbenchmark-pages-new.cpp
	cl /std:c++20 /F 0x10000000 /EHsc -DNDEBUG /Ox /MD -DOBJECT_SIZE=32 -DOBJECT_DISTANCE=4096 microbenchmark-pages-new.cpp printf.cpp
