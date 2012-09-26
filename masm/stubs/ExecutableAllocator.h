#ifndef MASM_EXECUTABLEALLOCATOR_H
#define MASM_EXECUTABLEALLOCATOR_H

#include <RefPtr.h>
#include <RefCounted.h>

#include <sys/mman.h>

namespace JSC {

struct JSGlobalData;

struct ExecutableMemoryHandle : public RefCounted<ExecutableMemoryHandle> {
    ExecutableMemoryHandle(int size)
        : m_size(size)
    {
        m_data = malloc(m_size);
    }
    ~ExecutableMemoryHandle()
    {
        free(m_data);
    }

    void* start() { return m_data; }
    int sizeInBytes() { return m_size; }

    void* m_data;
    int m_size;
};

struct ExecutableAllocator {
    PassRefPtr<ExecutableMemoryHandle> allocate(JSGlobalData&, int size, void*, int)
    {
        return adoptRef(new ExecutableMemoryHandle(size));
    }

    static void makeWritable(void*, int)
    {
    }

    static void makeExecutable(void* addr, int size)
    {
        size_t pageSize = sysconf(_SC_PAGESIZE);
        size_t iaddr = reinterpret_cast<size_t>(addr);
        size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
        int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
        mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode);
    }
};

}

#endif // MASM_EXECUTABLEALLOCATOR_H
