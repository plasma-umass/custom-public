#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <random>
#include <vector>

#include <argparse/argparse.hpp>
#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "", argparse::default_arguments::help);
    program.add_argument("-s", "--allocation-size")
        .required()
        .help("the size of each allocated object (B).")
        .metavar("SIZE")
        .scan<'d', std::size_t>();
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
    program.add_argument("--seed").default_value(std::random_device()()).scan<'d', unsigned int>();
    program.add_argument("--min-chunk-size").default_value(std::size_t(8)).scan<'d', std::size_t>();
    program.add_argument("--max-chunk-size").default_value(std::size_t(4096)).scan<'d', std::size_t>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return EXIT_FAILURE;
    }

    const auto allocationSize = program.get<std::size_t>("--allocation-size");
    const auto nObjects = program.get<std::uint64_t>("--number-of-objects");
    const auto iterations = program.get<std::uint64_t>("--iterations");
    const auto seed = program.get<unsigned int>("seed");
    auto minChunkSize = program.get<std::size_t>("--min-chunk-size");
    if (minChunkSize > allocationSize) {
        minChunkSize = 1;
    }
    const auto maxChunkSize = std::min(program.get<std::size_t>("--max-chunk-size"), allocationSize);

    std::cout << "allocation size   : " << allocationSize << std::endl;
    std::cout << "number of objects : " << nObjects << std::endl;
    std::cout << "iterations        : " << iterations << std::endl;
    std::cout << "seed              : " << seed << std::endl;
    std::cout << "chunk size        : " << minChunkSize << " - " << maxChunkSize << std::endl;

    std::mt19937_64 generator(seed);

    std::cout << "Allocating " << nObjects << " objects of size " << allocationSize << "..." << std::endl;
    std::vector<void*> objects;
    objects.reserve(nObjects);
    for (std::uint64_t i = 0; i < nObjects; ++i) {
        objects.push_back(std::malloc(allocationSize));
    }

    std::cout << "Iterating..." << std::endl;
    const auto start = std::chrono::high_resolution_clock::now();

    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < iterations; ++i) {
        const auto size = std::uniform_int_distribution<std::size_t>(minChunkSize, maxChunkSize)(generator);
        const auto offset = std::uniform_int_distribution<std::size_t>(0, allocationSize - size)(generator);

        for (const auto* object : objects) {
            const auto* ptr = static_cast<const std::uint8_t*>(object) + offset;
            for (std::size_t k = 0; k < size; ++k) {
                sum += ptr[k];
            }
        }
    }
    benchmark::DoNotOptimize(sum);

    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << "ms." << std::endl;
}
