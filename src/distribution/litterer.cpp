#include <cassert>
#if _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include <fmt/core.h>

#include <nlohmann/json.hpp>

namespace {
void assertOrExit(bool condition, std::FILE* log, const std::string& message) {
    if (!condition) {
        fmt::println(log, "[ERROR] %s\n", message.c_str());
        exit(EXIT_FAILURE);
    }
}

template <typename T, typename Generator> void partial_shuffle(std::vector<T>& v, std::size_t n, Generator& g) {
    const auto m = std::min(n, v.size() - 2);
    for (std::size_t i = 0; i < m; ++i) {
        const auto j = std::uniform_int_distribution<std::size_t>(i, v.size() - 1)(g);
        std::swap(v[i], v[j]);
    }
}

std::vector<std::uint64_t> cumulative_sum(const std::vector<std::uint64_t>& bins) {
    std::vector<std::uint64_t> cumsum(bins.size());
    std::partial_sum(bins.begin(), bins.end(), cumsum.begin());
    return cumsum;
}
} // namespace

void runLitterer() {
    std::FILE* log = stderr;
    if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
        log = std::fopen(env, "w");
        assertOrExit(log != nullptr, log, "Could not open log file.");
    }

    std::uint32_t seed = std::random_device()();
    if (const char* env = std::getenv("LITTER_SEED")) {
        seed = std::atoi(env);
    }
    auto generator = std::mt19937_64(seed);

    double occupancy = 0.95;
    if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
        occupancy = std::atof(env);
        assertOrExit(occupancy >= 0 && occupancy <= 1, log, "Occupancy must be between 0 and 1.");
    }

    bool shuffle = true;
    if (const char* env = std::getenv("LITTER_SHUFFLE")) {
        shuffle = std::atoi(env);
    }

    bool sort = false;
    if (const char* env = std::getenv("LITTER_SORT")) {
        sort = std::atoi(env);
    }

    assertOrExit(!(shuffle && sort), log, "Select either shuffle or sort, not both.");

    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = std::atoi(env);
    }

    std::uint32_t multiplier = 20;
    if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
        multiplier = std::atoi(env);
    }

    std::string dataFilename = "distribution.json";
    if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
        dataFilename = env;
    }

    assertOrExit(std::filesystem::exists(dataFilename), log, dataFilename + " does not exist.");

    std::ifstream inputFile(dataFilename);
    nlohmann::json data;
    inputFile >> data;

#if _WIN32
    HMODULE mallocModule;
    const DWORD status
        = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR) &malloc, &mallocModule);
    assertOrExit(status, log, "Could not get malloc info.");

    char mallocFileName[MAX_PATH];
    GetModuleFileNameA(mallocModule, mallocFileName, MAX_PATH);
    const auto mallocSourceObject = std::string(mallocFileName);
#else
    Dl_info mallocInfo;
    const int status = dladdr((void*) &malloc, &mallocInfo);
    assertOrExit(status != 0, log, "Could not get malloc info.");
    const auto mallocSourceObject = std::string(mallocInfo.dli_fname);
#endif

    const auto sizeClasses = data["sizeClasses"].get<std::vector<std::size_t>>();
    const auto bins = data["bins"].get<std::vector<std::uint64_t>>();
    const auto nAllocations = data["nAllocations"].get<std::uint64_t>();
    const auto maxLiveAllocations = data["maxLiveAllocations"].get<std::int64_t>();
    const std::size_t nAllocationsLitter = maxLiveAllocations * multiplier;

    fmt::println(log, "==================================== Litterer ====================================\n");
    fmt::println(log, "malloc     : {}\n", mallocSourceObject);
    fmt::println(log, "seed       : {}\n", seed);
    fmt::println(log, "occupancy  : {}\n", occupancy);
    fmt::println(log, "shuffle    : {}\n", shuffle ? "yes" : "no");
    fmt::println(log, "sort       : {}\n", sort ? "yes" : "no");
    fmt::println(log, "sleep      : {}\n", sleepDelay ? std::to_string(sleepDelay) : "no");
    fmt::println(log, "litter     : {} * {} = {}\n", multiplier, maxLiveAllocations, nAllocationsLitter);
    fmt::println(log, "timestamp  : {} {}\n", __DATE__, __TIME__);
    fmt::println(log, "==================================================================================\n");

    assertOrExit(std::accumulate(bins.begin(), bins.end(), std::uint64_t(0)) == nAllocations, log,
                 "Invalid bin distribution.");
    const std::vector<std::uint64_t> binsCumSum = cumulative_sum(bins);

    const auto start = std::chrono::high_resolution_clock::now();

    std::uniform_int_distribution<std::uint64_t> distribution(1, nAllocations);
    std::vector<void*> objects(nAllocationsLitter);

    for (std::size_t i = 0; i < nAllocationsLitter; ++i) {
        const auto offset = distribution(generator);
        const auto it = std::lower_bound(binsCumSum.begin(), binsCumSum.end(), offset);
        const auto bin = std::distance(binsCumSum.begin(), it) + 1;
        objects[i] = std::malloc(bin);
    }

    const auto nObjectsToBeFreed = static_cast<std::size_t>((1 - occupancy) * nAllocationsLitter);

    if (shuffle) {
        fmt::println(log, "Shuffling {} object(s) to be freed.\n", nObjectsToBeFreed);
        partial_shuffle(objects, nObjectsToBeFreed, generator);
    } else if (sort) {
        fmt::println(log, "Sorting all {} objects.\n", objects.size());
        std::sort(objects.begin(), objects.end(), std::greater<void*>());
    }

    for (std::size_t i = 0; i < nObjectsToBeFreed; ++i) {
        std::free(objects[i]);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>((end - start));
    fmt::println(log, "Finished littering. Time taken: {} seconds.\n", elapsed.count());

    if (sleepDelay) {
#ifdef _WIN32
        const auto pid = GetCurrentProcessId();
#else
        const auto pid = getpid();
#endif
        fmt::println(log, "Sleeping {} seconds before resuming (PID: {})...\n", sleepDelay, pid);
        std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
        fmt::println(log, "Resuming program now!\n");
    }

    fmt::println(log, "==================================================================================\n");
    if (log != stderr) {
        fclose(log);
    }
}
