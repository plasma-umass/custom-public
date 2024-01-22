#include <dlfcn.h>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <static_vector.hpp>

namespace {
const auto* kDefaultOutputFilename = "distribution.json";
const auto* kDefaultSizeClassScheme = "under-4096";
const auto kDefaultEnableOverflowBin = false;

std::atomic_bool initialized = false;
thread_local auto busy = 0;
auto enableOverflowBin = kDefaultEnableOverflowBin;
std::vector<std::size_t> sizeClasses;
static_vector<std::atomic_uint64_t> bins;
std::atomic_int64_t liveAllocations = 0;
std::atomic_int64_t maxLiveAllocations = 0;

struct Initialization {
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

        bins.reset(sizeClasses.size() + (enableOverflowBin ? 1 : 0));

        initialized = true;
    }

    ~Initialization() {
        initialized = false;

        std::string outputFilename = kDefaultOutputFilename;
        if (const char* env = std::getenv("LITTER_DATA_FILENAME")) {
            outputFilename = env;
        }

        nlohmann::json data = {
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
};

const Initialization _;
} // namespace

template <bool NewAlloc> void processAllocation(std::size_t size) {
    if (size == 0) {
        return;
    }

    if (busy || !initialized) {
        return;
    }

    ++busy;

    if (size > sizeClasses.back()) {
        if (enableOverflowBin) {
            ++bins.back();
        }
    } else {
        const auto it = std::lower_bound(sizeClasses.begin(), sizeClasses.end(), size);
        const auto index = std::distance(sizeClasses.begin(), it);
        ++bins[index];
    }

    if constexpr (NewAlloc) {
        const auto liveAllocationsSnapshot = ++liveAllocations;
        auto maxLiveAllocationsSnapshot = maxLiveAllocations.load();
        while (liveAllocationsSnapshot > maxLiveAllocationsSnapshot) {
            maxLiveAllocations.compare_exchange_weak(maxLiveAllocationsSnapshot, liveAllocationsSnapshot);
            maxLiveAllocationsSnapshot = maxLiveAllocations.load();
        }
    }

    --busy;
}

void processFree(void* pointer) {
    if (pointer == nullptr) {
        return;
    }

    ++busy;
    --liveAllocations;
    --busy;
}

extern "C" void* malloc(size_t size) {
    static auto* next = reinterpret_cast<decltype(malloc)*>(dlsym(RTLD_NEXT, "malloc"));
    processAllocation<true>(size);
    return next(size);
}

extern "C" void free(void* pointer) {
    static auto* next = reinterpret_cast<decltype(free)*>(dlsym(RTLD_NEXT, "free"));
    processFree(pointer);
    next(pointer);
}

extern "C" void* calloc(size_t n, size_t size) {
    static auto* next = reinterpret_cast<decltype(calloc)*>(dlsym(RTLD_NEXT, "calloc"));
    processAllocation<true>(n * size);
    return next(n, size);
}

extern "C" void* realloc(void* pointer, size_t size) {
    static auto* next = reinterpret_cast<decltype(realloc)*>(dlsym(RTLD_NEXT, "realloc"));
    processAllocation<false>(size);
    return next(pointer, size);
}

extern "C" void* reallocarray(void* pointer, size_t n, size_t size) {
    static auto* next = reinterpret_cast<decltype(reallocarray)*>(dlsym(RTLD_NEXT, "reallocarray"));
    processAllocation<false>(n * size);
    return next(pointer, n, size);
}

extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size) {
    static auto* next = reinterpret_cast<decltype(posix_memalign)*>(dlsym(RTLD_NEXT, "posix_memalign"));
    processAllocation<true>(size);
    return next(memptr, alignment, size);
}

extern "C" void* aligned_alloc(size_t alignment, size_t size) {
    static auto* next = reinterpret_cast<decltype(aligned_alloc)*>(dlsym(RTLD_NEXT, "aligned_alloc"));
    processAllocation<true>(size);
    return next(alignment, size);
}
