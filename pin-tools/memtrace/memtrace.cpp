#include "pin.H"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

struct ThreadLocalityData {
    std::vector<uint64_t> recentAccesses;
    size_t bufferIndex = 0;
    uint64_t operations = 0;
    uint64_t hits = 0;

    ThreadLocalityData(size_t bufferSize) : recentAccesses(bufferSize, 0) {}
};

namespace {
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                                 "memtrace.out", "specify output file name");
KNOB<uint64_t> KnobBufferSize(KNOB_MODE_WRITEONCE, "pintool", "k", "32",
                              "size of the recent address buffer per thread");
KNOB<uint64_t>
    KnobReportInterval(KNOB_MODE_WRITEONCE, "pintool", "n", "1000000",
                       "report locality percentage every N operations");

TLS_KEY tlsKey;
PIN_LOCK fileOutputLock;
std::ofstream outFile;

ThreadLocalityData* GetOrCreateThreadData(THREADID threadId) {
    auto* tdata
        = static_cast<ThreadLocalityData*>(PIN_GetThreadData(tlsKey, threadId));
    if (tdata == nullptr) {
        tdata = new ThreadLocalityData(KnobBufferSize.Value());
        PIN_SetThreadData(tlsKey, tdata, threadId);
    }
    return tdata;
}

void TrackMemoryAccess(THREADID threadId, void* addr) {
    ThreadLocalityData* tdata = GetOrCreateThreadData(threadId);
    const auto address = reinterpret_cast<uint64_t>(addr);

    const bool found = std::find(tdata->recentAccesses.begin(),
                                 tdata->recentAccesses.end(), address)
                       != tdata->recentAccesses.end();

    tdata->operations++;
    if (found) {
        tdata->hits++;
    }

    // Update circular buffer.
    tdata->recentAccesses[tdata->bufferIndex] = address;
    tdata->bufferIndex
        = (tdata->bufferIndex + 1) % tdata->recentAccesses.size();

    // Report every N operations.
    if (tdata->operations % KnobReportInterval.Value() == 0) {
        const double hitPercentage
            = (tdata->operations > 0)
                  ? (100.0 * static_cast<double>(tdata->hits))
                        / static_cast<double>(tdata->operations)
                  : 0.0;

        PIN_GetLock(&fileOutputLock, threadId);
        outFile << "Thread " << threadId << " at operation "
                << tdata->operations << ": " << hitPercentage << "% hit rate"
                << std::endl;
        PIN_ReleaseLock(&fileOutputLock);

        // Reset counters for next interval.
        tdata->hits = 0;
        tdata->operations = 0;
    }
}

void Instruction(INS ins, [[maybe_unused]] void* v) {
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                reinterpret_cast<AFUNPTR>(TrackMemoryAccess), IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp, IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                reinterpret_cast<AFUNPTR>(TrackMemoryAccess), IARG_THREAD_ID,
                IARG_MEMORYOP_EA, memOp, IARG_END);
        }
    }
}

void Fini([[maybe_unused]] int32_t code, [[maybe_unused]] void* v) {
    outFile.close();
}

int32_t Usage() {
    std::cerr << "This tool tracks memory access locality over time"
              << std::endl;
    std::cerr
        << "It maintains a circular buffer of K recent addresses per thread"
        << std::endl;
    std::cerr << "and reports the hit rate every N operations" << std::endl;
    std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;
    return -1;
}
} // namespace

int main(int argc, char* argv[]) {
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    outFile.open(KnobOutputFile.Value().c_str());

    tlsKey = PIN_CreateThreadDataKey(nullptr);
    PIN_InitLock(&fileOutputLock);

    INS_AddInstrumentFunction(Instruction, nullptr);
    PIN_AddFiniFunction(Fini, nullptr);
    PIN_StartProgram();
}
