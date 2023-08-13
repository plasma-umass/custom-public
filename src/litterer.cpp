#include <litterer/litterer.h>

#if _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

#include <litterer/constants.h>

namespace {
using Clock = std::chrono::steady_clock;

template <typename... T>
void assertOrExit(bool condition, fmt::format_string<T...> format, T&&... args) {
    if (!condition) {
        fmt::print(stderr, "[ERROR] ");
        fmt::print(stderr, format, std::forward<T>(args)...);
        fmt::print(stderr, "\n");
        exit(1);
    }
}
} // namespace

void runLitterer() {
    std::uint32_t seed = std::random_device{}();
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

    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

    std::uint32_t multiplier = 20;
    if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
        multiplier = atoi(env);
    }

    FILE* log = stderr;
    if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
        log = fopen(env, "w");
    }

    std::string dataFilename = DETECTOR_OUTPUT_FILENAME;
    if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
        dataFilename = env;
    }

    assertOrExit(std::filesystem::exists(dataFilename), "{} does not exist.", dataFilename);

    std::ifstream inputFile(dataFilename);
    nlohmann::json data;
    inputFile >> data;

#if _WIN32
    HMODULE mallocModule;
    const auto status
        = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR) &malloc, &mallocModule);
    assert(status && "Could not get malloc info.");

    char mallocFileName[MAX_PATH];
    GetModuleFileName(mallocModule, mallocFileName, MAX_PATH);
    const std::string mallocSourceObject = mallocFileName;
#else
    Dl_info mallocInfo;
    const auto status = dladdr((void*) &malloc, &mallocInfo);
    assertOrExit(status != 0, "Could not get malloc info.");
    const std::string mallocSourceObject = mallocInfo.dli_fname;
#endif

    fmt::print(log, "==================================== Litterer ====================================\n");
    fmt::print(log, "malloc     : {}\n", mallocSourceObject);
    fmt::print(log, "seed       : {}\n", seed);
    fmt::print(log, "occupancy  : {}\n", occupancy);
    fmt::print(log, "shuffle    : {}\n", shuffle ? "no" : "yes");
    fmt::print(log, "sleep      : {}\n", sleepDelay ? std::to_string(sleepDelay) : "no");
    fmt::print(log, "multiplier : {}\n", multiplier);
    fmt::print(log, "timestamp  : {} {}\n", __DATE__, __TIME__);
    fmt::print(log, "==================================================================================\n");

    const std::size_t nAllocations = data["NAllocations"].get<std::size_t>();
    const std::size_t maxLiveAllocations = data["MaxLiveAllocations"].get<std::size_t>();
    const std::size_t nAllocationsLitter = maxLiveAllocations * multiplier;

    // This can happen if no allocations were recorded.
    if (!data["Bins"].empty()) {
        if (data["Bins"][data["SizeClasses"].size()].get<int>() != 0) {
            fmt::print("[WARNING] Allocations of size greater than the maximum size class were recorded.\n");
            fmt::print("[WARNING] There will be no littering for these allocations.\n");
            fmt::print("[WARNING] This represents {:.2f}% of all allocations recorded.\n",
                       (static_cast<double>(data["Bins"][data["SizeClasses"].size()]) / nAllocations) * 100);
        }

        const auto litterStart = std::chrono::high_resolution_clock::now();

        std::uniform_int_distribution<std::size_t> distribution(
            0, nAllocations - data["Bins"][data["SizeClasses"].size()].get<int>() - 1);
        std::vector<void*> objects = *(new std::vector<void*>);
        objects.reserve(nAllocationsLitter);

        for (std::size_t i = 0; i < nAllocationsLitter; ++i) {
            std::size_t minAllocationSize = 0;
            std::size_t sizeClassIndex = 0;
            std::int64_t offset = static_cast<int64_t>(distribution(generator)) - data["Bins"][0].get<std::int64_t>();

            while (offset >= 0) {
                minAllocationSize = data["SizeClasses"][sizeClassIndex].get<std::size_t>() + 1;
                ++sizeClassIndex;
                offset -= static_cast<std::size_t>(data["Bins"][sizeClassIndex].get<int>());
            }
            const std::size_t maxAllocationSize = data["SizeClasses"][sizeClassIndex].get<std::size_t>();
            std::uniform_int_distribution<std::size_t> allocationSizeDistribution(minAllocationSize, maxAllocationSize);
            const std::size_t allocationSize = allocationSizeDistribution(generator);

            void* pointer = malloc(allocationSize);
            objects.push_back(pointer);
        }

        const std::size_t nObjectsToBeFreed = static_cast<std::size_t>((1 - occupancy) * nAllocationsLitter);

        if (shuffle) {
            for (std::size_t i = 0; i < nObjectsToBeFreed; ++i) {
                std::uniform_int_distribution<std::size_t> indexDistribution(i, objects.size() - 1);
                std::size_t index = indexDistribution(generator);
                std::swap(objects[i], objects[index]);
            }
        } else {
            std::sort(objects.begin(), objects.end(), std::greater<void*>());
        }

        for (std::size_t i = 0; i < nObjectsToBeFreed; ++i) {
            free(objects[i]);
        }

        const auto litterEnd = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>((litterEnd - litterStart));
        fmt::print(log, "Finished littering. Time taken: {} seconds.\n", elapsed.count());
    }

    if (sleepDelay) {
#ifdef WIN32
        const auto pid = GetCurrentProcessId();
#else
        const auto pid = getpid();
#endif
        fmt::print(log, "Sleeping {} seconds before resuming (PID: {})...\n", sleepDelay, pid);
        std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
        fmt::print(log, "Resuming program now!\n");
    }

    fmt::print(log, "==================================================================================\n");
}
