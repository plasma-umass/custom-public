#pragma once

// Taken from Google Benchmark.
template <typename T>
inline __attribute__((always_inline)) void DoNotOptimize(const T& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}
