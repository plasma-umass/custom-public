#include <litterer/litterer.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

using Clock = std::chrono::steady_clock;

template <typename... T> void assertOrExit(bool condition, fmt::format_string<T...> format, T&&... args) {
    if (!condition) {
        fmt::print(stderr, "[ERROR] {}\n", format, args...);
        exit(1);
    }
}

extern "C" ATTRIBUTE_EXPORT void runLitterer() {
    uint32_t seed = std::random_device{}();
    if (const char* env = std::getenv("LITTER_SEED")) {
        seed = atoi(env);
    }
    std::mt19937_64 generator(seed);

    double occupancy = 0.95;
    if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
        occupancy = atof(env);
        assertOrExit(occupancy >= 0 && occupancy <= 1, "Occupancy must be between 0 and 1.");
    }

    bool shuffle = true;
    if (const char* env = std::getenv("LITTER_SHUFFLE")) {
        shuffle = atoi(env) == 0;
    }

    uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

    uint32_t multiplier = 20;
    if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
        multiplier = atoi(env);
    }

    FILE* log = stderr;
    if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
        log = fopen(env, "w");
    }

    assertOrExit(std::filesystem::exists(DETECTOR_OUTPUT_FILENAME), "{} does not exist.", DETECTOR_OUTPUT_FILENAME);

    std::ifstream inputFile(DETECTOR_OUTPUT_FILENAME);
    nlohmann::json data;
    inputFile >> data;

    Dl_info mallocInfo;
    auto status = dladdr((void*) &malloc, &mallocInfo);
    assert(status != 0);

    fmt::print(log, "==================================== Litterer ====================================\n");
    fmt::print(log, "malloc     : {}\n", mallocInfo.dli_fname);
    fmt::print(log, "seed       : {}\n", seed);
    fmt::print(log, "occupancy  : {}\n", occupancy);
    fmt::print(log, "shuffle    : {}\n", shuffle ? "no" : "yes");
    fmt::print(log, "sleep      : {}\n", sleepDelay ? std::to_string(sleepDelay) : "no");
    fmt::print(log, "multiplier : {}\n", multiplier);
    fmt::print(log, "timestamp  : {} {}\n", __DATE__, __TIME__);
    fmt::print(log, "==================================================================================\n");

    long long int nAllocations = data["NAllocations"].get<long long int>();
    long long int maxLiveAllocations = data["MaxLiveAllocations"].get<long long int>();
    long long int nAllocationsLitter = maxLiveAllocations * multiplier;

    // This can happen if no allocations were recorded.
    if (!data["Bins"].empty()) {
        if (data["Bins"][data["SizeClasses"].size()].get<int>() != 0) {
            fmt::print("[WARNING] Allocations of size greater than the maximum size class were recorded.\n");
            fmt::print("[WARNING] There will be no littering for these allocations.\n");
            fmt::print("[WARNING] This represents {:.2f}% of all allocations recorded.\n",
                       (static_cast<double>(data["Bins"][data["SizeClasses"].size()]) / nAllocations) * 100);
        }

        std::chrono::high_resolution_clock::time_point litterStart = std::chrono::high_resolution_clock::now();

        std::uniform_int_distribution<long long int> distribution(
            0, nAllocations - data["Bins"][data["SizeClasses"].size()].get<int>() - 1);
        std::vector<void*> objects = *(new std::vector<void*>);
        objects.reserve(nAllocationsLitter);

        for (long long int i = 0; i < nAllocationsLitter; ++i) {
            size_t minAllocationSize = 0;
            size_t sizeClassIndex = 0;
            long long int offset = distribution(generator) - (long long int) data["Bins"][0].get<int>();

            while (offset >= 0LL) {
                minAllocationSize = data["SizeClasses"][sizeClassIndex].get<size_t>() + 1;
                ++sizeClassIndex;
                offset -= (long long int) data["Bins"][sizeClassIndex].get<int>();
            }
            size_t maxAllocationSize = data["SizeClasses"][sizeClassIndex].get<size_t>();
            std::uniform_int_distribution<size_t> allocationSizeDistribution(minAllocationSize, maxAllocationSize);
            size_t allocationSize = allocationSizeDistribution(generator);

            void* pointer = malloc(allocationSize);
            objects.push_back(pointer);
        }

        long long int nObjectsToBeFreed = (1 - occupancy) * nAllocationsLitter;

        if (shuffle) {
            for (int i = 0; i < nObjectsToBeFreed; ++i) {
                std::uniform_int_distribution<int> indexDistribution(i, objects.size() - 1);
                int index = indexDistribution(generator);
                std::swap(objects[i], objects[index]);
            }
        } else {
            std::sort(objects.begin(), objects.end(), std::greater<void*>());
        }

        for (long long int i = 0; i < nObjectsToBeFreed; ++i) {
            free(objects[i]);
        }

        std::chrono::high_resolution_clock::time_point litterEnd = std::chrono::high_resolution_clock::now();
        std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>((litterEnd - litterStart));
        fmt::print(log, "Finished littering. Time taken: {} seconds.\n", elapsed.count());
    }

    if (sleepDelay) {
        fmt::print(log, "Sleeping {} seconds before resuming (PID: {})...\n", sleepDelay, getpid());
        sleep(sleepDelay);
        fmt::print(log, "Resuming program now!\n");
    }

    fmt::print(log, "==================================================================================\n");
}
