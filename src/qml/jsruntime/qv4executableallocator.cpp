// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4executableallocator_p.h"
#include <QtQml/private/qv4functiontable_p.h>

#include <wtf/StdLibExtras.h>
#include <wtf/PageAllocation.h>

using namespace QV4;

void *ExecutableAllocator::Allocation::exceptionHandlerStart() const
{
    return reinterpret_cast<void*>(addr);
}

size_t ExecutableAllocator::Allocation::exceptionHandlerSize() const
{
    return QV4::exceptionHandlerSize();
}

void *ExecutableAllocator::Allocation::memoryStart() const
{
    return reinterpret_cast<void*>(addr);
}

void *ExecutableAllocator::Allocation::codeStart() const
{
    return reinterpret_cast<void*>(addr + exceptionHandlerSize());
}

void ExecutableAllocator::Allocation::deallocate(ExecutableAllocator *allocator)
{
    if (isValid())
        allocator->free(this);
    else
        delete this;
}

ExecutableAllocator::Allocation *ExecutableAllocator::Allocation::split(size_t dividingSize)
{
    Allocation *remainder = new Allocation;
    if (next)
        next->prev = remainder;

    remainder->next = next;
    next = remainder;

    remainder->prev = this;

    remainder->size = size - dividingSize;
    remainder->free = free;
    remainder->addr = addr + dividingSize;
    size = dividingSize;

    return remainder;
}

bool ExecutableAllocator::Allocation::mergeNext(ExecutableAllocator *allocator)
{
    Q_ASSERT(free);
    if (!next || !next->free)
        return false;

    allocator->freeAllocations.remove(size, this);
    allocator->freeAllocations.remove(next->size, next);

    size += next->size;
    Allocation *newNext = next->next;
    delete next;
    next = newNext;
    if (next)
        next->prev = this;

    allocator->freeAllocations.insert(size, this);
    return true;
}

bool ExecutableAllocator::Allocation::mergePrevious(ExecutableAllocator *allocator)
{
    Q_ASSERT(free);
    if (!prev || !prev->free)
        return false;

    allocator->freeAllocations.remove(size, this);
    allocator->freeAllocations.remove(prev->size, prev);

    prev->size += size;
    if (next)
        next->prev = prev;
    prev->next = next;

    allocator->freeAllocations.insert(prev->size, prev);

    delete this;
    return true;
}

ExecutableAllocator::ChunkOfPages::~ChunkOfPages()
{
    Allocation *alloc = firstAllocation;
    while (alloc) {
        Allocation *next = alloc->next;
        if (alloc->isValid())
            delete alloc;
        alloc = next;
    }
    pages->deallocate();
    delete pages;
}

bool ExecutableAllocator::ChunkOfPages::contains(Allocation *alloc) const
{
    Allocation *it = firstAllocation;
    while (it) {
        if (it == alloc)
            return true;
        it = it->next;
    }
    return false;
}

ExecutableAllocator::ExecutableAllocator()
    = default;

ExecutableAllocator::~ExecutableAllocator()
{
    for (ChunkOfPages *chunk : std::as_const(chunks)) {
        for (Allocation *allocation = chunk->firstAllocation; allocation; allocation = allocation->next)
            if (!allocation->free)
                allocation->invalidate();
    }

    qDeleteAll(chunks);
}

ExecutableAllocator::Allocation *ExecutableAllocator::allocate(size_t size)
{
    QMutexLocker locker(&mutex);
    Allocation *allocation = nullptr;

    // Code is best aligned to 16-byte boundaries.
    size = WTF::roundUpToMultipleOf(16, size + exceptionHandlerSize());

    QMultiMap<size_t, Allocation*>::Iterator it = freeAllocations.lowerBound(size);
    if (it != freeAllocations.end()) {
        allocation = *it;
        freeAllocations.erase(it);
    }

    if (!allocation) {
        ChunkOfPages *chunk = new ChunkOfPages;
        size_t allocSize = WTF::roundUpToMultipleOf(WTF::pageSize(), size);
        chunk->pages = new WTF::PageAllocation(WTF::PageAllocation::allocate(allocSize, OSAllocator::JSJITCodePages));
        chunks.insert(reinterpret_cast<quintptr>(chunk->pages->base()) - 1, chunk);
        allocation = new Allocation;
        allocation->addr = reinterpret_cast<quintptr>(chunk->pages->base());
        allocation->size = allocSize;
        allocation->free = true;
        chunk->firstAllocation = allocation;
    }

    Q_ASSERT(allocation);
    Q_ASSERT(allocation->free);

    allocation->free = false;

    if (allocation->size > size) {
        Allocation *remainder = allocation->split(size);
        remainder->free = true;
        if (!remainder->mergeNext(this))
            freeAllocations.insert(remainder->size, remainder);
    }

    return allocation;
}

void ExecutableAllocator::free(Allocation *allocation)
{
    QMutexLocker locker(&mutex);

    Q_ASSERT(allocation);

    allocation->free = true;

    QMap<quintptr, ChunkOfPages*>::Iterator it = chunks.lowerBound(allocation->addr);
    if (it != chunks.begin())
        --it;
    Q_ASSERT(it != chunks.end());
    ChunkOfPages *chunk = *it;
    Q_ASSERT(chunk->contains(allocation));

    bool merged = allocation->mergeNext(this);
    merged |= allocation->mergePrevious(this);
    if (!merged)
        freeAllocations.insert(allocation->size, allocation);

    allocation = nullptr;

    if (!chunk->firstAllocation->next) {
        freeAllocations.remove(chunk->firstAllocation->size, chunk->firstAllocation);
        chunks.erase(it);
        delete chunk;
        return;
    }
}

ExecutableAllocator::ChunkOfPages *ExecutableAllocator::chunkForAllocation(Allocation *allocation) const
{
    QMutexLocker locker(&mutex);

    QMap<quintptr, ChunkOfPages*>::ConstIterator it = chunks.lowerBound(allocation->addr);
    if (it != chunks.begin())
        --it;
    if (it == chunks.end())
        return nullptr;
    return *it;
}

