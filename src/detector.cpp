#include <atomic>
#include <cstddef>
#include <fstream>
#include <vector>

#include <mimalloc.h>

#define PAGE_SIZE 4096zu

namespace {
static std::atomic_bool ready{false};
static thread_local int busy{0};

static std::vector<std::atomic_uint64_t> bins(PAGE_SIZE);

static std::atomic_uint64_t nAllocations{0};
static std::atomic<double> average{0};

// Increments by 1 on malloc/calloc, and decrements by 1 on free.
static std::atomic_int64_t liveAllocations{0};
static std::atomic_int64_t maxLiveAllocations{0};

template <bool addToTotal>
void processAllocation(std::size_t size) {
    // This only does not mess with the statistics because we ignore malloc(0)
    // and free(nullptr).
    if (size == 0) {
        return;
    }

    // Update average.
    average = average + (size - average) / (nAllocations + 1);
    nAllocations++;

    // Increment histogram entry.
    const std::size_t index = std::min(size, PAGE_SIZE) - 1;
    bins[index]++;

    if constexpr (addToTotal) {
        // Increment total live allocations and possibly update maximum.
        const std::int64_t liveAllocationsSnapshot = liveAllocations.fetch_add(1) + 1;
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

        std::ofstream outputFile("detector.out");

        outputFile << "{" << std::endl;

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
    if (size == 0) {
        return nullptr;
    }

    void* pointer = mi_malloc(size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return pointer;
}

extern "C" void free(void* pointer) {
    if (pointer == nullptr) {
        return;
    }

    if (!busy && ready) {
        ++busy;
        liveAllocations--;
        --busy;
    }

    mi_free(pointer);
}

extern "C" void* calloc(std::size_t nmemb, std::size_t size) {
    if (nmemb == 0 || size == 0) {
        return nullptr;
    }

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

    if (size == 0) {
        return nullptr;
    }

    if (!busy && ready) {
        ++busy;
        processAllocation<false>(size);
        --busy;
    }

    return pointer;
}

extern "C" void* reallocarray(void* ptr, std::size_t nmemb, std::size_t size) {
    void* pointer = mi_reallocarray(ptr, nmemb, size);

    if (nmemb == 0 || size == 0) {
        return nullptr;
    }

    if (!busy && ready) {
        ++busy;
        processAllocation<false>(nmemb * size);
        --busy;
    }

    return pointer;
}

extern "C" int posix_memalign(void** memptr, std::size_t alignment, std::size_t size) {
    if (size == 0) {
        *memptr = nullptr;
        return 0;
    }

    int result = mi_posix_memalign(memptr, alignment, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return result;
}

extern "C" void* aligned_alloc(std::size_t alignment, std::size_t size) {
    if (size == 0) {
        return nullptr;
    }

    void* pointer = mi_aligned_alloc(alignment, size);

    if (!busy && ready) {
        ++busy;
        processAllocation<true>(size);
        --busy;
    }

    return pointer;
}
