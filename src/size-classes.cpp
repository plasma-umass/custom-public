#include <iostream>
#include <stdio.h>

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

#if !defined(PSAPI_VERSION)
#define PSAPI_VERSION 1
#endif
#include <psapi.h>
#include <windows.h>

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

int main(void) {
    auto pid = GetCurrentProcessId();
    PrintMemoryInfo(pid);

#if 0
  // Get the list of process identifiers.

    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
    {
        return 1;
    }

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the memory usage for each process

    for ( i = 0; i < cProcesses; i++ )
    {
        PrintMemoryInfo( aProcesses[i] );
    }
#endif
    return 0;
}
