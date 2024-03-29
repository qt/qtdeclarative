// Copyright (C) Rolland Dudemaine All rights reserved.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "config.h"
#include "OSAllocator.h"

#include <INTEGRITY.h>
#include <memory_region.h>
#include <set>
#include <wtf/Assertions.h>
#include <wtf/UnusedParam.h>

#define ASP_PAGESIZE    0x1000

namespace WTF {
struct MRPair {
    mutable MemoryRegion    pmr;
    mutable MemoryRegion    vmr;

    mutable bool mapped;

    Address start;

    MRPair(Address _start = 0) :
        pmr(0),
        vmr(0),
        mapped(false),
        start(_start)
    {}

    bool operator<(const MRPair& rhs) const
    {
        return this->start < rhs.start;
    }
};

class MRContainer
{
private:
    std::set<MRPair>        mrset;
    LocalMutex              iteratorGuard;
public:
    MRContainer() {
        CheckSuccess(CreateLocalMutex(&iteratorGuard));
    }
    const MRPair* getMRPair(Address start) {
        WaitForLocalMutex(iteratorGuard);
        auto pairIterator = mrset.find(MRPair(start));
        const MRPair* result = ((pairIterator == mrset.end()) ? NULL : &(*pairIterator));
        ReleaseLocalMutex(iteratorGuard);
        return result;
    }
    Error deleteMRPair(const MRPair* pair) {
        int erased = 0;
        WaitForLocalMutex(iteratorGuard);
        erased = mrset.erase(*pair);
        ReleaseLocalMutex(iteratorGuard);
        if(erased == 1)
            return Success;
        else
            return ArgumentError;   /* An exception could be thrown in this case */
    }
    Error insertMRPair(MRPair* pair) {
        WaitForLocalMutex(iteratorGuard);
        auto inserted = mrset.insert(*pair);
        ReleaseLocalMutex(iteratorGuard);
        if(inserted.second == true)
            return Success;
        else
            return Failure;         /* An exception could be thrown in this case */
    }
    ~MRContainer() {
        CheckSuccess(CloseLocalMutex(iteratorGuard));
    }
};

static MRContainer memoryRegionsContainer;

Error setAttributes(MemoryRegion mr, bool writable, bool executable)
{
    Value attributes = MEMORY_READ;
    if(writable)
        attributes |= MEMORY_WRITE;
    if(executable)
        attributes |= MEMORY_EXEC;
    return SetMemoryRegionAttributes(mr, attributes);
}

void OSAllocator::setMemoryAttributes(void* addr, size_t size, bool writable, bool executable)
{
    Address addressIterator = Address(addr);
    for(int i=0; i<(size + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++) {
        const MRPair* pair = memoryRegionsContainer.getMRPair(addressIterator);
        CheckSuccess(setAttributes(pair->vmr, writable, executable));
        addressIterator += ASP_PAGESIZE;
    }
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage usage, bool writable, bool executable)
{
    MemoryRegion VMR;

    Address virtualStart, length;

    CheckSuccess(AllocateAnyMemoryRegion(__ghs_VirtualMemoryRegionPool, bytes, &VMR));
    CheckSuccess(GetMemoryRegionAddresses(VMR, &virtualStart, &length));
    Address addressIterator = virtualStart;
    for(int i=0; i<(bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++) {
        MRPair pair;
        pair.start = addressIterator;
        CheckSuccess(SplitMemoryRegion(VMR, ASP_PAGESIZE, &pair.vmr));
        CheckSuccess(setAttributes(pair.vmr, writable, executable));

        memoryRegionsContainer.insertMRPair(&pair);
        addressIterator += ASP_PAGESIZE;
    }

    CheckSuccess(CloseMemoryRegion(VMR));
    return (void*)virtualStart;
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool writable, bool executable, bool includesGuardPages)
{
    MemoryRegion VMR;

    Address virtualStart, length;

    CheckSuccess(AllocateAnyMemoryRegion(__ghs_VirtualMemoryRegionPool, bytes, &VMR));
    CheckSuccess(GetMemoryRegionAddresses(VMR, &virtualStart, &length));

    Address addressIterator = virtualStart;
    for(int i=0; i<(bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++) {
        MRPair pair;
        pair.start = addressIterator;
        CheckSuccess(SplitMemoryRegion(VMR, ASP_PAGESIZE, &pair.vmr));
        CheckSuccess(setAttributes(pair.vmr, writable, executable));
        /* Do not map the first and the last pages if guard pages are required */
        if(!includesGuardPages || (i!=0 && i!= (bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE -1))
        {
            CheckSuccess(GetPageFromAddressSpaceFreeList(GetCurrentAddressSpace(), &pair.pmr));
            CheckSuccess(MapMemoryRegion(pair.vmr, pair.pmr));
            pair.mapped = true;
        }

        memoryRegionsContainer.insertMRPair(&pair);
        addressIterator += ASP_PAGESIZE;
    }

    CheckSuccess(CloseMemoryRegion(VMR));
    return (void*)virtualStart;
}

void OSAllocator::commit(void* address, size_t bytes, bool writable, bool executable)
{
    for(int i=0; i<(bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++)
    {
        const MRPair* pair = memoryRegionsContainer.getMRPair((Address)address);
        if(pair == NULL)
            return;
        CheckSuccess(setAttributes(pair->vmr, writable, executable));
        CheckSuccess(GetPageFromAddressSpaceFreeList(GetCurrentAddressSpace(), &pair->pmr));
        CheckSuccess(MapMemoryRegion(pair->vmr, pair->pmr));
        pair->mapped = true;
        address = (char*)address +  ASP_PAGESIZE;
    }
}

void OSAllocator::decommit(void* address, size_t bytes)
{
    for(int i=0; i<(bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++)
    {
        const MRPair* pair = memoryRegionsContainer.getMRPair((Address)address);
        if(pair == NULL)
            return;
        if(pair->mapped == false)
            continue;

        CheckSuccess(UnmapMemoryRegion(pair->vmr));
        CheckSuccess(PutPageOnAddressSpaceFreeList(GetCurrentAddressSpace(), pair->pmr));
        pair->mapped = false;
        address = (char*)address + ASP_PAGESIZE;
    }
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    for(int i=0; i<(bytes + ASP_PAGESIZE -1)/ASP_PAGESIZE; i++)
    {
        const MRPair* pair = memoryRegionsContainer.getMRPair((Address)address);
        if(pair == NULL)
            return;
        /* Check if the memory is still committed */
        if(pair->mapped == true)
        {
            CheckSuccess(UnmapMemoryRegion(pair->vmr));
            CheckSuccess(PutPageOnAddressSpaceFreeList(GetCurrentAddressSpace(), pair->pmr));
            pair->mapped = false;
        }
        CheckSuccess(AddToMemoryPool(__ghs_VirtualMemoryRegionPool, pair->vmr));
        address = (char*)address + ASP_PAGESIZE;

        memoryRegionsContainer.deleteMRPair(pair);
    }
}

bool OSAllocator::canAllocateExecutableMemory()
{
    return true;
}

} // namespace WTF
