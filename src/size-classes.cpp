#include <iostream>
#include <cstdio>

#include <windows.h>

#if !defined(PSAPI_VERSION)
#define PSAPI_VERSION 1
#endif
#include <psapi.h>

std::string GetHeapType() {
    ULONG heapFragValue = 0;
    
    // Use HeapQueryInformation to get heap information.
    BOOL success = HeapQueryInformation(
        GetProcessHeap(), // Handle to the default process heap
        HeapCompatibilityInformation,
        &heapFragValue,
        sizeof(heapFragValue),
        nullptr
    );
    
    if (!success) {
        return "Failed to retrieve heap information.";
    }
    
    switch (heapFragValue) {
        case 0:
            return "Default heap.";
        case 1:
            return "Non-growable heap.";
        case 2:
            return "Low-fragmentation heap (LFH).";
        default:
            return "Unknown heap type.";
    }
}

SIZE_T getHeapMemoryUsed() {
    SIZE_T heapSize = 0;
    DWORD heapCount = GetProcessHeaps(0, NULL);

    if (heapCount == 0) {
        return 0;
    }

    HANDLE* heaps = new HANDLE[heapCount];
    heapCount = GetProcessHeaps(heapCount, heaps);

    for (DWORD i = 0; i < heapCount; i++) {
        PROCESS_HEAP_ENTRY entry;
        entry.lpData = NULL;

        while (HeapWalk(heaps[i], &entry) != 0) {
            if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) {
                // Account for block size and overhead
                heapSize += entry.cbData + entry.cbOverhead;
            }

            // If this is the last block in a region, account for the segment overhead but not block size.
            if (entry.wFlags & PROCESS_HEAP_REGION) {
                heapSize += entry.cbOverhead; // Only account for overhead
            }
        }
    }

    delete[] heaps;
    return heapSize;
}

void PrintMemoryInfo(DWORD processID) {
    const auto iterations = 1'000'000;
    auto objs = new char*[iterations];

    float prevOverhead = (float) INT_MAX;
    auto prevIndex = 0;
    auto hHeap = GetProcessHeap();

    for (auto objectSize = 0; objectSize < 256; objectSize++) {
        auto beforeMem = getHeapMemoryUsed();

        for (int i = 0; i < iterations; i++) {
            objs[i] = new char[objectSize];
            for (auto j = 0; j < objectSize; j++) {
                objs[i][j] = j % 256;
            }
        }

        if (hHeap != NULL) {
            HeapCompact(hHeap, 0);
        }

        auto afterMem = getHeapMemoryUsed();

        auto usedMem = afterMem - beforeMem;
        float overhead = (float) usedMem / (iterations * objectSize);

        if (overhead >= prevOverhead) {
            std::cout << prevIndex << " (overhead = " << prevOverhead << ")" << std::endl;
        }
        prevOverhead = overhead;
        prevIndex = objectSize;

        for (int i = 0; i < iterations; i++) {
            delete[] objs[i];
        }

        if (hHeap != NULL) {
            HeapCompact(hHeap, 0);
        }
    }
}

int main() {
    std::cout << "Heap type: " << GetHeapType() << std::endl;
    PrintMemoryInfo(GetCurrentProcessId());
}
