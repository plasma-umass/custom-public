#include <arpa/inet.h>
#include <cstring>
#include <dlfcn.h>
#include <errno.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <fmt/core.h>

#define EXPORT __attribute__((visibility("default")))

static thread_local uint32_t busy;
static int sockfd;

struct Initialization {
    Initialization() {
        if ((sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) == -1) {
            fmt::print("Error while creating the socket: {}\n", strerror(errno));
        }

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) != 0) {
            fmt::print("Error while naming the socket: {}\n", strerror(errno));
        }

        socklen_t address_len = sizeof(address);
        if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&address), &address_len) != 0
            || address_len != sizeof(address)) {
            fmt::print("Error while getting the socket's port: {}\n", strerror(errno));
        }

        fmt::print(stderr, "[LITTERER] Initialized\n");
        fmt::print(stderr, "[LITTERER] PID: {}\n", getpid());
        fmt::print(stderr, "[LITTERER] Port: {}\n", address.sin_port);
    }
};

static const Initialization _;

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
