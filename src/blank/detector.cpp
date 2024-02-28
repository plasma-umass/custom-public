#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#include <interpose.h>

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(size_t size) {
    static auto* real = GET_REAL_FUNCTION(malloc);
    return real(size);
}
INTERPOSE(malloc);

extern "C" void INTERPOSE_FUNCTION_NAME(free)(void* pointer) {
    static auto* real = GET_REAL_FUNCTION(free);
    real(pointer);
}
INTERPOSE(free);

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(size_t n, size_t size) {
    static auto* real = GET_REAL_FUNCTION(calloc);
    return real(n, size);
}
INTERPOSE(calloc);

extern "C" void* INTERPOSE_FUNCTION_NAME(realloc)(void* pointer, size_t size) {
    static auto* real = GET_REAL_FUNCTION(realloc);
    return real(pointer, size);
}
INTERPOSE(realloc);

#ifndef __APPLE__
extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer, size_t n, size_t size) {
    static auto* real = GET_REAL_FUNCTION(reallocarray);
    return real(pointer, n, size);
}
INTERPOSE(reallocarray);
#endif

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr, size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(posix_memalign);
    return real(memptr, alignment, size);
}
INTERPOSE(posix_memalign);

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(aligned_alloc);
    return real(alignment, size);
}
INTERPOSE(aligned_alloc);
