#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include <argparse/argparse.hpp>
#include <benchmark/benchmark.h>

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "", argparse::default_arguments::help);
    program.add_argument("-s", "--allocation-size")
        .required()
        .help("the size in bytes of each allocated object")
        .metavar("SIZE")
        .scan<'d', std::size_t>();
    program.add_argument("-f", "--footprint")
        .required()
        .help("the total number of bytes that should be allocated (n = f / s)")
        .metavar("SIZE")
        .scan<'d', std::uint64_t>();
    program.add_argument("-i", "--iterations")
        .required()
        .help("the number of iterations over the entire allocated population")
        .metavar("N")
        .scan<'d', std::uint64_t>();
    program.add_argument("--seed")
        .help("initial seed for the random number generator")
        .default_value(std::random_device()())
        .scan<'d', unsigned int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return EXIT_FAILURE;
    }

    const auto allocationSize = program.get<std::size_t>("--allocation-size");
    const auto footprint = program.get<std::uint64_t>("--footprint");
    const auto nObjects
        = static_cast<std::size_t>(std::ceil(static_cast<double>(footprint) / static_cast<double>(allocationSize)));
    const auto iterations = program.get<std::uint64_t>("--iterations");
    const auto seed = program.get<unsigned int>("seed");

    std::cout << "allocation size   : " << allocationSize << std::endl;
    std::cout << "number of objects : " << nObjects << std::endl;
    std::cout << "iterations        : " << iterations << std::endl;
    std::cout << "seed              : " << seed << std::endl;

    std::mt19937_64 generator(seed);

    std::cout << "Allocating " << nObjects << " objects of size " << allocationSize << "..." << std::endl;
    std::vector<void*> objects;
    objects.reserve(nObjects);
    for (std::uint64_t i = 0; i < nObjects; ++i) {
        void* object = std::malloc(allocationSize);
        for (std::size_t j = 0; j < allocationSize; ++j) {
            static_cast<std::uint8_t*>(object)[j]
                = static_cast<std::uint8_t>(std::uniform_int_distribution<>()(generator));
        }
        objects.push_back(object);
    }

    std::cout << "Iterating..." << std::endl;
    const auto start = std::chrono::high_resolution_clock::now();

    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < iterations; ++i) {
        const auto offset = std::uniform_int_distribution<std::size_t>(0, allocationSize - 1)(generator);

        for (const auto* object : objects) {
            const auto* ptr = static_cast<const std::uint8_t*>(object) + offset;
            sum += *ptr;
        }
    }
    benchmark::DoNotOptimize(sum);

    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << " ms." << std::endl;
}
