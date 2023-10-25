// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4GC_H
#define QV4GC_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4global_p.h>
#include <private/qv4value_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4object_p.h>
#include <private/qv4mmdefs_p.h>
#include <QVector>

#define MM_DEBUG 0

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ChunkAllocator;
struct MemorySegment;

struct BlockAllocator {
    BlockAllocator(ChunkAllocator *chunkAllocator, ExecutionEngine *engine)
        : chunkAllocator(chunkAllocator), engine(engine)
    {
        memset(freeBins, 0, sizeof(freeBins));
    }

    enum { NumBins = 8 };

    static inline size_t binForSlots(size_t nSlots) {
        return nSlots >= NumBins ? NumBins - 1 : nSlots;
    }

    HeapItem *allocate(size_t size, bool forceAllocation = false);

    size_t totalSlots() const {
        return Chunk::AvailableSlots*chunks.size();
    }

    size_t allocatedMem() const {
        return chunks.size()*Chunk::DataSize;
    }
    size_t usedMem() const {
        uint used = 0;
        for (auto c : chunks)
            used += c->nUsedSlots()*Chunk::SlotSize;
        return used;
    }

    void sweep();
    void freeAll();
    void resetBlackBits();

    // bump allocations
    HeapItem *nextFree = nullptr;
    size_t nFree = 0;
    size_t usedSlotsAfterLastSweep = 0;
    HeapItem *freeBins[NumBins];
    ChunkAllocator *chunkAllocator;
    ExecutionEngine *engine;
    std::vector<Chunk *> chunks;
    uint *allocationStats = nullptr;
};

struct HugeItemAllocator {
    HugeItemAllocator(ChunkAllocator *chunkAllocator, ExecutionEngine *engine)
        : chunkAllocator(chunkAllocator), engine(engine)
    {}

    HeapItem *allocate(size_t size);
    void sweep(ClassDestroyStatsCallback classCountPtr);
    void freeAll();
    void resetBlackBits();

    size_t usedMem() const {
        size_t used = 0;
        for (const auto &c : chunks)
            used += c.size;
        return used;
    }

    ChunkAllocator *chunkAllocator;
    ExecutionEngine *engine;
    struct HugeChunk {
        MemorySegment *segment;
        Chunk *chunk;
        size_t size;
    };

    std::vector<HugeChunk> chunks;
};


class Q_QML_EXPORT MemoryManager
{
    Q_DISABLE_COPY(MemoryManager);

public:
    MemoryManager(ExecutionEngine *engine);
    ~MemoryManager();

    // TODO: this is only for 64bit (and x86 with SSE/AVX), so exend it for other architectures to be slightly more efficient (meaning, align on 8-byte boundaries).
    // Note: all occurrences of "16" in alloc/dealloc are also due to the alignment.
    constexpr static inline std::size_t align(std::size_t size)
    { return (size + Chunk::SlotSize - 1) & ~(Chunk::SlotSize - 1); }

    /* NOTE: allocManaged comes in various overloads. If size is not passed explicitly
       sizeof(ManagedType::Data) is used for size. However, there are quite a few cases
       where we allocate more than sizeof(ManagedType::Data); that's generally the case
       when the Object has a ValueArray member.
       If no internal class pointer is provided, ManagedType::defaultInternalClass(engine)
       will be used as the internal class.
    */

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size, Heap::InternalClass *ic)
    {
        Q_STATIC_ASSERT(std::is_trivial_v<typename ManagedType::Data>);
        size = align(size);
        typename ManagedType::Data *d = static_cast<typename ManagedType::Data *>(allocData(size));
        d->internalClass.set(engine, ic);
        Q_ASSERT(d->internalClass && d->internalClass->vtable);
        Q_ASSERT(ic->vtable == ManagedType::staticVTable());
        return d;
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(Heap::InternalClass *ic)
    {
        return allocManaged<ManagedType>(sizeof(typename ManagedType::Data), ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size, InternalClass *ic)
    {
        return allocManaged<ManagedType>(size, ic->d());
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(InternalClass *ic)
    {
        return allocManaged<ManagedType>(sizeof(typename ManagedType::Data), ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size)
    {
        Scope scope(engine);
        Scoped<InternalClass> ic(scope, ManagedType::defaultInternalClass(engine));
        return allocManaged<ManagedType>(size, ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged()
    {
        auto constexpr size = sizeof(typename ManagedType::Data);
        Scope scope(engine);
        Scoped<InternalClass> ic(scope, ManagedType::defaultInternalClass(engine));
        return allocManaged<ManagedType>(size, ic);
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject(Heap::InternalClass *ic)
    {
        Heap::Object *o = allocObjectWithMemberData(ObjectType::staticVTable(), ic->size);
        o->internalClass.set(engine, ic);
        Q_ASSERT(o->internalClass.get() && o->vtable());
        Q_ASSERT(o->vtable() == ObjectType::staticVTable());
        return static_cast<typename ObjectType::Data *>(o);
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject(InternalClass *ic)
    {
        return allocateObject<ObjectType>(ic->d());
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject()
    {
        Scope scope(engine);
        Scoped<InternalClass> ic(scope,  ObjectType::defaultInternalClass(engine));
        ic = ic->changeVTable(ObjectType::staticVTable());
        ic = ic->changePrototype(ObjectType::defaultPrototype(engine)->d());
        return allocateObject<ObjectType>(ic);
    }

    template <typename ManagedType, typename Arg1>
    typename ManagedType::Data *allocWithStringData(std::size_t unmanagedSize, Arg1 &&arg1)
    {
        typename ManagedType::Data *o = reinterpret_cast<typename ManagedType::Data *>(allocString(unmanagedSize));
        o->internalClass.set(engine, ManagedType::defaultInternalClass(engine));
        Q_ASSERT(o->internalClass && o->internalClass->vtable);
        o->init(std::forward<Arg1>(arg1));
        return o;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocObject(Heap::InternalClass *ic, Args&&... args)
    {
        typename ObjectType::Data *d = allocateObject<ObjectType>(ic);
        d->init(std::forward<Args>(args)...);
        return d;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocObject(InternalClass *ic, Args&&... args)
    {
        typename ObjectType::Data *d = allocateObject<ObjectType>(ic);
        d->init(std::forward<Args>(args)...);
        return d;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocate(Args&&... args)
    {
        Scope scope(engine);
        Scoped<ObjectType> t(scope, allocateObject<ObjectType>());
        t->d_unchecked()->init(std::forward<Args>(args)...);
        return t->d();
    }

    template <typename ManagedType, typename... Args>
    typename ManagedType::Data *alloc(Args&&... args)
    {
        Scope scope(engine);
        Scoped<ManagedType> t(scope, allocManaged<ManagedType>());
        t->d_unchecked()->init(std::forward<Args>(args)...);
        return t->d();
    }

    void runGC();

    void dumpStats() const;

    size_t getUsedMem() const;
    size_t getAllocatedMem() const;
    size_t getLargeItemsMem() const;

    // called when a JS object grows itself. Specifically: Heap::String::append
    // and InternalClassDataPrivate<PropertyAttributes>.
    void changeUnmanagedHeapSizeUsage(qptrdiff delta) { unmanagedHeapSize += delta; }

    template<typename ManagedType>
    typename ManagedType::Data *allocIC()
    {
        Heap::Base *b = *allocate(&icAllocator, align(sizeof(typename ManagedType::Data)));
        return static_cast<typename ManagedType::Data *>(b);
    }

    void registerWeakMap(Heap::MapObject *map);
    void registerWeakSet(Heap::SetObject *set);

protected:
    /// expects size to be aligned
    Heap::Base *allocString(std::size_t unmanagedSize);
    Heap::Base *allocData(std::size_t size);
    Heap::Object *allocObjectWithMemberData(const QV4::VTable *vtable, uint nMembers);

private:
    enum {
        MinUnmanagedHeapSizeGCLimit = 128 * 1024
    };

    void collectFromJSStack(MarkStack *markStack) const;
    void mark();
    void sweep(bool lastSweep = false, ClassDestroyStatsCallback classCountPtr = nullptr);
    bool shouldRunGC() const;
    void collectRoots(MarkStack *markStack);

    HeapItem *allocate(BlockAllocator *allocator, std::size_t size)
    {
        bool didGCRun = false;
        if (aggressiveGC) {
            runGC();
            didGCRun = true;
        }

        if (unmanagedHeapSize > unmanagedHeapSizeGCLimit) {
            if (!didGCRun)
                runGC();

            if (3*unmanagedHeapSizeGCLimit <= 4 * unmanagedHeapSize) {
                // more than 75% full, raise limit
                unmanagedHeapSizeGCLimit = std::max(unmanagedHeapSizeGCLimit,
                                                    unmanagedHeapSize) * 2;
            } else if (unmanagedHeapSize * 4 <= unmanagedHeapSizeGCLimit) {
                // less than 25% full, lower limit
                unmanagedHeapSizeGCLimit = qMax(std::size_t(MinUnmanagedHeapSizeGCLimit),
                                                unmanagedHeapSizeGCLimit/2);
            }
            didGCRun = true;
        }

        if (size > Chunk::DataSize)
            return hugeItemAllocator.allocate(size);

        if (HeapItem *m = allocator->allocate(size))
            return m;

        if (!didGCRun && shouldRunGC())
            runGC();

        return allocator->allocate(size, true);
    }

public:
    QV4::ExecutionEngine *engine;
    ChunkAllocator *chunkAllocator;
    BlockAllocator blockAllocator;
    BlockAllocator icAllocator;
    HugeItemAllocator hugeItemAllocator;
    PersistentValueStorage *m_persistentValues;
    PersistentValueStorage *m_weakValues;
    QVector<Value *> m_pendingFreedObjectWrapperValue;
    Heap::MapObject *weakMaps = nullptr;
    Heap::SetObject *weakSets = nullptr;

    std::size_t unmanagedHeapSize = 0; // the amount of bytes of heap that is not managed by the memory manager, but which is held onto by managed items.
    std::size_t unmanagedHeapSizeGCLimit;
    std::size_t usedSlotsAfterLastFullSweep = 0;

    bool gcBlocked = false;
    bool aggressiveGC = false;
    bool gcStats = false;
    bool gcCollectorStats = false;

    int allocationCount = 0;
    size_t lastAllocRequestedSlots = 0;

    struct {
        size_t maxReservedMem = 0;
        size_t maxAllocatedMem = 0;
        size_t maxUsedMem = 0;
        uint allocations[BlockAllocator::NumBins];
    } statistics;
};

}

QT_END_NAMESPACE

#endif // QV4GC_H
