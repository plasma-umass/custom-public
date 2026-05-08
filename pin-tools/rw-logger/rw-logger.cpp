#include "pin.H"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sys/syscall.h>
#include <unordered_map>
#include <vector>

namespace {
KNOB<std::string> knobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o",
                                 "rw-logger.bin", "output file name");

std::atomic_bool attached = false;

PIN_LOCK outputLock;
TLS_KEY tlsKey;
std::ofstream outputFile;

static constexpr std::uint64_t kBufferSize = 1 << 20;
static constexpr uint64_t kPayloadMask = (1ULL << 48) - 1;
enum class EntryKind : uint64_t {
    Read = 0b00ULL << 62,
    Write = 0b01ULL << 62,
    Syscall = 0b10ULL << 62
};

void WriteToFile(THREADID tid, std::vector<std::uint64_t>& buffer) {
    PIN_GetLock(&outputLock, tid);

    uint64_t tid64 = static_cast<uint64_t>(tid);
    uint64_t size = buffer.size();

    std::cerr << "[PIN] Writing " << size << " entries to file (TID: " << tid
              << ")..." << std::endl;

    outputFile.write(reinterpret_cast<const char*>(&tid64), sizeof(uint64_t));
    outputFile.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
    outputFile.write(reinterpret_cast<const char*>(buffer.data()),
                     size * sizeof(uint64_t));

    buffer.clear();
    PIN_ReleaseLock(&outputLock);
}

void writeToFileIfFilledBuffer(THREADID tid,
                               std::vector<std::uint64_t>& buffer) {
    if (buffer.size() >= kBufferSize) {
        WriteToFile(tid, buffer);
    }
}

void ThreadStart(THREADID tid, [[maybe_unused]] CONTEXT* ctxt,
                 [[maybe_unused]] INT32 flags, [[maybe_unused]] void* v) {
    auto* buffer = new std::vector<std::uint64_t>();
    buffer->reserve(kBufferSize);
    PIN_SetThreadData(tlsKey, buffer, tid);
    std::cerr << "[PIN] Thread " << tid << " created." << std::endl;
}

void TrackMemoryRead(THREADID tid, void* addr) {
    auto* buffer = static_cast<std::vector<std::uint64_t>*>(
        PIN_GetThreadData(tlsKey, tid));
    buffer->push_back(static_cast<std::uint64_t>(EntryKind::Read)
                      | (reinterpret_cast<std::uint64_t>(addr) & kPayloadMask));
    writeToFileIfFilledBuffer(tid, *buffer);
}

void TrackMemoryWrite(THREADID tid, void* addr) {
    auto* buffer = static_cast<std::vector<std::uint64_t>*>(
        PIN_GetThreadData(tlsKey, tid));
    buffer->push_back(static_cast<std::uint64_t>(EntryKind::Write)
                      | (reinterpret_cast<std::uint64_t>(addr) & kPayloadMask));
    writeToFileIfFilledBuffer(tid, *buffer);
}

void SyscallEntry(THREADID tid, [[maybe_unused]] CONTEXT* ctxt,
                  [[maybe_unused]] SYSCALL_STANDARD std,
                  [[maybe_unused]] void* v) {
    auto* buffer = static_cast<std::vector<std::uint64_t>*>(
        PIN_GetThreadData(tlsKey, tid));
    std::uint64_t syscallNumber = PIN_GetSyscallNumber(ctxt, std);
    buffer->push_back(static_cast<std::uint64_t>(EntryKind::Syscall)
                      | (syscallNumber & kPayloadMask));
}

void ThreadFini(THREADID tid, [[maybe_unused]] const CONTEXT* ctxt,
                [[maybe_unused]] INT32 code, [[maybe_unused]] void* v) {
    auto* buffer = static_cast<std::vector<std::uint64_t>*>(
        PIN_GetThreadData(tlsKey, tid));

    std::cerr << "[PIN] Finalizing thread " << tid << "." << std::endl;

    if (buffer != nullptr) {
        WriteToFile(tid, *buffer);
        delete buffer;
    }
}

void Instruction(INS ins, [[maybe_unused]] void* v) {
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
        if (INS_MemoryOperandIsRead(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(TrackMemoryRead),
                IARG_THREAD_ID, IARG_MEMORYOP_EA, memOp, IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memOp)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, reinterpret_cast<AFUNPTR>(TrackMemoryWrite),
                IARG_THREAD_ID, IARG_MEMORYOP_EA, memOp, IARG_END);
        }
    }
}

void Fini([[maybe_unused]] int32_t code, [[maybe_unused]] void* v) {
    std::cout << "[PIN] Program finished execution. Finalizing output..."
              << std::endl;
    PIN_GetLock(&outputLock, 0);
    outputFile.close();
    PIN_ReleaseLock(&outputLock);
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

    PIN_InitLock(&outputLock);
    tlsKey = PIN_CreateThreadDataKey(nullptr);
    outputFile.open(knobOutputFile.Value(), std::ios::binary);

    std::cerr << "[PIN] Starting to log accesses and syscalls." << std::endl;

    INS_AddInstrumentFunction(Instruction, nullptr);
    PIN_AddSyscallEntryFunction(SyscallEntry, nullptr);
    PIN_AddThreadStartFunction(ThreadStart, nullptr);
    PIN_AddThreadFiniFunction(ThreadFini, nullptr);
    PIN_AddFiniFunction(Fini, nullptr);
    PIN_StartProgram();
}
