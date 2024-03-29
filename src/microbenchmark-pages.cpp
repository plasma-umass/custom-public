#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>

#if _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define PAGE_SIZE 4096

#ifndef OBJECT_DISTANCE
#define OBJECT_DISTANCE PAGE_SIZE
#endif

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& v) {
    if (v.empty()) {
        return o << "[]";
    }

    o << "[" << v[0];
    for (std::size_t i = 1; i < v.size(); ++i) {
        o << ", " << v[i];
    }
    return o << "]";
}

namespace {
std::size_t guess(std::size_t objectSize, std::size_t nObjectsAlreadyAllocated, std::size_t nPagesFilled,
                  std::size_t nPages, std::size_t pageSize = PAGE_SIZE) {
    return (nObjectsAlreadyAllocated && nPagesFilled) ? nObjectsAlreadyAllocated * nPages / nPagesFilled
                                                      : nPages * pageSize / objectSize;
}

template <typename T>
T abs(T x) {
    return x < 0 ? -x : x;
}
} // namespace

std::unordered_set<void*> litter(std::size_t objectSize, std::size_t nPages,
                                 std::default_random_engine::result_type seed = std::random_device()(),
                                 std::size_t pageSize = PAGE_SIZE) {
    std::vector<void*> allocated;

    auto nAllocations = guess(objectSize, 0, 0, nPages, pageSize);
    std::size_t PagesFilled = 0;

    for (;;) {
        std::cout << "Allocating " << nAllocations << " objects..." << std::endl;
        while (allocated.size() < nAllocations) {
            allocated.push_back(std::malloc(objectSize));
        }

        // Recount how many pages our current allocations are filling.
        std::sort(allocated.begin(), allocated.end());
        PagesFilled = 0;
        std::uintptr_t previous = (std::uintptr_t) allocated[0];
        for (std::size_t i = 1; i < allocated.size(); ++i) {
            if ((std::uintptr_t) allocated[i] - previous >= pageSize) {
                previous = (std::uintptr_t) allocated[i];
                ++PagesFilled;
            }
        }

        // If we fell short, allocate more.
        if (PagesFilled >= nPages) {
            break;
        }

        nAllocations = guess(objectSize, nAllocations, PagesFilled, nPages, pageSize);
    }

    std::vector<void*> toBeFreed;
    toBeFreed.reserve(PagesFilled);

    auto previous = reinterpret_cast<std::uintptr_t>(allocated[0]);
    for (std::size_t i = 1; i < allocated.size(); ++i) {
        if (reinterpret_cast<std::uintptr_t>(allocated[i]) - previous >= pageSize) {
            toBeFreed.push_back((void*) previous);
            previous = reinterpret_cast<std::uintptr_t>(allocated[i]);
        }
    }

    std::cout << "Freeing " << toBeFreed.size() << " objects..." << std::endl;
    std::shuffle(toBeFreed.begin(), toBeFreed.end(), std::default_random_engine(seed));
    for (auto ptr : toBeFreed) {
        std::free(ptr);
    }

    return std::unordered_set<void*>(toBeFreed.begin(), toBeFreed.end());
}

#ifndef N
#define N 10'000 // Number of pages.
#endif

#ifndef OBJECT_SIZE
#define OBJECT_SIZE 32 // Size of objects.
#endif

#ifndef ITERATIONS
#define ITERATIONS 40'000
#endif

const auto distanceClampMax = PAGE_SIZE;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

    std::cout << "Object size: " << OBJECT_SIZE << std::endl;
    std::cout << "Object distance: " << OBJECT_DISTANCE << std::endl;

    std::vector<void*> objects;
    objects.reserve(N);

    const auto freed = litter(OBJECT_SIZE, N, std::random_device()(), OBJECT_DISTANCE);

    if (sleepDelay) {
#ifdef _WIN32
        const auto pid = GetCurrentProcessId();
#else
        const auto pid = getpid();
#endif
        std::cout << "Sleeping " << sleepDelay << " seconds before resuming (PID: " << pid << ")..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(sleepDelay));
        std::cout << "Resuming program now!" << std::endl;
    }

    for (std::size_t i = 0; i < N; ++i) {
        objects.push_back(std::malloc(OBJECT_SIZE));
    }

    std::sort(objects.begin(), objects.end());

    auto distances = std::map<std::size_t, std::size_t>();
    std::size_t sumDistances = 0;

    for (std::size_t i = 1; i < objects.size(); ++i) {
        const auto distance = std::clamp<std::size_t>(reinterpret_cast<std::uintptr_t>(objects[i])
                                                          - reinterpret_cast<std::uintptr_t>(objects[i - 1]),
                                                      0, distanceClampMax);

        sumDistances += distance;
        ++distances[distance];
    }

    const auto intersection
        = std::count_if(objects.begin(), objects.end(), [&](const auto o) { return freed.contains(o); });

    std::cout << "Intersection: " << intersection << " / " << objects.size() << std::endl;

    const auto avgDistance = (double) sumDistances / (objects.size() - 1);

    std::cout << "Min distances:" << std::endl;
    for (const auto [distance, count] : distances) {
        if (count > 20) {
            std::cout << "\t" << distance << ": " << count << std::endl;
        }
    }
    std::cout << "Avg distance: " << avgDistance << std::endl;

    const auto start = std::chrono::high_resolution_clock::now();

    volatile std::uint8_t count = 0;
    for (std::size_t i = 0; i < ITERATIONS; i++) {
        for (const auto object : objects) {
            volatile char buffer[OBJECT_SIZE];
            std::memcpy((void*) buffer, object, OBJECT_SIZE);
            count += buffer[OBJECT_SIZE - 1];
        }
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed: " << duration << "ms" << std::endl;
}