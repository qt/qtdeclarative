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
        static size_t pageSize = sysconf(_SC_PAGESIZE);
        m_size = (m_size + pageSize - 1) & ~(pageSize - 1);
        m_data = mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    ~ExecutableMemoryHandle()
    {
        munmap(m_data, m_size);
    }

    inline void shrink(size_t) {
        // ### TODO.
    }

    inline bool isManaged() const { return true; }

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
        static size_t pageSize = sysconf(_SC_PAGESIZE);
        size_t iaddr = reinterpret_cast<size_t>(addr);
        size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
        int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
        mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode);
    }
};

}

#endif // MASM_EXECUTABLEALLOCATOR_H
