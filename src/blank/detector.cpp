#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <interpose.h>

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(size_t size) {
    static auto* real = GET_REAL_FUNCTION(malloc);
    return real(size);
}

extern "C" void INTERPOSE_FUNCTION_NAME(free)(void* pointer) {
    static auto* real = GET_REAL_FUNCTION(free);
    real(pointer);
}

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(size_t n, size_t size) {
    static auto* real = GET_REAL_FUNCTION(calloc);
    return real(n, size);
}

extern "C" void* INTERPOSE_FUNCTION_NAME(realloc)(void* pointer, size_t size) {
    static auto* real = GET_REAL_FUNCTION(realloc);
    return real(pointer, size);
}

#ifndef __APPLE__
extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer, size_t n, size_t size) {
    static auto* real = GET_REAL_FUNCTION(reallocarray);
    return real(pointer, n, size);
}
#endif

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr, size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(posix_memalign);
    return real(memptr, alignment, size);
}

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(aligned_alloc);
    return real(alignment, size);
}

INTERPOSE(malloc)
INTERPOSE(free)
INTERPOSE(calloc)
INTERPOSE(realloc)
INTERPOSE(reallocarray)
INTERPOSE(posix_memalign)
INTERPOSE(aligned_alloc)
