#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>

#include <fmt/core.h>

#include <interpose.h>

namespace {
const std::size_t kAcceptableOffset = 64;
const std::size_t kMaxAnticipatedRun = 4096;

std::array<std::atomic_size_t, kMaxAnticipatedRun> runLengths;
thread_local std::size_t currentRun = 1;
thread_local std::uintptr_t lastResultPlusSize = 0;

const struct Initialization {
    ~Initialization() {
        std::ofstream output(fmt::format("sequence.{}.json", getpid()));
        output << '[' << runLengths.at(0);
        for (std::size_t i = 1; i < kMaxAnticipatedRun; ++i) {
            output << ',' << runLengths.at(i);
        }
        output << ']';
    };

    Initialization() = default;
    Initialization(const Initialization&) = delete;
    Initialization& operator=(const Initialization&) = delete;
    Initialization(Initialization&&) = delete;
    Initialization& operator=(Initialization&&) = delete;
} _;

void processEvent(size_t size, void* result) {
    static thread_local int busy = 0;
    if (busy > 0) {
        return;
    }
    ++busy;

    const auto delta
        = reinterpret_cast<std::uintptr_t>(result) - lastResultPlusSize;
    lastResultPlusSize = reinterpret_cast<std::uintptr_t>(result) + size;

    if (delta <= kAcceptableOffset) {
        ++currentRun;
    } else {
        ++runLengths.at(currentRun - 1);
        currentRun = 1;
    }

    --busy;
}
} // namespace

extern "C" void* INTERPOSE_FUNCTION_NAME(malloc)(size_t size) {
    static auto* next = GET_REAL_FUNCTION(malloc);
    void* result = next(size);
    processEvent(size, result);
    return result;
}
INTERPOSE(malloc);

extern "C" void* INTERPOSE_FUNCTION_NAME(calloc)(size_t n, size_t size) {
    static auto* next = GET_REAL_FUNCTION(calloc);
    void* result = next(n, size);
    processEvent(n * size, result);
    return result;
}
INTERPOSE(calloc);
