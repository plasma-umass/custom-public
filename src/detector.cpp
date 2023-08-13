#include <atomic>
#include <cstddef>
#include <fstream>
#include <vector>

#include <mimalloc.h>

#include <litterer/constants.h>

#ifndef SIZE_CLASSES
// See http://jemalloc.net/jemalloc.3.html, up to 64MiB.
#define JEMALLOC_SIZE_CLASSES                                                                                          \
    {                                                                                                                  \
        8, 16, 32, 48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640, 768, 896, 1024, 1280, 1536,  \
            1792, 2048, 2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384, 20480, 24576,      \
            28672, 32768, 40960, 49152, 57344, 65536, 81920, 98304, 114688, 131072, 163840, 196608, 229376, 262144,    \
            327680, 393216, 458752, 524288, 655360, 786432, 917504, 1048576, 1310720, 1572864, 1835008, 2097152,       \
            2621440, 3145728, 3670016, 4194304, 5242880, 6291456, 7340032, 8388608, 10485760, 12582912, 14680064,      \
            16777216, 20971520, 25165824, 29360128, 33554432, 41943040, 50331648, 58720256, 67108864                   \
    }
#define SIZE_CLASSES JEMALLOC_SIZE_CLASSES
#endif

namespace {
static std::atomic_bool ready{false};
static thread_local int busy{0};

static const std::vector<std::size_t> sizeClasses = SIZE_CLASSES;
static std::vector<std::atomic_int> bins(sizeClasses.size() + 1);

static std::atomic_uint64_t nAllocations{0};
static std::atomic<double> average{0};

// Increments by 1 on malloc/calloc, and decrements by 1 on free.
static std::atomic_int64_t liveAllocations{0};
static std::atomic_int64_t maxLiveAllocations{0};

template <bool addToTotal>
void processAllocation(std::size_t size) {
    // Update average.
    average = average + (size - average) / (nAllocations + 1);
    nAllocations++;

    // Increment histogram entry.
    std::size_t index = 0;
    while (size > sizeClasses[index] && index < sizeClasses.size()) {
        ++index;
    }
    bins[index]++;

    if constexpr (addToTotal) {
        // Increment total live allocations and possibly update maximum.
        std::int64_t liveAllocationsSnapshot = liveAllocations.fetch_add(1) + 1;
        std::int64_t maxLiveAllocationsSnapshot = maxLiveAllocations;
        while (liveAllocationsSnapshot > maxLiveAllocationsSnapshot) {
            maxLiveAllocations.compare_exchange_weak(maxLiveAllocationsSnapshot, liveAllocationsSnapshot);
            maxLiveAllocationsSnapshot = maxLiveAllocations;
        }
    }
}

class Initialization {
  public:
    Initialization() {
        ready = true;
    }

    ~Initialization() {
        ready = false;

        std::ofstream outputFile(DETECTOR_OUTPUT_FILENAME);

        outputFile << "{" << std::endl;

        if (sizeClasses.size() == 0) {
            outputFile << "\t\"SizeClasses\": []," << std::endl;
        } else {
            outputFile << "\t\"SizeClasses\": [ " << sizeClasses[0];
            for (std::size_t i = 1; i < sizeClasses.size(); ++i) {
                outputFile << ", " << sizeClasses[i];
            }
            outputFile << " ]," << std::endl;
        }

        outputFile << "\t\"Bins\": [ " << bins[0];
        for (std::size_t i = 1; i < bins.size(); ++i) {
            outputFile << ", " << bins[i];
        }
        outputFile << "]," << std::endl;

        outputFile << "\t\"NAllocations\": " << nAllocations << ", \"Average\": " << average
                   << ", \"MaxLiveAllocations\": " << maxLiveAllocations << std::endl;
        outputFile << "}" << std::endl;
    }
};

static Initialization _;
} // namespace

extern "C" void* malloc(std::size_t size) {
    void* pointer = mi_malloc(size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return pointer;
}

extern "C" void free(void* pointer) {
    if (!busy && ready) {
        ++busy;
        liveAllocations--;
        --busy;
    }

    mi_free(pointer);
}

extern "C" void* calloc(std::size_t nmemb, std::size_t size) {
    void* pointer = mi_calloc(nmemb, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(nmemb * size);
        --busy;
    }

    return pointer;
}

extern "C" void* realloc(void* ptr, std::size_t size) {
    void* pointer = mi_realloc(ptr, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<false>(size);
        --busy;
    }

    return pointer;
}

extern "C" void* reallocarray(void* ptr, std::size_t nmemb, std::size_t size) {
    void* pointer = mi_reallocarray(ptr, nmemb, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<false>(nmemb * size);
        --busy;
    }

    return pointer;
}

extern "C" int posix_memalign(void** memptr, std::size_t alignment, std::size_t size) {
    int result = mi_posix_memalign(memptr, alignment, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return result;
}

extern "C" void* aligned_alloc(std::size_t alignment, std::size_t size) {
    void* pointer = mi_aligned_alloc(alignment, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return pointer;
}
