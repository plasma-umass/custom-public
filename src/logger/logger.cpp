#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>

#include <interpose.h>

namespace {
enum class EventType : std::uint8_t {
    Null,
    Malloc,
    Free,
    Calloc,
    Realloc,
    ReallocArray,
    PosixMemalign,
    AlignedAlloc,
};

struct Event {
    EventType type = EventType::Null;
    std::size_t size = 0;
    std::uintptr_t pointer = 0;
    std::uintptr_t result = 0;
};

bool initialized = false;
std::ofstream output;
std::mutex lock;

const struct Initialization {
    Initialization() {
        output = std::ofstream("events.bin", std::ios::binary);
        initialized = true;
    };
} _;
} // namespace

void processEvent(const Event& event) {
    static thread_local int busy = 0;
    if (!initialized || busy > 0) {
        return;
    }
    ++busy;
    const std::unique_lock<std::mutex> guard(lock);
    output.write(reinterpret_cast<const char*>(&event), sizeof(event));
    --busy;
}

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(size_t size) {
    static auto* next = GET_REAL_FUNCTION(malloc);
    void* result = next(size);
    processEvent({.type = EventType::Malloc, .size = size, .result = reinterpret_cast<std::uintptr_t>(result)});
    return result;
}
INTERPOSE(malloc);

extern "C" void INTERPOSE_FUNCTION_NAME(free)(void* pointer) {
    static auto* next = GET_REAL_FUNCTION(free);
    processEvent({.type = EventType::Free, .pointer = reinterpret_cast<std::uintptr_t>(pointer)});
    next(pointer);
}
INTERPOSE(free);

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(size_t n, size_t size) {
    static auto* next = GET_REAL_FUNCTION(calloc);
    void* result = next(n, size);
    processEvent({.type = EventType::Calloc, .size = n * size, .result = reinterpret_cast<std::uintptr_t>(result)});
    return result;
}
INTERPOSE(calloc);

extern "C" void* INTERPOSE_FUNCTION_NAME(realloc)(void* pointer, size_t size) {
    static auto* next = GET_REAL_FUNCTION(realloc);
    void* result = next(pointer, size);
    processEvent({.type = EventType::Realloc,
                  .size = size,
                  .pointer = reinterpret_cast<std::uintptr_t>(pointer),
                  .result = reinterpret_cast<std::uintptr_t>(result)});
    return result;
}
INTERPOSE(realloc);

#ifndef __APPLE__
extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer, size_t n, size_t size) {
    static auto* next = GET_REAL_FUNCTION(reallocarray);
    void* result = next(pointer, n, size);
    processEvent({.type = EventType::ReallocArray,
                  .size = n * size,
                  .pointer = reinterpret_cast<std::uintptr_t>(pointer),
                  .result = reinterpret_cast<std::uintptr_t>(result)});
    return result;
}
INTERPOSE(reallocarray);
#endif

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr, size_t alignment, size_t size) {
    static auto* next = GET_REAL_FUNCTION(posix_memalign);
    const int result = next(memptr, alignment, size);
    processEvent({.type = EventType::PosixMemalign, .size = size, .result = reinterpret_cast<std::uintptr_t>(*memptr)});
    return result;
}
INTERPOSE(posix_memalign);

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(size_t alignment, size_t size) {
    static auto* next = GET_REAL_FUNCTION(aligned_alloc);
    void* result = next(alignment, size);
    processEvent({.type = EventType::AlignedAlloc, .size = size, .result = reinterpret_cast<std::uintptr_t>(result)});
    return result;
}
INTERPOSE(aligned_alloc);
