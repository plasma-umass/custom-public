#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)

#include <interpose.h>

namespace {
const auto* kDefaultOutputFilename = "distribution.json";
const auto* kDefaultSizeClassScheme = "under-4096";
const auto kDefaultEnableOverflowBin = false;

std::atomic_bool initialized = false;
auto enableOverflowBin = kDefaultEnableOverflowBin;
std::vector<std::size_t> sizeClasses;
std::deque<std::atomic_uint64_t> bins;
std::atomic_int64_t liveAllocations = 0;
std::atomic_int64_t maxLiveAllocations = 0;

const struct Initialization {
    Initialization() {
        std::string sizeClassScheme = kDefaultSizeClassScheme;
        if (const char* env = std::getenv("LITTER_SIZE_CLASSES")) {
            sizeClassScheme = env;
        }

        if (const char* env = std::getenv("LITTER_OVERFLOW_BIN")) {
            enableOverflowBin = std::atoi(env) != 0;
        }

        if (sizeClassScheme == "under-4096") {
            sizeClasses.reserve(4096);
            for (std::size_t i = 1; i <= 4096; ++i) {
                sizeClasses.push_back(i);
            }
        } else {
            std::cerr << "Invalid size class scheme." << std::endl;
            exit(EXIT_FAILURE);
        }

        bins.resize(sizeClasses.size() + (enableOverflowBin ? 1 : 0));

        initialized = true;
    }

    ~Initialization() {
        initialized = false;

        std::string outputFilename = kDefaultOutputFilename;
        if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
            outputFilename = env;
        }

        const nlohmann::json data = // NOLINT(misc-include-cleaner)
            {
                {"sizeClasses", sizeClasses},
                {"bins", std::vector<std::uint64_t>(bins.begin(), bins.end())},
                {"maxLiveAllocations", maxLiveAllocations.load()},
            };

        std::ofstream output(outputFilename);
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
        if (enableOverflowBin) {
            ++bins.back();
        }
    } else {
        const auto it = std::ranges::lower_bound(sizeClasses, size);
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
