#include "pin.H"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/syscall.h>
#include <unordered_map>

namespace {
KNOB<std::string> knobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                                 "reuse.json", "output file name");
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

std::unordered_map<std::uint64_t, std::uint64_t>
    lastAccessByAddress; // TODO: F14ValueMap?
std::vector<std::uint64_t> histogram;
std::uint64_t currentCount = 0;
std::uint64_t granularity;
bool attached = false;

void TrackMemoryAccess(void* addr) {
    ++currentCount;
    auto address = reinterpret_cast<std::uint64_t>(addr) >> granularity;
    const auto [it, inserted]
        = lastAccessByAddress.insert({address, currentCount});

    if (inserted) {
        ++histogram.back();
        return;
    }

    const std::uint64_t distance = currentCount - it->second;
    it->second = currentCount;
    ++histogram[std::min(distance - 1, histogram.size() - 1)];
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
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertIfPredicatedCall(ins, IPOINT_BEFORE,
                                       reinterpret_cast<AFUNPTR>(IsAttached),
                                       IARG_END);
            INS_InsertThenPredicatedCall(
                ins, IPOINT_BEFORE,
                reinterpret_cast<AFUNPTR>(TrackMemoryAccess), IARG_MEMORYOP_EA,
                memOp, IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memOp)) {
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
    std::cerr << "[PIN] Program finished execution. Final access map size: "
              << lastAccessByAddress.size() << "." << std::endl;
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

    // Single write operation
    auto outFile = std::ofstream(knobOutputFile.Value());
    outFile << output;

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
