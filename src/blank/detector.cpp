#include <dlfcn.h>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

extern "C" void* malloc(size_t size) {
    static auto* next = reinterpret_cast<decltype(malloc)*>(dlsym(RTLD_NEXT, "malloc"));
    return next(size);
}

extern "C" void free(void* pointer) {
    static auto* next = reinterpret_cast<decltype(free)*>(dlsym(RTLD_NEXT, "free"));
    next(pointer);
}

extern "C" void* calloc(size_t n, size_t size) {
    static auto* next = reinterpret_cast<decltype(calloc)*>(dlsym(RTLD_NEXT, "calloc"));
    return next(n, size);
}

extern "C" void* realloc(void* pointer, size_t size) {
    static auto* next = reinterpret_cast<decltype(realloc)*>(dlsym(RTLD_NEXT, "realloc"));
    return next(pointer, size);
}

extern "C" void* reallocarray(void* pointer, size_t n, size_t size) {
    static auto* next = reinterpret_cast<decltype(reallocarray)*>(dlsym(RTLD_NEXT, "reallocarray"));
    return next(pointer, n, size);
}

extern "C" int posix_memalign(void** memptr, size_t alignment, size_t size) {
    static auto* next = reinterpret_cast<decltype(posix_memalign)*>(dlsym(RTLD_NEXT, "posix_memalign"));
    return next(memptr, alignment, size);
}

extern "C" void* aligned_alloc(size_t alignment, size_t size) {
    static auto* next = reinterpret_cast<decltype(aligned_alloc)*>(dlsym(RTLD_NEXT, "aligned_alloc"));
    return next(alignment, size);
}
