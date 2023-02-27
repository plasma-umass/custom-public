#include <dlfcn.h>

#define EXPORT __attribute__((visibility("default")))

extern "C" EXPORT void* malloc(size_t size) {
    static decltype(malloc)* nextMalloc = (decltype(malloc)*) dlsym(RTLD_NEXT, "malloc");
    return nextMalloc(size);
}

extern "C" EXPORT void free(void* pointer) {
    static decltype(free)* nextFree = (decltype(free)*) dlsym(RTLD_NEXT, "free");
    nextFree(pointer);
}

extern "C" EXPORT void* calloc(size_t n, size_t size) {
    static decltype(calloc)* nextCalloc = (decltype(calloc)*) dlsym(RTLD_NEXT, "calloc");
    return nextCalloc(n, size);
}

extern "C" EXPORT void* realloc(void* pointer, size_t size) {
    static decltype(realloc)* nextRealloc = (decltype(realloc)*) dlsym(RTLD_NEXT, "realloc");
    return nextRealloc(pointer, size);
}

extern "C" EXPORT void* reallocarray(void* pointer, size_t n, size_t size) {
    static decltype(reallocarray)* nextReallocarray = (decltype(reallocarray)*) dlsym(RTLD_NEXT, "reallocarray");
    return nextReallocarray(pointer, n, size);
}
