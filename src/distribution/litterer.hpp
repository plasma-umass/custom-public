#ifndef DISTRIBUTION_LITTERER_HPP
#define DISTRIBUTION_LITTERER_HPP

#include <dlfcn.h>
#include <sys/syscall.h>
#include <unistd.h>

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

#include "json.hpp"

namespace distribution::litterer {

namespace detail {
void assertOrExit(bool condition, std::FILE* log, const std::string& message) {
    if (!condition) {
        std::fprintf(log, "[ERROR] %s\n", message.c_str());
        exit(EXIT_FAILURE);
    }
}

template <typename T, typename Generator>
void partial_shuffle(std::vector<T>& v, std::size_t n, Generator& g) {
    const auto m = std::min(n, v.size() - 2);
    for (std::size_t i = 0; i < m; ++i) {
        const auto j
            = std::uniform_int_distribution<std::size_t>(i, v.size() - 1)(g);
        std::swap(v[i], v[j]);
    }
}

std::vector<std::uint64_t>
cumulative_sum(const std::vector<std::uint64_t>& bins) {
    std::vector<std::uint64_t> cumsum(bins.size());
    std::partial_sum(bins.begin(), bins.end(), cumsum.begin());
    return cumsum;
}
} // namespace detail

void runLitterer() {
    std::FILE* log = stderr;
    if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
        log = std::fopen(env, "a");
        detail::assertOrExit(log != nullptr, log, "Could not open log file.");
    }

    std::uint32_t seed = std::random_device()();
    if (const char* env = std::getenv("LITTER_SEED")) {
        seed = std::atoi(env);
    }
    auto generator = std::mt19937_64(seed);

    double occupancy = 0.95;
    if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
        occupancy = std::atof(env);
        detail::assertOrExit(occupancy >= 0 && occupancy <= 1, log,
                             "Occupancy must be between 0 and 1.");
    }

    bool shuffle = true;
    if (const char* env = std::getenv("LITTER_SHUFFLE")) {
        shuffle = std::atoi(env) != 0;
    }

    bool sort = false;
    if (const char* env = std::getenv("LITTER_SORT")) {
        sort = std::atoi(env) != 0;
    }

    detail::assertOrExit(!(shuffle && sort), log,
                         "Select either shuffle or sort, not both.");

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

    detail::assertOrExit(std::filesystem::exists(dataFilename), log,
                         dataFilename + " does not exist.");

    std::ifstream inputFile(dataFilename);
    nlohmann::json data; // NOLINT(misc-include-cleaner)
    inputFile >> data;

    Dl_info mallocInfo;
    const int status = dladdr(reinterpret_cast<void*>(&malloc), &mallocInfo);
    detail::assertOrExit(status != 0, log, "Could not get malloc info.");
    const auto mallocSourceObject = std::string(mallocInfo.dli_fname);

    const auto sizeClasses
        = data["sizeClasses"].get<std::vector<std::size_t>>();
    const auto bins = data["bins"].get<std::vector<std::uint64_t>>();
    const auto maxLiveAllocations
        = data["maxLiveAllocations"].get<std::int64_t>();
    const auto nAllocations
        = std::accumulate(bins.begin(), bins.end(), std::uint64_t(0));
    const std::size_t nAllocationsLitter = maxLiveAllocations * multiplier;

    std::fprintf(log, "==================================== Litterer "
                      "====================================\n");
    std::fprintf(log, "malloc     : %s\n", mallocSourceObject.c_str());
    std::fprintf(log, "seed       : %u\n", seed);
    std::fprintf(log, "occupancy  : %f\n", occupancy);
    std::fprintf(log, "shuffle    : %s\n", shuffle ? "yes" : "no");
    std::fprintf(log, "sort       : %s\n", sort ? "yes" : "no");
    std::fprintf(log, "sleep      : %s\n",
                 sleepDelay != 0 ? std::to_string(sleepDelay).c_str() : "no");
    std::fprintf(log, "litter     : %u * %ld = %zu\n", multiplier,
                 maxLiveAllocations, nAllocationsLitter);
    std::fprintf(log, "========================================================"
                      "==========================\n");

    const std::vector<std::uint64_t> binsCumSum = detail::cumulative_sum(bins);

    const auto start = std::chrono::high_resolution_clock::now();

    std::uniform_int_distribution<std::uint64_t> distribution(1, nAllocations);
    std::vector<void*> objects(nAllocationsLitter);

    for (std::size_t i = 0; i < nAllocationsLitter; ++i) {
        const auto offset = distribution(generator);
        const auto it
            = std::lower_bound(binsCumSum.begin(), binsCumSum.end(), offset);
        const auto bin = std::distance(binsCumSum.begin(), it);
        objects[i] = std::malloc(sizeClasses.at(bin));
    }

    const auto nObjectsToBeFreed = static_cast<std::size_t>(
        (1 - occupancy) * static_cast<double>(nAllocationsLitter));

    if (shuffle) {
        std::fprintf(log, "Shuffling %zu object(s) to be freed.\n",
                     nObjectsToBeFreed);
        detail::partial_shuffle(objects, nObjectsToBeFreed, generator);
    } else if (sort) {
        std::fprintf(log, "Sorting all %zu objects.\n", objects.size());
        std::sort(objects.begin(), objects.end(), std::greater<>());
    }

    for (std::size_t i = 0; i < nObjectsToBeFreed; ++i) {
        std::free(objects[i]);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto elapsed_ms
        = std::chrono::duration_cast<std::chrono::seconds>((end - start))
              .count();
    std::fprintf(log, "Finished littering. Time taken: %ld seconds.\n",
                 elapsed_ms);

    if (sleepDelay != 0) {
        std::fprintf(log, "Sleeping %u seconds before resuming...\n",
                     sleepDelay);
        std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
        std::fprintf(log, "Resuming program now!\n");
    }

    std::fprintf(log, "========================================================"
                      "==========================\n");
    if (log != stderr) {
        std::fclose(log);
    }

    // A marker syscall to inform any instrumentation that littering is done.
    syscall(SYS_getpid);
}
} // namespace distribution::litterer

#endif
