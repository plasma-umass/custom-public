#include <dlfcn.h>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)

#ifdef __APPLE__
#include <dyld-interposing.h>
#define GET_REAL_FUNCTION(NAME) ::NAME
#define INTERPOSE_FUNCTION_NAME(NAME) interpose_##NAME##__
#define INTERPOSE(REPLACEMENT, REPLACEE) DYLD_INTERPOSE(REPLACEMENT, REPLACEE)
#else
#define GET_REAL_FUNCTION(NAME) reinterpret_cast<decltype(::NAME)*>(dlsym(RTLD_NEXT, #NAME))
#define INTERPOSE_FUNCTION_NAME(NAME) NAME
#define INTERPOSE(REPLACEMENT, REPLACEE)
#endif

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

extern "C" void* INTERPOSE_FUNCTION_NAME(reallocarray)(void* pointer, size_t n, size_t size) {
    static auto* real = GET_REAL_FUNCTION(reallocarray);
    return real(pointer, n, size);
}

extern "C" int INTERPOSE_FUNCTION_NAME(posix_memalign)(void** memptr, size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(posix_memalign);
    return real(memptr, alignment, size);
}

extern "C" void* INTERPOSE_FUNCTION_NAME(aligned_alloc)(size_t alignment, size_t size) {
    static auto* real = GET_REAL_FUNCTION(aligned_alloc);
    return real(alignment, size);
}

INTERPOSE(INTERPOSE_FUNCTION_NAME(malloc), malloc)
INTERPOSE(INTERPOSE_FUNCTION_NAME(free), free)
INTERPOSE(INTERPOSE_FUNCTION_NAME(calloc), calloc)
INTERPOSE(INTERPOSE_FUNCTION_NAME(realloc), realloc)
INTERPOSE(INTERPOSE_FUNCTION_NAME(reallocarray), reallocarray)
INTERPOSE(INTERPOSE_FUNCTION_NAME(posix_memalign), posix_memalign)
INTERPOSE(INTERPOSE_FUNCTION_NAME(aligned_alloc), aligned_alloc)
