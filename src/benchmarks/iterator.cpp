#ifndef _WIN32
#include <sys/mman.h>
#endif

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <argparse/argparse.hpp>
#include <benchmark/benchmark.h>

#ifdef ENABLE_PERF
#include <linux/perf_event.h>
#include <utils/perf.hpp>
#endif

std::size_t mmapAllocationSize(std::size_t needed, std::size_t pageSize) {
    // mmap tends to return the extra space in a page, and also requests for that extra size to be passed in munmap.
    const auto extra = pageSize - needed % pageSize;
    return needed + extra;
}

std::vector<std::uint8_t*> allocateObjects(const std::string& policy, std::size_t nObjects, std::size_t allocationSize,
                                           std::mt19937_64& generator) {
    std::vector<std::uint8_t*> objects;
    objects.reserve(nObjects);

    if (policy == "individual-malloc") {
        for (std::uint64_t i = 0; i < nObjects; ++i) {
            auto* object = reinterpret_cast<std::uint8_t*>(std::malloc(allocationSize));
            for (std::size_t j = 0; j < allocationSize; ++j) {
                object[j] = static_cast<std::uint8_t>(std::uniform_int_distribution<>()(generator));
            }
            objects.push_back(object);
        }
    } else {
        std::uint8_t* allocation = nullptr;
        if (policy == "arena-malloc") {
            allocation = reinterpret_cast<std::uint8_t*>(std::malloc(nObjects * allocationSize));
        }
#ifndef _WIN32
        else if (policy == "arena-mmap") {
            // Assuming default page size is 4096...
            allocation
                = reinterpret_cast<std::uint8_t*>(mmap(nullptr, mmapAllocationSize(nObjects * allocationSize, 4096),
                                                       PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        }
#ifndef __APPLE__
        else if (policy == "arena-mmap-hugepage") {
            // Assuming huge page size of 2MB...
            allocation = reinterpret_cast<std::uint8_t*>(
                mmap(nullptr, mmapAllocationSize(nObjects * allocationSize, 1 << 21), PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0));
        }
#endif
#endif
        else {
            std::abort();
        }

        if (allocation == nullptr) {
            std::cerr << "Failed to allocate memory..." << std::endl;
            std::cerr << std::strerror(errno) << std::endl;
            std::abort();
        }

        for (std::uint64_t i = 0; i < nObjects; ++i) {
            auto* object = allocation + i * allocationSize;
            for (std::size_t j = 0; j < allocationSize; ++j) {
                object[j] = static_cast<std::uint8_t>(std::uniform_int_distribution<>()(generator));
            }
            objects.push_back(object);
        }
    }

    return objects;
};

void runBenchmark(std::uint64_t iterations, const std::vector<std::uint8_t*>& objects) {
    const auto nObjects = objects.size();
    std::uint64_t sum = 0;
    for (std::uint64_t i = 0; i < iterations; ++i) {
        const auto* ptr = objects[i % nObjects];
        sum += *ptr;
    }
    benchmark::DoNotOptimize(sum);
}

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
    program.add_argument("--allocation-policy")
        .help("the allocation policy to use")
        .default_value("individual-malloc")
#ifdef _WIN32
        .choices("individual-malloc", "arena-malloc")
#elif defined(__APPLE__)
        .choices("individual-malloc", "arena-malloc", "arena-mmap")
#else
        .choices("individual-malloc", "arena-malloc", "arena-mmap", "arena-mmap-hugepage")
#endif
        .metavar("POLICY");
    program.add_argument("--seed")
        .help("initial seed for the random number generator")
        .default_value(std::random_device()())
        .scan<'d', unsigned int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    const auto allocationSize = program.get<std::size_t>("--allocation-size");
    const auto footprint = program.get<std::uint64_t>("--footprint");
    const auto nObjects
        = static_cast<std::size_t>(std::ceil(static_cast<double>(footprint) / static_cast<double>(allocationSize)));
    const auto iterations = program.get<std::uint64_t>("--iterations");
    const auto policy = program.get<std::string>("--allocation-policy");
    const auto seed = program.get<unsigned int>("seed");

    std::cout << "allocation size   : " << allocationSize << std::endl;
    std::cout << "number of objects : " << nObjects << std::endl;
    std::cout << "iterations        : " << iterations << std::endl;
    std::cout << "seed              : " << seed << std::endl;

    std::mt19937_64 generator(seed);
    std::cout << "Allocating " << nObjects << " objects of size " << allocationSize << "..." << std::endl;
    std::vector<std::uint8_t*> objects = allocateObjects(policy, nObjects, allocationSize, generator);

    std::cout << "Iterating..." << std::endl;

#ifdef ENABLE_PERF
    const auto events = std::vector<std::pair<std::uint32_t, std::uint64_t>>(
        {{PERF_TYPE_HW_CACHE,
          PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)},
         {PERF_TYPE_HW_CACHE,
          PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)},
         {PERF_TYPE_HW_CACHE,
          PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)},
         {PERF_TYPE_HW_CACHE,
          PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)}});
    utils::perf::Group group(events);
    group.reset();
    group.enable();
#endif
    const auto start = std::chrono::high_resolution_clock::now();

    runBenchmark(iterations, objects);

    const auto end = std::chrono::high_resolution_clock::now();
#ifdef ENABLE_PERF
    group.disable();
#endif

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << " ms." << std::endl;
#ifdef ENABLE_PERF
    const auto counts = group.read();
    std::cout << "dTLB read miss rate: " << 100.0 * static_cast<double>(counts[0]) / static_cast<double>(counts[2])
              << "%" << std::endl;
    std::cout << "dTLB write miss rate: " << 100.0 * static_cast<double>(counts[1]) / static_cast<double>(counts[3])
              << "%" << std::endl;
    std::cout << "dTLB total miss rate: "
              << 100.0 * static_cast<double>(counts[0] + counts[1]) / static_cast<double>(counts[2] + counts[3]) << "%"
              << std::endl;
#endif

    if (policy == "individual-malloc") {
        for (auto* object : objects) {
            std::free(object);
        }
    } else if (policy == "arena-malloc") {
        std::free(objects[0]);
    }
#ifndef _WIN32
    else if (policy == "arena-mmap" || policy == "arena-mmap-hugepage") {
        const auto status = munmap(
            objects[0], mmapAllocationSize(nObjects * allocationSize, policy == "arena-mmap" ? 4096 : 1 << 21));
        if (status != 0) {
            std::cerr << "Failed to free memory..." << std::endl;
            std::cerr << std::strerror(errno) << std::endl;
            std::abort();
        }
    }
#endif
    else {
        std::abort();
    }
}
