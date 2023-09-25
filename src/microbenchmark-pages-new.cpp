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
#else
#include <unistd.h>
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

#ifndef N
#define N 10'000 // Number of pages.
#endif

#ifndef OBJECT_SIZE
#define OBJECT_SIZE 32 // Size of objects.
#endif

#ifndef ITERATIONS
#define ITERATIONS 40'000
#endif

#define MAX_OBJECTS (N * PAGE_SIZE / OBJECT_SIZE + 1)

int litter(std::array<void*, MAX_OBJECTS>& toBeFreed,
	    std::size_t objectSize,
	    std::size_t nPages,
	    std::default_random_engine::result_type seed = std::random_device()(),
	    std::size_t pageSize = PAGE_SIZE)
{
    auto nAllocations = guess(objectSize, 0, 0, nPages, pageSize);
    std::array<void*, MAX_OBJECTS> allocated;
    int nAllocated = 0;
    int nFreed = 0;
    std::size_t PagesFilled = 0;

    for (;;) {
      std::cout << "Allocating " << (nAllocations - nAllocated) << " objects..." << std::endl;
      while ((nAllocated < nAllocations) && (nAllocated < MAX_OBJECTS)) {
	  auto ptr = std::malloc(objectSize);
	  // std::cout << "  allocated " << objectSize << " : " << (uintptr_t) ptr << std::endl;
	  allocated[nAllocated++] = ptr;
        }

        // Recount how many pages our current allocations are filling.
        std::sort(allocated.begin(), allocated.begin() + nAllocated);
        PagesFilled = 0;
        std::uintptr_t previous = (std::uintptr_t) allocated[0];
        for (std::size_t i = 1; i < nAllocated; ++i) {
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

    auto previous = reinterpret_cast<std::uintptr_t>(allocated[0]);
    for (std::size_t i = 1; i < nAllocated; ++i) {
        if (reinterpret_cast<std::uintptr_t>(allocated[i]) - previous >= pageSize) {
            toBeFreed[nFreed++] = ((void*) previous);
            previous = reinterpret_cast<std::uintptr_t>(allocated[i]);
        }
    }

    // std::cout << "Freeing " << toBeFreed.size() << " objects..." << std::endl;
    std::shuffle(toBeFreed.begin(), toBeFreed.begin() + nFreed, std::default_random_engine(seed));
    for (auto i = 0; i < nFreed; i++) {
      //      std::cout << "  freeing " << (uintptr_t) ptr << std::endl;
      std::free(toBeFreed[i]);
    }
    return nFreed;
}

const auto distanceClampMax = PAGE_SIZE;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    std::uint32_t sleepDelay = 0;
    if (const char* env = std::getenv("LITTER_SLEEP")) {
        sleepDelay = atoi(env);
    }

    std::cout << "Object size: " << OBJECT_SIZE << std::endl;
    std::cout << "Object distance: " << OBJECT_DISTANCE << std::endl;

    std::array<void *, MAX_OBJECTS> freed;
    auto nFreed = litter(freed, OBJECT_SIZE, N, std::random_device()(), OBJECT_DISTANCE);

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

    std::array<void*, N> objects;
    //    objects.reserve(N);

    for (std::size_t i = 0; i < N; ++i) {
      auto ptr = std::malloc(OBJECT_SIZE);
      // std::cout << "object " << OBJECT_SIZE << " pushing " << (uintptr_t) ptr << std::endl;
      objects[i] = ptr;
    }

    std::sort(objects.begin(), objects.end());

    std::size_t sumDistances = 0;
    auto distances = std::map<std::size_t, std::size_t>();
    for (std::size_t i = 1; i < objects.size(); ++i) {
      const auto distance = std::clamp<std::size_t>(reinterpret_cast<std::uintptr_t>(objects[i])
						    - reinterpret_cast<std::uintptr_t>(objects[i - 1]),
						    0, distanceClampMax);
	
      sumDistances += distance;
      ++distances[distance];
    }


    int intersection = 0;
    int intersectionBytes = 0;
    {
      std::unordered_set<void *> freedBytes;
      {
	std::unordered_set<void *> freedSet;
	for (auto i = 0; i < nFreed; i++) {
	  freedSet.insert(freed[i]);
	}
	for (const auto& f : freedSet) {
	  assert(f);
	  for (auto i = 0; i < OBJECT_SIZE; i++) {
	    freedBytes.insert((void *) ((uintptr_t) f + i));
	  }
	}
	intersection
	  = std::count_if(objects.begin(), objects.end(), [&](const auto o) { return freedSet.contains(o); });
      }
      
      {
	std::unordered_set<void *> objectsBytes;
	for (const auto& o : objects) {
	  for (auto i = 0; i < OBJECT_SIZE; i++) {
	    objectsBytes.insert((void *) ((uintptr_t) o + i));
	  }
	}
	
	intersectionBytes
	  = std::count_if(objectsBytes.begin(), objectsBytes.end(), [&](const auto o) { return freedBytes.contains(o); });
      }
    }
    
    std::cout << "Intersection (objects): " << intersection << " / " << (objects.size()) << std::endl;
    std::cout << "Intersection (bytes): " << intersectionBytes << " / " << (OBJECT_SIZE * objects.size()) << std::endl;

    const auto avgDistance = (double) sumDistances / (objects.size() - 1);

    std::cout << "Min distances:" << std::endl;
    for (const auto [distance, count] : distances) {
        if (count > 20) {
            std::cout << "\t" << distance << ": " << count << std::endl;
        }
    }
    std::cout << "Avg distance: " << avgDistance << std::endl;


    volatile unsigned long count = 0;
    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < ITERATIONS; i++) {
        for (const auto object : objects) {
            volatile char buffer[OBJECT_SIZE];
            std::memcpy((void*) buffer, object, OBJECT_SIZE);
            count += buffer[OBJECT_SIZE - 1];
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed (littered): " << duration << "ms" << std::endl;


    std::array<void*, N> newObjects;
    for (auto i = 0; i < N; i++) {
      newObjects[i] = std::malloc(OBJECT_SIZE);
    }
    
    start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < ITERATIONS; i++) {
        for (const auto object : newObjects) {
            volatile char buffer[OBJECT_SIZE];
            std::memcpy((void*) buffer, object, OBJECT_SIZE);
            count += buffer[OBJECT_SIZE - 1];
        }
    }

    end = std::chrono::high_resolution_clock::now();
    auto durationContiguous = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Elapsed (contiguous): " << durationContiguous << "ms" << std::endl;

    std::cout << "Ratio = " << (float) duration / (float) durationContiguous << std::endl;
}
