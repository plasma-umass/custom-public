#include <sys/mman.h>

#include <algorithm>
#include <array>
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

#ifdef ENABLE_PERF
#include <linux/perf_event.h>
#include <utils/perf.hpp>
#endif

namespace {
std::size_t align(std::size_t size, std::size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

std::vector<void*> allocatePages(std::size_t n, bool useHugepages) {
    std::vector<void*> objects;
    objects.reserve(n);

    if (useHugepages) {

        // Careful for hugepages, this will be auto-aligned to multiple of
        // underlying page size. So care when freeing.

        // For munmap(), addr, and length must both be a multiple of the
        //       underlying huge page size.
        std::abort();
    }

    auto* allocation = reinterpret_cast<std::uint8_t*>(
        mmap(nullptr, n * 4096, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < 4096; ++j) {
            allocation[i * 4096 + j] = 13;
        }
    }

    for (std::size_t i = 0; i < n; ++i) {
        objects.push_back(reinterpret_cast<void*>(allocation + i * 4096));
    }

    return objects;
};

void runBenchmark(std::uint64_t iterations, std::vector<void*>& objects,
                  const std::vector<std::size_t>& offsets) {
    const auto nObjects = objects.size();
    for (std::uint64_t i = 0; i < iterations; ++i) {
        reinterpret_cast<std::uint8_t*>(
            objects[i % nObjects])[offsets[i % nObjects]]
            = 13;
    }
}
} // namespace

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "",
                                            argparse::default_arguments::help);
    program.add_argument("-n", "--number-pages")
        .required()
        .help("the number of pages to be allocated")
        .metavar("N")
        .scan<'d', std::uint64_t>();
    program.add_argument("-i", "--iterations")
        .required()
        .metavar("N")
        .scan<'d', std::uint64_t>();
#if defined(__linux__)
    program.add_argument("--use-hugepages")
        .help("use hugepages for the memory allocation")
        .default_value(false)
        .implicit_value(true);
#endif
    program.add_argument("--seed")
        .help("initial seed for the random number generator")
        .default_value(std::random_device()())
        .scan<'d', unsigned int>();
    program.add_argument("--output-csv")
        .help("quiet output, pastable into Excel/Sheets")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    const auto nPages = program.get<std::uint64_t>("--number-pages");
    const auto iterations = program.get<std::uint64_t>("--iterations");
#if defined(__linux__)
    const auto useHugepages = program.get<bool>("--use-hugepages");
#else
    const auto useHugepages = false;
#endif
    const auto seed = program.get<unsigned int>("seed");
    const auto outputCsv = program.get<bool>("--output-csv");

    if (nPages == 0) {
        std::cerr << "The number of pages must be greater than zero."
                  << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (!outputCsv) {
        std::cout << "number of pages : " << nPages << std::endl;
        std::cout << "iterations      : " << iterations << std::endl;
        std::cout << "use hugepages   : " << std::boolalpha << useHugepages
                  << std::endl;
        std::cout << "seed            : " << seed << std::endl;
    }

    std::vector<void*> pages = allocatePages(nPages, useHugepages);

    if (!outputCsv) {
        std::cout << "Shuffling iteration order..." << std::endl;
    }
    std::mt19937_64 generator(seed);
    // Keep the first address in place to use it when releasing the memory.
    std::shuffle(std::next(pages.begin()), pages.end(), generator);

    // Generate random offsets to avoid prefetching or collisions.
    std::uniform_int_distribution<std::size_t> distribution(0, 4095);
    std::vector<std::size_t> offsets;
    offsets.reserve(nPages);
    for (std::size_t i = 0; i < nPages; ++i) {
        offsets.push_back(distribution(generator));
    }

    if (!outputCsv) {
        std::cout << "Iterating..." << std::endl;
    }

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

    runBenchmark(iterations, pages, offsets);

    const auto end = std::chrono::high_resolution_clock::now();
#ifdef ENABLE_PERF
    group.disable();
#endif

    const auto elapsed_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();

    if (outputCsv) {
        std::cout << nPages << ";" << elapsed_ms;
    } else {
        std::cout << "Done. Time elapsed: " << elapsed_ms << " ms."
                  << std::endl;
    }
#ifdef ENABLE_PERF
    const auto counts = group.read();
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (outputCsv) {
            std::cout << ";" << counts[i];
        } else {
            std::cout << utils::perf::toString(events.at(i)) << " = "
                      << counts[i] << std::endl;
        }
    }

    if (outputCsv) {
        std::cout << std::endl;
    } else {
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
    }
#endif

    const auto status = munmap(
        pages[0], useHugepages ? align(nPages * 4096, 1 << 21) : nPages * 4096);
    if (status != 0) {
        std::cerr << "Failed to free memory..." << std::endl;
        std::cerr << std::strerror(errno) << std::endl;
        std::abort();
    }
}
