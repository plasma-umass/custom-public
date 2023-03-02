#include <litterer/litterer.h>

#include <chrono>
#include <iostream>

using Clock = std::chrono::steady_clock;

struct Initialization {
    Initialization() {
        runLitterer();
        programStart = Clock::now();
    }

    ~Initialization() {
        auto programEnd = Clock::now();
        std::cerr << "==================================================================================" << std::endl;
        std::cerr << "Time elapsed: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(programEnd - programStart).count() / 1000.0
                  << std::endl;
        std::cerr << "==================================================================================" << std::endl;
    }

  private:
    Clock::time_point programStart;
};

static Initialization _;
