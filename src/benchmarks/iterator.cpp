#ifndef _WIN32
#include <sys/mman.h>
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
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

namespace {
constexpr std::size_t alignUp(std::size_t size, std::size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

struct Node {
    Node* next;
};

std::vector<Node*> allocateObjects(const std::string& policy,
                                   std::size_t nObjects,
                                   std::size_t allocationSize,
                                   std::mt19937_64& generator) {
    std::vector<Node*> objects;
    objects.reserve(nObjects);

    if (policy == "individual-malloc") {
        for (std::uint64_t i = 0; i < nObjects; ++i) {
            auto* allocation = std::malloc(allocationSize);
            std::memset(allocation, 0, allocationSize);
            auto* object = reinterpret_cast<Node*>(allocation);
            objects.push_back(object);
        }

        return objects;
    }

    void* allocation = nullptr;
    if (policy == "arena-malloc") {
        allocation = std::malloc(nObjects * allocationSize);
    }
#ifndef _WIN32
    else if (policy == "arena-mmap") {
        // Assuming default page size is 4096...
        allocation
            = mmap(nullptr, alignUp(nObjects * allocationSize, 4096),
                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
#ifndef __APPLE__
    else if (policy == "arena-mmap-hugepage") {
        // Assuming huge page size of 2MB...
        allocation = mmap(nullptr, alignUp(nObjects * allocationSize, 1 << 21),
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
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

    std::memset(allocation, 0, allocationSize);

    // Keep the first address in place to use it when releasing the memory.
    objects.push_back(reinterpret_cast<Node*>(allocation));
    for (std::uint64_t i = 1; i < nObjects; ++i) {
        // Keep things 8-aligned.
        std::uniform_int_distribution<std::uint64_t> distribution(
            0, (allocationSize - sizeof(Node)) / 8);
        auto* object = reinterpret_cast<Node*>(
            reinterpret_cast<std::uintptr_t>(allocation) + (i * allocationSize)
            + (8 * distribution(generator)));
        objects.push_back(object);
    }

    return objects;
};

void runBenchmark(std::uint64_t iterations, Node* n) {
    while (--iterations > 0) {
        n = n->next;
    }
    benchmark::DoNotOptimize(n);
}
} // namespace

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "",
                                            argparse::default_arguments::help);
    program.add_argument("-n", "--number-objects")
        .required()
        .help("the number of objects to be allocated")
        .metavar("N")
        .scan<'u', std::uint64_t>();
    program.add_argument("-s", "--allocation-size")
        .default_value(std::uint64_t(4096))
        .help("the size in bytes of each allocated object (min: sizeof(void*))")
        .metavar("SIZE")
        .scan<'u', std::uint64_t>();
    program.add_argument("-i", "--iterations")
        .required()
        .help("the number of iterations over the entire allocated population")
        .metavar("N")
        .scan<'u', std::uint64_t>();
    program.add_argument("-p", "--allocation-policy")
        .help("the allocation policy to use")
        .default_value("individual-malloc")
#ifdef _WIN32
        .choices("individual-malloc", "arena-malloc")
#elif defined(__APPLE__)
        .choices("individual-malloc", "arena-malloc", "arena-mmap")
#else
        .choices("individual-malloc", "arena-malloc", "arena-mmap",
                 "arena-mmap-hugepage")
#endif
        .metavar("POLICY");
    program.add_argument("--seed")
        .help("initial seed for the random number generator")
        .default_value(std::random_device()())
        .scan<'d', unsigned int>();
    program.add_argument("--no-shuffle")
        .help("disable shuffling the cycle")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    const auto nObjects = program.get<std::uint64_t>("--number-objects");
    const auto allocationSize = program.get<std::uint64_t>("--allocation-size");
    const auto iterations = program.get<std::uint64_t>("--iterations");
    const auto policy = program.get<std::string>("--allocation-policy");
    const auto seed = program.get<unsigned int>("--seed");
    const auto shuffle = !program.get<bool>("--no-shuffle");

    if (allocationSize < sizeof(Node)) {
        std::cerr << "Allocation size must be at least sizeof(Node) = "
                  << sizeof(void*) << "." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "policy            : " << policy << std::endl;
    std::cout << "number of objects : " << nObjects << std::endl;
    std::cout << "allocation size.  : " << allocationSize << std::endl;
    std::cout << "iterations        : " << iterations << std::endl;
    std::cout << "seed              : " << seed << std::endl;
    std::cout << "shuffle           : " << (shuffle ? "yes" : "no")
              << std::endl;

    std::mt19937_64 generator(seed);
    std::cout << "Allocating " << nObjects << " objects of size "
              << allocationSize << "..." << std::endl;
    std::vector<Node*> objects
        = allocateObjects(policy, nObjects, allocationSize, generator);

    if (shuffle) {
        std::cout << "Shuffling iteration order..." << std::endl;
        // Keep the first address in place to use it when releasing the memory.
        std::shuffle(std::next(objects.begin()), objects.end(), generator);
    }

    std::cout << "Setting up cycle..." << std::endl;
    objects.back()->next = objects[0];
    for (std::size_t i = 0; i < nObjects - 1; ++i) {
        objects[i]->next = objects[i + 1];
    }

    std::cout << "Iterating..." << std::endl;

#ifdef ENABLE_PERF
    const auto events = std::vector<std::pair<std::uint32_t, std::uint64_t>>(
        {{PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_DTLB
                                  | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                                  | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)},
         {PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_DTLB
                                  | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                                  | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)},
         {PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_DTLB
                                  | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                                  | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)},
         {PERF_TYPE_HW_CACHE, PERF_COUNT_HW_CACHE_DTLB
                                  | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                                  | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)},
         {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES},
         {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES}});
    // FIXME: Adding the prefetch counters and more precise L1/LLC counters
    // would be nice, but not supported.
    utils::perf::Group group(events);
    group.reset();
    group.enable();
#endif
    const auto start = std::chrono::high_resolution_clock::now();

    runBenchmark(iterations, objects[0]);

    const auto end = std::chrono::high_resolution_clock::now();
#ifdef ENABLE_PERF
    group.disable();
#endif

    const auto elapsed_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << " ms." << std::endl;
#ifdef ENABLE_PERF
    const auto counts = group.read();
    for (std::size_t i = 0; i < events.size(); ++i) {
        std::cout << utils::perf::toString(events.at(i)) << " = " << counts[i]
                  << std::endl;
    }
    std::cout << "dTLB read miss rate: "
              << 100.0 * static_cast<double>(counts[0])
                     / static_cast<double>(counts[2])
              << "%" << std::endl;
    std::cout << "dTLB write miss rate: "
              << 100.0 * static_cast<double>(counts[1])
                     / static_cast<double>(counts[3])
              << "%" << std::endl;
    std::cout << "LLC miss rate: "
              << 100.0 * static_cast<double>(counts[4])
                     / static_cast<double>(counts[5])
              << "%" << std::endl;
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
            objects[0], alignUp(nObjects * allocationSize,
                                policy == "arena-mmap" ? 4096 : 1 << 21));
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
