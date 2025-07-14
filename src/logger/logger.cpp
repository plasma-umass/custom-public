#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>

#include <dlfcn.h>
#include <interpose.h>

#include "shared.hpp"

using Clock = std::chrono::steady_clock;

namespace {
bool initialized = false;
std::ofstream output;
std::mutex lock;
Clock::time_point startTime;

const struct Initialization {
    Initialization() {
        Dl_info info;
        const auto next = GET_REAL_FUNCTION(malloc);
        const int status = dladdr(reinterpret_cast<void*>(next), &info);
        const std::string object = (status != 0) ? info.dli_fname : "[unknown]";
        std::cerr << "Using malloc from: " << object << std::endl;

        output = std::ofstream("events.bin", std::ios::binary);
        startTime = Clock::now();
        initialized = true;
    };
} _;

void processEvent(Event event) {
    static thread_local int busy = 0;
    if (!initialized || busy > 0) {
        return;
    }

    if (event.type == EventType::Reallocation && event.pointer == 0)
        [[unlikely]] {
        processEvent({.type = EventType::Allocation,
                      .size = event.size,
                      .result = event.result});
        return;
    }

    event.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             Clock::now() - startTime)
                             .count();

    if (event.type == EventType::Free && event.pointer == 0) [[unlikely]] {
        return;
    }

    ++busy;
    const std::unique_lock<std::mutex> guard(lock);
    output.write(reinterpret_cast<const char*>(&event), sizeof(event));
    --busy;
}
} // namespace

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(malloc);
    void* result = next(size);
    processEvent({.type = EventType::Allocation,
                  .size = size,
                  .result = reinterpret_cast<std::uint64_t>(result)});
    return result;
}
INTERPOSE(malloc);

extern "C" void INTERPOSE_FUNCTION_NAME(free)(void* pointer) {
    static auto* next = GET_REAL_FUNCTION(free);
    processEvent({.type = EventType::Free,
                  .pointer = reinterpret_cast<std::uint64_t>(pointer)});
    next(pointer);
}
INTERPOSE(free);

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(uint64_t n, uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(calloc);
    void* result = next(n, size);
    processEvent({.type = EventType::Allocation,
                  .size = n * size,
                  .result = reinterpret_cast<std::uint64_t>(result)});
    return result;
}
INTERPOSE(calloc);

extern "C" void* INTERPOSE_FUNCTION_NAME(realloc)(void* pointer,
                                                  uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(realloc);
    void* result = next(pointer, size);
    processEvent({.type = EventType::Reallocation,
                  .size = size,
                  .pointer = reinterpret_cast<std::uint64_t>(pointer),
                  .result = reinterpret_cast<std::uint64_t>(result)});
    return result;
}
INTERPOSE(realloc);

#ifndef __APPLE__
extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer,
                                                       uint64_t n,
                                                       uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(reallocarray);
    void* result = next(pointer, n, size);
    processEvent({.type = EventType::Reallocation,
                  .size = n * size,
                  .pointer = reinterpret_cast<std::uint64_t>(pointer),
                  .result = reinterpret_cast<std::uint64_t>(result)});
    return result;
}
INTERPOSE(reallocarray);
#endif

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr,
                                                       uint64_t alignment,
                                                       uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(posix_memalign);
    const int result = next(memptr, alignment, size);
    processEvent({.type = EventType::Allocation,
                  .size = size,
                  .result = reinterpret_cast<std::uint64_t>(*memptr)});
    return result;
}
INTERPOSE(posix_memalign);

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(uint64_t alignment,
                                                        uint64_t size) {
    static auto* next = GET_REAL_FUNCTION(aligned_alloc);
    void* result = next(alignment, size);
    processEvent({.type = EventType::Allocation,
                  .size = size,
                  .result = reinterpret_cast<std::uint64_t>(result)});
    return result;
}
INTERPOSE(aligned_alloc);
