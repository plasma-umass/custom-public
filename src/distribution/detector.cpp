#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)

#include <interpose.h>

namespace {
const auto* kDefaultDataFilename = "distribution.json";
const auto* kDefaultSizeClassScheme = "under-4096";

std::atomic_bool initialized = false;
std::vector<std::size_t> sizeClasses;
std::deque<std::atomic_uint64_t> bins;
std::atomic_uint64_t ignored = 0;
std::atomic_int64_t liveAllocations = 0;
std::atomic_int64_t maxLiveAllocations = 0;
std::string dataFilename;

const struct Initialization {
    Initialization() {
        dataFilename = kDefaultDataFilename;
        if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
            dataFilename = env;
        }

        if (std::getenv("LITTER_DETECTOR_APPEND") != nullptr
            && std::filesystem::exists(dataFilename)) {
            std::ifstream inputFile(dataFilename);
            nlohmann::json data; // NOLINT(misc-include-cleaner)
            inputFile >> data;

            // Note: APPEND will ignore SIZE_CLASSES, and just use the existing
            // ones.
            sizeClasses = data["sizeClasses"].get<std::vector<std::size_t>>();
            bins = std::deque<std::atomic_uint64_t>(
                data["bins"].get<std::vector<std::uint64_t>>().begin(),
                data["bins"].get<std::vector<std::uint64_t>>().end());
        } else {
            std::string sizeClassScheme = kDefaultSizeClassScheme;
            if (const char* env = std::getenv("LITTER_SIZE_CLASSES")) {
                sizeClassScheme = env;
            }

            if (sizeClassScheme == "under-4096") {
                // Will be overriden if LITTER_DETECTOR_APPEND is set.
                sizeClasses.reserve(4096);
                for (std::size_t i = 1; i <= 4096; ++i) {
                    sizeClasses.push_back(i);
                }
            } else {
                std::cerr << "Invalid size class scheme." << std::endl;
                exit(EXIT_FAILURE);
            }

            bins.resize(sizeClasses.size());
        }

        initialized = true;
    }

    ~Initialization() {
        initialized = false;

        const nlohmann::json data = // NOLINT(misc-include-cleaner)
            {
                {"sizeClasses", sizeClasses},
                {"bins", std::vector<std::uint64_t>(bins.begin(), bins.end())},
                {"maxLiveAllocations", maxLiveAllocations.load()},
                {"ignored", ignored.load()},
            };

        std::ofstream output(dataFilename);
        output << data.dump(4) << std::endl;
    }

    Initialization(const Initialization&) = delete;
    Initialization& operator=(const Initialization&) = delete;
    Initialization(Initialization&&) = delete;
    Initialization& operator=(Initialization&&) = delete;
} _;

// This function CANNOT call any memory allocation functions.
template <bool NewAlloc>
void processAllocation(std::size_t size) {
    if (size == 0) {
        return;
    }

    if (!initialized) {
        return;
    }

    if (size > sizeClasses.back()) {
        ++ignored;
    } else {
        const auto it
            = std::lower_bound(sizeClasses.begin(), sizeClasses.end(), size);
        const auto index = std::distance(sizeClasses.begin(), it);
        ++bins[index];
    }

    if constexpr (NewAlloc) {
        const auto liveAllocationsSnapshot = ++liveAllocations;
        auto maxLiveAllocationsSnapshot = maxLiveAllocations.load();
        while (liveAllocationsSnapshot > maxLiveAllocationsSnapshot) {
            maxLiveAllocations.compare_exchange_weak(maxLiveAllocationsSnapshot,
                                                     liveAllocationsSnapshot);
            maxLiveAllocationsSnapshot = maxLiveAllocations.load();
        }
    }
}

void processFree(void* pointer) {
    if (pointer == nullptr) {
        return;
    }

    --liveAllocations;
}
} // namespace

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(size_t size) {
    static auto* next = GET_REAL_FUNCTION(malloc);
    processAllocation<true>(size);
    return next(size);
}
INTERPOSE(malloc);

extern "C" void INTERPOSE_FUNCTION_NAME(free)(void* pointer) {
    static auto* next = GET_REAL_FUNCTION(free);
    processFree(pointer);
    next(pointer);
}
INTERPOSE(free);

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(size_t n, size_t size) {
    static auto* next = GET_REAL_FUNCTION(calloc);
    processAllocation<true>(n * size);
    return next(n, size);
}
INTERPOSE(calloc);

extern "C" void* INTERPOSE_FUNCTION_NAME(realloc)(void* pointer, size_t size) {
    static auto* next = GET_REAL_FUNCTION(realloc);
    processAllocation<false>(size);
    return next(pointer, size);
}
INTERPOSE(realloc);

#ifndef __APPLE__
extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer, size_t n,
                                                       size_t size) {
    static auto* next = GET_REAL_FUNCTION(reallocarray);
    processAllocation<false>(n * size);
    return next(pointer, n, size);
}
INTERPOSE(reallocarray);
#endif

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr,
                                                       size_t alignment,
                                                       size_t size) {
    static auto* next = GET_REAL_FUNCTION(posix_memalign);
    processAllocation<true>(size);
    return next(memptr, alignment, size);
}
INTERPOSE(posix_memalign);

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(size_t alignment,
                                                        size_t size) {
    static auto* next = GET_REAL_FUNCTION(aligned_alloc);
    processAllocation<true>(size);
    return next(alignment, size);
}
INTERPOSE(aligned_alloc);
