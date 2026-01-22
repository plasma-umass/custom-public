#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <fmt/base.h>

#include "litterer.hpp"

namespace {
struct Initialization {
    Initialization() {
        distribution::litterer::runLitterer();
        start = std::chrono::high_resolution_clock::now();
    }

    ~Initialization() {
        std::FILE* log = stderr;
        if (const char* env = std::getenv("LITTER_LOG_FILENAME")) {
            log = std::fopen(env, "a");
            assert(log != nullptr);
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const auto elapsed_ms
            = std::chrono::duration_cast<std::chrono::milliseconds>(
                  (end - start))
                  .count();
        fmt::println(log,
                     "========================================================"
                     "==========================");
        fmt::println(log, "Time elapsed: {} ms", elapsed_ms);
        fmt::println(log,
                     "========================================================"
                     "==========================");

        if (log != stderr) {
            std::fclose(log);
        }
    }

    Initialization(const Initialization&) = delete;
    Initialization& operator=(const Initialization&) = delete;
    Initialization(Initialization&&) = delete;
    Initialization& operator=(Initialization&&) = delete;

  private:
    std::chrono::high_resolution_clock::time_point start;
};

const Initialization _;
} // namespace
