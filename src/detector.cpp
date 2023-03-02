#include <atomic>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <vector>

#include <litterer/constants.h>

static std::atomic_bool ready{false};
static thread_local int busy{0};
static thread_local bool in_dlsym{false};

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

static const std::vector<size_t> sizeClasses = SIZE_CLASSES;
static std::vector<std::atomic_int> bins(sizeClasses.size() + 1);

static std::atomic_uint64_t nAllocations{0};
static std::atomic<double> average{0};

// Increments by 1 on malloc/calloc, and decrements by 1 on free.
static std::atomic_int64_t liveAllocations{0};
static std::atomic_int64_t maxLiveAllocations{0};

class Initialization {
  public:
    Initialization() { ready = true; }

    ~Initialization() {
        ready = false;

        std::ofstream outputFile(DETECTOR_OUTPUT_FILENAME);

        outputFile << "{" << std::endl;

        if (sizeClasses.size() == 0) {
            outputFile << "\t\"SizeClasses\": []," << std::endl;
        } else {
            outputFile << "\t\"SizeClasses\": [ " << sizeClasses[0];
            for (size_t i = 1; i < sizeClasses.size(); ++i) {
                outputFile << ", " << sizeClasses[i];
            }
            outputFile << " ]," << std::endl;
        }

        outputFile << "\t\"Bins\": [ " << bins[0];
        for (size_t i = 1; i < bins.size(); ++i) {
            outputFile << ", " << bins[i];
        }
        outputFile << "]," << std::endl;

        outputFile << "\t\"NAllocations\": " << nAllocations << ", \"Average\": " << average
                   << ", \"MaxLiveAllocations\": " << maxLiveAllocations << std::endl;
        outputFile << "}" << std::endl;
    }
};

static Initialization _;

static void* local_dlsym(void* handle, const char* symbol) {
    in_dlsym = true;
    auto result = dlsym(handle, symbol);
    in_dlsym = false;
    return result;
}

extern "C" ATTRIBUTE_EXPORT void* malloc(size_t size) noexcept {
    if (in_dlsym) {
        return nullptr;
    }
    static decltype(malloc)* nextMalloc = (decltype(malloc)*) local_dlsym(RTLD_NEXT, "malloc");

    void* pointer = nextMalloc(size);

    if (!busy && ready) {
        ++busy;

        size_t index = 0;
        while (size > sizeClasses[index] && index < sizeClasses.size()) {
            ++index;
        }
        bins[index]++;

        average = average + (size - average) / (nAllocations + 1);
        nAllocations++;

        long int liveAllocationsSnapshot = liveAllocations.fetch_add(1) + 1;
        long int maxLiveAllocationsSnapshot = maxLiveAllocations;
        while (liveAllocationsSnapshot > maxLiveAllocationsSnapshot) {
            maxLiveAllocations.compare_exchange_weak(maxLiveAllocationsSnapshot, liveAllocationsSnapshot);
            maxLiveAllocationsSnapshot = maxLiveAllocations;
        }

        --busy;
    }

    return pointer;
}

extern "C" ATTRIBUTE_EXPORT void* calloc(size_t n, size_t size) noexcept {
    if (in_dlsym) {
        return nullptr;
    }
    static decltype(calloc)* nextCalloc = (decltype(calloc)*) local_dlsym(RTLD_NEXT, "calloc");

    void* pointer = nextCalloc(n, size);

    size_t totalSize = n * size;
    if (!busy && ready) {
        ++busy;

        size_t index = 0;
        while (size > sizeClasses[index] && index < sizeClasses.size()) {
            ++index;
        }
        bins[index]++;

        average = average + (totalSize - average) / (nAllocations + 1);
        nAllocations++;

        long int liveAllocationsSnapshot = liveAllocations.fetch_add(1) + 1;
        long int maxLiveAllocationsSnapshot = maxLiveAllocations;
        while (liveAllocationsSnapshot > maxLiveAllocationsSnapshot) {
            maxLiveAllocations.compare_exchange_weak(maxLiveAllocationsSnapshot, liveAllocationsSnapshot);
            maxLiveAllocationsSnapshot = maxLiveAllocations;
        }

        --busy;
    }

    return pointer;
}

extern "C" ATTRIBUTE_EXPORT void free(void* pointer) noexcept {
    static decltype(free)* nextFree = (decltype(free)*) local_dlsym(RTLD_NEXT, "free");

    if (!busy && ready) {
        ++busy;
        liveAllocations--;
        --busy;
    }

    nextFree(pointer);
}
