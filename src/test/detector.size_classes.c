#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

// This test is tuned for the jemalloc size classes.
int main() {
    void* ptr;

    ptr = malloc(1);
    ptr = realloc(ptr, 8);
    free(ptr);

    ptr = calloc(1, 16);
    ptr = reallocarray(ptr, 1, 32);
    free(ptr);

    int result = posix_memalign(&ptr, 4096, 48);
    assert(result == 0);
    free(ptr);

    ptr = aligned_alloc(64, 64);
    free(ptr);
}
