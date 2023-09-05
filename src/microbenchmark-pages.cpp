#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#if _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <benchmark/benchmark.h>

#define PAGE_SIZE 4096

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

void litter(std::size_t objectSize, std::size_t nPages, std::size_t seed = std::random_device()(),
            std::size_t pageSize = PAGE_SIZE) {
    std::vector<void*> allocated;

    auto nAllocations = guess(objectSize, 0, 0, nPages, pageSize) * 1.05;
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
            if (abs(((std::uintptr_t) allocated[i]) - previous) >= pageSize) {
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
        if (abs(reinterpret_cast<std::uintptr_t>(allocated[i]) - previous) >= pageSize) {
            toBeFreed.push_back((void*) previous);
            previous = reinterpret_cast<std::uintptr_t>(allocated[i]);
        }
    }

    std::cout << "Freeing " << toBeFreed.size() << " objects..." << std::endl;
    std::shuffle(toBeFreed.begin(), toBeFreed.end(), std::default_random_engine(seed));
    for (auto ptr : toBeFreed) {
        free(ptr);
    }
}

#ifndef N
#define N 100'000 // Number of pages.
#endif

#ifndef OBJECT_SIZE
#define OBJECT_SIZE 256 // Size of objects.
#endif

#ifndef ITERATIONS
#define ITERATIONS 40'000
#endif

const auto nMinDistances = 64;
const auto distanceClampMax = PAGE_SIZE;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

#ifdef LITTER
    litter(OBJECT_SIZE, N);
#endif

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

    std::vector<void*> objects;
    objects.reserve(N);

    for (std::size_t i = 0; i < N; ++i) {
        objects.push_back(std::malloc(OBJECT_SIZE));
    }

    std::sort(objects.begin(), objects.end());

    auto minDistances = std::vector<std::size_t>(nMinDistances, std::numeric_limits<std::size_t>::max());
    std::size_t sumDistances = 0;

    for (std::size_t i = 1; i < objects.size(); ++i) {
        const auto distance = std::clamp<std::size_t>(reinterpret_cast<std::uintptr_t>(objects[i])
                                                          - reinterpret_cast<std::uintptr_t>(objects[i - 1]),
                                                      0, distanceClampMax);

        sumDistances += distance;

        minDistances.insert(std::upper_bound(minDistances.begin(), minDistances.end(), distance), distance);
        minDistances.erase(minDistances.end() - 1);
    }

    const auto avgDistance = (double) sumDistances / (objects.size() - 1);

    std::cout << "Min distances: " << minDistances << std::endl;
    std::cout << "Avg distance: " << avgDistance << std::endl;

    const auto start = std::chrono::high_resolution_clock::now();

    std::uint8_t count = 0;
    for (std::size_t i = 0; i < ITERATIONS; i++) {
        for (const auto object : objects) {
            char buffer[OBJECT_SIZE];
            std::memcpy(buffer, object, OBJECT_SIZE);
            count += buffer[OBJECT_SIZE - 1];
            benchmark::DoNotOptimize(buffer);
        }
    }
    benchmark::DoNotOptimize(count);
    benchmark::DoNotOptimize(objects);

    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed: " << duration << "ms" << std::endl;
}