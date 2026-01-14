#include "pin.H"

#include <cstdint>
#include <iostream>

namespace {
std::uint64_t numReads = 0;
std::uint64_t numWrites = 0;

void RecordMemRead() {
    ++numReads;
}

void RecordMemWrite() {
    ++numWrites;
}

void Instruction(INS ins, [[maybe_unused]] void* v) {
    const auto numMemOps = INS_MemoryOperandCount(ins);
    for (UINT32 i = 0; i < numMemOps; ++i) {
        if (INS_MemoryOperandIsRead(ins, i)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                                     (AFUNPTR) RecordMemRead, IARG_END);
        }
        if (INS_MemoryOperandIsWritten(ins, i)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                                     (AFUNPTR) RecordMemWrite, IARG_END);
        }
    }
}

void Fini([[maybe_unused]] int32_t code, [[maybe_unused]] void* v) {
    std::cerr << "Memory Reads : " << numReads << std::endl;
    std::cerr << "Memory Writes: " << numWrites << std::endl;
}

int32_t Usage() {
    std::cerr << "This tool counts memory accesses (reads and writes)."
              << std::endl;
    std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;
    return -1;
}
} // namespace

int main(int argc, char* argv[]) {
    if (PIN_Init(argc, argv)) {
        return Usage();
    }

    INS_AddInstrumentFunction(Instruction, nullptr);
    PIN_AddFiniFunction(Fini, nullptr);
    PIN_StartProgram();
}
