#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>

#include <argparse/argparse.hpp>
#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "", argparse::default_arguments::help);
    program.add_argument("-s", "--allocation-size")
        .required()
        .help("the size of each allocated object (B).")
        .metavar("SIZE")
        .scan<'d', std::uint64_t>();
    program.add_argument("-n", "--number-of-objects")
        .required()
        .help("the number of objects.")
        .metavar("N")
        .scan<'d', std::uint64_t>();
    program.add_argument("-i", "--iterations")
        .required()
        .help("the number of iterations over the entire allocated population.")
        .metavar("N")
        .scan<'d', std::uint64_t>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return EXIT_FAILURE;
    }

    const auto allocationSize = program.get<std::uint64_t>("--allocation-size");
    const auto nObjects = program.get<std::uint64_t>("--number-of-objects");
    const auto iterations = program.get<std::uint64_t>("--iterations");

    std::cout << "Allocation size  : " << allocationSize << std::endl;
    std::cout << "Number of objects: " << nObjects << std::endl;
    std::cout << "Iterations       : " << iterations << std::endl;

    std::cout << "Allocating " << nObjects << " objects of size " << allocationSize << "B..." << std::endl;
    std::vector<void*> objects;
    objects.reserve(nObjects);
    for (std::uint64_t i = 0; i < nObjects; ++i) {
        objects.push_back(std::malloc(allocationSize));
    }

    std::cout << "Iterating..." << std::endl;
    const auto start = std::chrono::high_resolution_clock::now();
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0, j = 0; i < iterations; ++i) {
        sum += *reinterpret_cast<std::uint8_t*>(objects[j % nObjects]);
    }
    benchmark::DoNotOptimize(sum);
    const auto end = std::chrono::high_resolution_clock::now();

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << "ms." << std::endl;
}
