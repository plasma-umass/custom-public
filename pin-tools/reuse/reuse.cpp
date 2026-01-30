#include "pin.H"

#include <sys/syscall.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "AVL.hpp"

namespace {
KNOB<std::string> knobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                                 "reuse.json",
                                 "output file name (default: reuse.json)");
KNOB<std::uint64_t>
    knobGranularity(KNOB_MODE_WRITEONCE, "pintool", "g", "6",
                    "power-of-two granularity (default: 6, cache line size)");
KNOB<std::uint64_t> knobHistogramSize(
    KNOB_MODE_WRITEONCE, "pintool", "k", "16777216",
    "number of entries to keep in the histogram (default: 2^24)");
KNOB<bool> knobAttachOnGetpid(KNOB_MODE_WRITEONCE, "pintool",
                              "attach-on-getpid", "false",
                              "only start tracking memory accesses after a "
                              "getpid() call (default: false)");

struct AVLNode {
    std::uint64_t timestamp;
    std::uint64_t address;
    int height;
    std::uint64_t size;
    AVLNode* left;
    AVLNode* right;

    AVLNode(std::uint64_t ts, std::uint64_t addr)
        : timestamp(ts), address(addr), height(1), size(1), left(nullptr),
          right(nullptr) {}
};

class AVLTracker {
  public:
    std::uint64_t trackAndGetDistance(std::uint64_t address) {
        ++now;
        auto it = timestampByAddress.find(address);

        if (it != timestampByAddress.end()) {
            std::uint64_t oldNow = it->second;
            std::uint64_t distance = tree.getRank(oldNow);

            tree.erase(oldNow);
            tree.insert(now);
            timestampByAddress[address] = now;
            return distance;
        }

        timestampByAddress[address] = now;
        tree.insert(now);
        return UINT64_MAX;
    }

  private:
    AVLTree<std::uint64_t> tree;
    std::uint64_t now;
    std::unordered_map<std::uint64_t, std::uint64_t> timestampByAddress;
};

bool attached = false;
std::vector<std::uint64_t> histogram;
std::uint64_t granularity;
AVLTracker tracker;

void TrackMemoryAccess(void* addr) {
    const std::uint64_t address
        = reinterpret_cast<std::uint64_t>(addr) >> granularity;
    const std::uint64_t distance = tracker.trackAndGetDistance(address);
    ++histogram.at(std::min(distance, histogram.size() - 1));
}

void SyscallEntry([[maybe_unused]] THREADID threadIndex,
                  [[maybe_unused]] CONTEXT* ctxt,
                  [[maybe_unused]] SYSCALL_STANDARD std,
                  [[maybe_unused]] void* v) {
    if (!attached) {
        auto syscallNumber = PIN_GetSyscallNumber(ctxt, std);

        if (syscallNumber == SYS_getpid) {
            std::cerr << "[PIN] Detected getpid() syscall marker." << std::endl;
            std::cerr
                << "[PIN] Starting reuse distance analysis with granularity "
                << (1ULL << granularity) << " bytes." << std::endl;
            attached = true;
        }
    }
}

bool IsAttached() {
    return attached;
}

void Instruction(INS ins, [[maybe_unused]] void* v) {
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        if ((INS_MemoryOperandIsRead(ins, memOp) && !INS_IsStackRead(ins))
            || (INS_MemoryOperandIsWritten(ins, memOp)
                && !INS_IsStackWrite(ins))) {
            INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE,
                                       reinterpret_cast<AFUNPTR>(IsAttached),
                                       IARG_END);
            INS_InsertThenPredicatedCall(
                ins, IPOINT_BEFORE,
                reinterpret_cast<AFUNPTR>(TrackMemoryAccess), IARG_MEMORYOP_EA,
                memOp, IARG_END);
        }
    }
}

void Fini([[maybe_unused]] int32_t code, [[maybe_unused]] void* v) {
    std::cerr << "[PIN] Program finished execution." << std::endl;
    std::cerr << "[PIN] Generating output..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    std::string output;
    output += "{\"granularity\":";
    output += std::to_string(granularity);
    output += ",\"histogram\":[";

    if (!histogram.empty()) {
        output += std::to_string(histogram[0]);
        for (size_t i = 1; i < histogram.size(); ++i) {
            output += ',';
            output += std::to_string(histogram[i]);
        }
    }
    output += "]}";

    std::ofstream(knobOutputFile.Value()) << output;

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "[PIN] Output generation took "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end
                                                                       - start)
                     .count()
              << "ms." << std::endl;
}

int32_t Usage() {
    std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;
    return -1;
}
} // namespace

int main(int argc, char* argv[]) {
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    granularity = knobGranularity.Value();
    assert(knobHistogramSize.Value() > 0);
    histogram.resize(knobHistogramSize.Value(), 0);

    if (knobAttachOnGetpid.Value()) {
        std::cerr << "[PIN] Waiting for syscall marker before tracking."
                  << std::endl;
    } else {
        std::cerr << "[PIN] Starting reuse distance analysis with granularity "
                  << (1ULL << granularity) << " bytes." << std::endl;
        attached = true;
    }

    INS_AddInstrumentFunction(Instruction, nullptr);
    PIN_AddSyscallEntryFunction(SyscallEntry, nullptr);
    PIN_AddFiniFunction(Fini, nullptr);
    PIN_StartProgram();
}
