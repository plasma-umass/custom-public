#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

// https://json.nlohmann.me
#include "json.hpp"
using json = nlohmann::json;

#include "constants.hpp"

using Clock = std::chrono::steady_clock;
Clock::time_point programStart;

class Initialization {
  public:
    Initialization() {
        auto seed = std::random_device{}();
        if (const char* env = std::getenv("LITTER_SEED")) {
            seed = atoi(env);
        }
        std::mt19937_64 generator(seed);

        auto occupancy = 0.95;
        if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
            occupancy = atof(env);
            assert(occupancy >= 0 && occupancy <= 1);
        }

        bool noShuffle = false;
        if (const char* env = std::getenv("LITTER_NO_SHUFFLE")) {
            noShuffle = atoi(env) != 0;
        }

        unsigned sleepDelay = 0;
        if (const char* env = std::getenv("LITTER_SLEEP")) {
            sleepDelay = atoi(env);
        }

        auto multiplier = 20;
        if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
            multiplier = atoi(env);
        }

        if (!std::filesystem::exists(DETECTOR_OUTPUT_FILENAME)) {
            std::cout << "[ERROR] " << DETECTOR_OUTPUT_FILENAME << " does not exist..." << std::endl;
            exit(1);
        }
        std::ifstream inputFile(DETECTOR_OUTPUT_FILENAME);
        json data;
        inputFile >> data;

        Dl_info mallocInfo;
        assert(dladdr((void*) &malloc, &mallocInfo) != 0);
        std::cerr << "==================================== Litterer ====================================" << std::endl;
        std::cerr << "malloc     : " << mallocInfo.dli_fname << std::endl;
        std::cerr << "seed       : " << seed << std::endl;
        std::cerr << "occupancy  : " << occupancy << std::endl;
        std::cerr << "shuffle    : " << (noShuffle ? "no" : "yes") << std::endl;
        std::cerr << "sleep      : " << (sleepDelay ? std::to_string(sleepDelay) : "no") << std::endl;
        std::cerr << "multiplier : " << multiplier << std::endl;
        std::cerr << "timestamp  : " << __DATE__ << " " << __TIME__ << std::endl;
        std::cerr << "==================================================================================" << std::endl;

        long long int nAllocations = data["NAllocations"].get<long long int>();
        long long int maxLiveAllocations = data["MaxLiveAllocations"].get<long long int>();
        long long int nAllocationsLitter = maxLiveAllocations * multiplier;

        // This can happen if no allocations were recorded.
        if (!data["Bins"].empty()) {
            if (data["Bins"][data["SizeClasses"].size()].get<int>() != 0) {
                std::cerr << "[WARNING] Allocations of size greater than the maximum size class were recorded."
                          << std::endl;
                std::cerr << "[WARNING] There will be no littering for these allocations." << std::endl;
                std::cerr << "[WARNING] This represents "
                          << ((double) data["Bins"][data["SizeClasses"].size()] / (double) nAllocations) * 100
                          << "% of all allocations recorded." << std::endl;
            }

            std::chrono::high_resolution_clock::time_point litterStart = std::chrono::high_resolution_clock::now();

            std::uniform_int_distribution<long long int> distribution(
                0, nAllocations - data["Bins"][data["SizeClasses"].size()].get<int>() - 1);
            std::vector<void*> objects = *(new std::vector<void*>);
            objects.reserve(nAllocationsLitter);

            for (long long int i = 0; i < nAllocationsLitter; ++i) {
                size_t minAllocationSize = 0;
                size_t sizeClassIndex = 0;
                long long int offset = distribution(generator) - (long long int) data["Bins"][0].get<int>();

                while (offset >= 0LL) {
                    minAllocationSize = data["SizeClasses"][sizeClassIndex].get<size_t>() + 1;
                    ++sizeClassIndex;
                    offset -= (long long int) data["Bins"][sizeClassIndex].get<int>();
                }
                size_t maxAllocationSize = data["SizeClasses"][sizeClassIndex].get<size_t>();
                std::uniform_int_distribution<size_t> allocationSizeDistribution(minAllocationSize, maxAllocationSize);
                size_t allocationSize = allocationSizeDistribution(generator);

                void* pointer = malloc(allocationSize);
                objects.push_back(pointer);
            }

            long long int nObjectsToBeFreed = (1 - occupancy) * nAllocationsLitter;

            if (noShuffle) {
                std::sort(objects.begin(), objects.end(), std::greater<void*>());
            } else {
                for (int i = 0; i < nObjectsToBeFreed; ++i) {
                    std::uniform_int_distribution<int> indexDistribution(i, objects.size() - 1);
                    int index = indexDistribution(generator);
                    std::swap(objects[i], objects[index]);
                }
            }

            for (long long int i = 0; i < nObjectsToBeFreed; ++i) {
                free(objects[i]);
            }

            std::chrono::high_resolution_clock::time_point litterEnd = std::chrono::high_resolution_clock::now();
            std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>((litterEnd - litterStart));
            std::cerr << "Finished littering. Time taken: " << elapsed.count() << " seconds." << std::endl;
        }

        if (sleepDelay) {
            std::cerr << "Sleeping " << sleepDelay << " seconds before starting the program (PID: " << getpid()
                      << ")..." << std::endl;
            sleep(sleepDelay);
            std::cerr << "Starting program now!" << std::endl;
        }

        std::cerr << "==================================================================================" << std::endl;

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
};

static Initialization _;
