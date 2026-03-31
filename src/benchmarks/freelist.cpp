#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <random>
#include <vector>

#include <argparse/argparse.hpp>

namespace {
void* freelist = nullptr;

void* do_malloc(std::size_t size, bool useMallocFree) {
    if (useMallocFree || freelist == nullptr) {
        return malloc(size);
    }

    void* ptr = freelist;
    freelist = *static_cast<void**>(freelist);
    return ptr;
}

void do_free(void* ptr, bool useMallocFree) {
    if (useMallocFree) {
        free(ptr);
        return;
    }

    *static_cast<void**>(ptr) = freelist;
    freelist = ptr;
}
} // namespace

int main(int argc, char** argv) {
    auto program = argparse::ArgumentParser("benchmark_iterator", "",
                                            argparse::default_arguments::help);
    program.add_argument("-s", "--allocation-size")
        .default_value(std::uint64_t(16))
        .help("the size in bytes of objects")
        .metavar("SIZE")
        .scan<'u', std::uint64_t>();
    program.add_argument("-i", "--iterations")
        .default_value(std::uint64_t(100'000'000))
        .help("the number of iterations")
        .metavar("N")
        .scan<'u', std::uint64_t>();
    program.add_argument("--no-freelist")
        .help("whether to use regular malloc/free over the freelist")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(EXIT_FAILURE);
    }

    const auto allocationSize = program.get<std::uint64_t>("--allocation-size");
    const auto useMallocFree = program.get<bool>("--no-freelist");
    const auto iterations = program.get<std::uint64_t>("--iterations");

    std::cout << "Using malloc/free? " << (useMallocFree ? "yes" : "no")
              << std::endl;

    auto generator = std::mt19937(std::random_device{}());
    std::bernoulli_distribution coin(0.5);
    std::vector<void*> live;

    constexpr std::array<std::size_t, 7> kNoiseSizes
        = {8, 16, 32, 64, 128, 256, 512};
    std::deque<void*> noises;
    for (int i = 0; i < 4096; ++i) {
        noises.push_back(malloc(kNoiseSizes.at(i % kNoiseSizes.size())));
    }

    const auto start = std::chrono::high_resolution_clock::now();

    for (std::uint64_t i = 0; i < iterations; ++i) {
        noises.push_back(malloc(kNoiseSizes.at(i % kNoiseSizes.size())));

        if (live.empty() || coin(generator)) {
            live.push_back(do_malloc(allocationSize, useMallocFree));
        } else {
            do_free(live.back(), useMallocFree);
            live.pop_back();
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();

    for (void* ptr : live) {
        free(ptr);
    }
    while (freelist != nullptr) {
        void* next = *static_cast<void**>(freelist);
        free(freelist);
        freelist = next;
    }
    for (void* ptr : noises) {
        free(ptr);
    }

    const auto elapsed_ms
        = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
    std::cout << "Done. Time elapsed: " << elapsed_ms << " ms." << std::endl;
}
