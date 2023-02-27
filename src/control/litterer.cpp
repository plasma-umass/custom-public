#include <dlfcn.h>
#include <unistd.h>

#include <cstdio>

#include <fmt/core.h>

#define EXPORT __attribute__((visibility("default")))

static thread_local uint32_t busy; 

extern "C" EXPORT void* malloc(size_t size) {
    static decltype(malloc)* nextMalloc = (decltype(malloc)*) dlsym(RTLD_NEXT, "malloc");

    if (busy) {
        return nextMalloc(size);
    }

    ++busy;
    void* allocation = nextMalloc(size);
    --busy;

    return allocation;
}

extern "C" EXPORT void free(void* pointer) {
    static decltype(free)* nextFree = (decltype(free)*) dlsym(RTLD_NEXT, "free");

    if (busy) {
        nextFree(pointer);
        return;
    }

    ++busy;
    nextFree(pointer);
    --busy;
}

extern "C" EXPORT void* calloc(size_t n, size_t size) {
    static decltype(calloc)* nextCalloc = (decltype(calloc)*) dlsym(RTLD_NEXT, "calloc");

    if (busy) {
        return nextCalloc(n, size);
    }

    ++busy;
    void* allocation = nextCalloc(n, size);
    --busy;

    return allocation;
}

extern "C" EXPORT void* realloc(void* pointer, size_t size) {
    static decltype(realloc)* nextRealloc = (decltype(realloc)*) dlsym(RTLD_NEXT, "realloc");

    if (busy) {
        return nextRealloc(pointer, size);
    }

    ++busy;
    void* allocation = nextRealloc(pointer, size);
    --busy;

    return allocation;
}

extern "C" EXPORT void* reallocarray(void* pointer, size_t n, size_t size) {
    static decltype(reallocarray)* nextReallocarray = (decltype(reallocarray)*) dlsym(RTLD_NEXT, "reallocarray");

    if (busy) {
        return nextReallocarray(pointer, n, size);
    }

    ++busy;
    void* allocation = nextReallocarray(pointer, n, size);
    --busy;

    return allocation;
}

struct Initialization {
    Initialization() {
        fmt::print("[LITTERER] Initialized\n");
        fmt::print("[LITTERER] PID: {}\n", getpid());
    }
};

static const Initialization _;
