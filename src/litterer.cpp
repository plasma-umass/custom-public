#include <litterer/litterer.h>
#include <numeric>

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
#include <functional>
#include <iterator>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#define MALLOC ::malloc
#define FREE ::free

namespace {
using Clock = std::chrono::steady_clock;

template <typename... T>
void assertOrExit(bool condition, FILE* log, const std::string& message) {
    if (!condition) {
        fprintf(log, "[ERROR] %s\n", message.c_str());
        exit(EXIT_FAILURE);
    }
}

template <typename T, typename Generator>
void partial_shuffle(std::vector<T>& v, std::size_t n, Generator& g) {
    const auto m = std::min(n, v.size() - 2);
    for (std::size_t i = 0; i < m; ++i) {
        const auto j = std::uniform_int_distribution<std::size_t>(i, v.size() - 1)(g);
        std::swap(v[i], v[j]);
    }
}

std::vector<size_t> cumulative_sum(const std::vector<size_t>& bins) {
    std::vector<size_t> cumsum(bins.size());
    std::partial_sum(bins.begin(), bins.end(), cumsum.begin());
    return cumsum;
}
} // namespace

void runLitterer() {
    FILE* log = stderr;
    if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
        log = fopen(env, "w");
    }

    std::uint32_t seed = std::random_device{}();
    if (const char* env = std::getenv("LITTER_SEED")) {
        seed = atoi(env);
    }
    std::mt19937_64 generator(seed);

    double occupancy = 0.95;
    if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
        occupancy = atof(env);
        assertOrExit(occupancy >= 0 && occupancy <= 1, log, "Occupancy must be between 0 and 1.");
    }

    bool shuffle = true;
    if (const char* env = std::getenv("LITTER_SHUFFLE")) {
        shuffle = atoi(env);
    }

    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

    std::uint32_t multiplier = 20;
    if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
        multiplier = atoi(env);
    }

    std::string dataFilename = "detector.out";
    if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
        dataFilename = env;
    }

    assertOrExit(std::filesystem::exists(dataFilename), log, dataFilename + " does not exist.");

    std::ifstream inputFile(dataFilename);
    nlohmann::json data;
    inputFile >> data;

#if _WIN32
    HMODULE mallocModule;
    const auto status
        = GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR) &malloc, &mallocModule);
    assertOrExit(status, log, "Could not get malloc info.");

    char mallocFileName[MAX_PATH];
    GetModuleFileNameA(mallocModule, mallocFileName, MAX_PATH);
    const std::string mallocSourceObject = mallocFileName;
#else
    Dl_info mallocInfo;
    const auto status = dladdr((void*) &malloc, &mallocInfo);
    assertOrExit(status != 0, log, "Could not get malloc info.");
    const std::string mallocSourceObject = mallocInfo.dli_fname;
#endif

    const auto bins = data["Bins"].get<std::vector<std::size_t>>();
    const auto nAllocations = data["NAllocations"].get<std::size_t>();
    const auto maxLiveAllocations = data["MaxLiveAllocations"].get<std::size_t>();
    const std::size_t nAllocationsLitter = maxLiveAllocations * multiplier;

    fprintf(log, "==================================== Litterer ====================================\n");
    fprintf(log, "malloc     : %s\n", mallocSourceObject.c_str());
    fprintf(log, "seed       : %u\n", seed);
    fprintf(log, "occupancy  : %f\n", occupancy);
    fprintf(log, "shuffle    : %s\n", shuffle ? "yes" : "no");
    fprintf(log, "sleep      : %s\n", sleepDelay ? std::to_string(sleepDelay).c_str() : "no");
    fprintf(log, "litter     : %u * %zu = %zu\n", multiplier, maxLiveAllocations, nAllocationsLitter);
    fprintf(log, "timestamp  : %s %s\n", __DATE__, __TIME__);
    fprintf(log, "==================================================================================\n");

    assert(std::accumulate(bins.begin(), bins.end(), 0zu) == nAllocations);
    const std::vector<std::size_t> binsCumSum = cumulative_sum(bins);

    const auto litterStart = std::chrono::high_resolution_clock::now();

    std::uniform_int_distribution<std::int64_t> distribution(1, nAllocations);
    std::vector<void*> objects = *(new std::vector<void*>);
    objects.reserve(nAllocationsLitter);

    for (std::size_t i = 0; i < nAllocationsLitter; ++i) {
        const auto offset = distribution(generator);
        const auto it = std::lower_bound(binsCumSum.begin(), binsCumSum.end(), offset);
        assert(it != binsCumSum.end());
        const auto bin = std::distance(binsCumSum.begin(), it) + 1;
        void* pointer = MALLOC(bin);
        objects.push_back(pointer);
    }

    const std::size_t nObjectsToBeFreed = static_cast<std::size_t>((1 - occupancy) * nAllocationsLitter);

    if (shuffle) {
        fprintf(log, "Shuffling %zu object(s) to be freed.\n", nObjectsToBeFreed);
        partial_shuffle(objects, nObjectsToBeFreed, generator);
    } else {
        // TODO: We should maybe make this a third-option.
        std::sort(objects.begin(), objects.end(), std::greater<void*>());
    }

    for (std::size_t i = 0; i < nObjectsToBeFreed; ++i) {
        FREE(objects[i]);
    }

    const auto litterEnd = std::chrono::high_resolution_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>((litterEnd - litterStart));
    fprintf(log, "Finished littering. Time taken: %s seconds.\n", std::to_string(elapsed.count()).c_str());

    if (sleepDelay) {
#ifdef _WIN32
        const auto pid = GetCurrentProcessId();
#else
        const auto pid = getpid();
#endif
        fprintf(log, "Sleeping %u seconds before resuming (PID: %d)...\n", sleepDelay, pid);
        std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
        fprintf(log, "Resuming program now!\n");
    }

    fprintf(log, "==================================================================================\n");
    if (log != stderr) {
        fclose(log);
    }
}
