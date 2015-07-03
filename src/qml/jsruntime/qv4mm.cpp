/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4engine_p.h"
#include "qv4object_p.h"
#include "qv4objectproto_p.h"
#include "qv4mm_p.h"
#include "qv4qobjectwrapper_p.h"
#include <qqmlengine.h>
#include "PageAllocation.h"
#include "StdLibExtras.h"

#include <QTime>
#include <QVector>
#include <QVector>
#include <QMap>

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "qv4alloca_p.h"
#include "qv4profiling_p.h"

#ifdef V4_USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#if OS(QNX)
#include <sys/storage.h>   // __tls()
#endif

#if USE(PTHREADS) && HAVE(PTHREAD_NP_H)
#include <pthread_np.h>
#endif

using namespace WTF;

QT_BEGIN_NAMESPACE

using namespace QV4;

struct MemoryManager::Data
{
    struct ChunkHeader {
        Heap::Base freeItems;
        ChunkHeader *nextNonFull;
        char *itemStart;
        char *itemEnd;
        int itemSize;
    };

    bool gcBlocked;
    bool aggressiveGC;
    bool gcStats;
    ExecutionEngine *engine;

    enum { MaxItemSize = 512 };
    ChunkHeader *nonFullChunks[MaxItemSize/16];
    uint nChunks[MaxItemSize/16];
    uint availableItems[MaxItemSize/16];
    uint allocCount[MaxItemSize/16];
    int totalItems;
    int totalAlloc;
    uint maxShift;
    std::size_t maxChunkSize;
    QVector<PageAllocation> heapChunks;
    std::size_t unmanagedHeapSize; // the amount of bytes of heap that is not managed by the memory manager, but which is held onto by managed items.
    std::size_t unmanagedHeapSizeGCLimit;

    struct LargeItem {
        LargeItem *next;
        size_t size;
        void *data;

        Heap::Base *heapObject() {
            return reinterpret_cast<Heap::Base *>(&data);
        }
    };

    LargeItem *largeItems;
    std::size_t totalLargeItemsAllocated;

    GCDeletable *deletable;

    // statistics:
#ifdef DETAILED_MM_STATS
    QVector<unsigned> allocSizeCounters;
#endif // DETAILED_MM_STATS

    Data()
        : gcBlocked(false)
        , engine(0)
        , totalItems(0)
        , totalAlloc(0)
        , maxShift(6)
        , maxChunkSize(32*1024)
        , unmanagedHeapSize(0)
        , unmanagedHeapSizeGCLimit(64 * 1024)
        , largeItems(0)
        , totalLargeItemsAllocated(0)
        , deletable(0)
    {
        memset(nonFullChunks, 0, sizeof(nonFullChunks));
        memset(nChunks, 0, sizeof(nChunks));
        memset(availableItems, 0, sizeof(availableItems));
        memset(allocCount, 0, sizeof(allocCount));
        aggressiveGC = !qgetenv("QV4_MM_AGGRESSIVE_GC").isEmpty();
        gcStats = !qgetenv("QV4_MM_STATS").isEmpty();

        QByteArray overrideMaxShift = qgetenv("QV4_MM_MAXBLOCK_SHIFT");
        bool ok;
        uint override = overrideMaxShift.toUInt(&ok);
        if (ok && override <= 11 && override > 0)
            maxShift = override;

        QByteArray maxChunkString = qgetenv("QV4_MM_MAX_CHUNK_SIZE");
        std::size_t tmpMaxChunkSize = maxChunkString.toUInt(&ok);
        if (ok)
            maxChunkSize = tmpMaxChunkSize;
    }

    ~Data()
    {
        for (QVector<PageAllocation>::iterator i = heapChunks.begin(), ei = heapChunks.end(); i != ei; ++i) {
            Q_V4_PROFILE_DEALLOC(engine, 0, i->size(), Profiling::HeapPage);
            i->deallocate();
        }
    }
};

namespace {

bool sweepChunk(MemoryManager::Data::ChunkHeader *header, uint *itemsInUse, ExecutionEngine *engine, std::size_t *unmanagedHeapSize)
{
    Q_ASSERT(unmanagedHeapSize);

    bool isEmpty = true;
    Heap::Base *tail = &header->freeItems;
//    qDebug("chunkStart @ %p, size=%x, pos=%x", header->itemStart, header->itemSize, header->itemSize>>4);
#ifdef V4_USE_VALGRIND
    VALGRIND_DISABLE_ERROR_REPORTING;
#endif
    for (char *item = header->itemStart; item <= header->itemEnd; item += header->itemSize) {
        Heap::Base *m = reinterpret_cast<Heap::Base *>(item);
//        qDebug("chunk @ %p, in use: %s, mark bit: %s",
//               item, (m->inUse() ? "yes" : "no"), (m->isMarked() ? "true" : "false"));

        Q_ASSERT((qintptr) item % 16 == 0);

        if (m->isMarked()) {
            Q_ASSERT(m->inUse());
            m->clearMarkBit();
            isEmpty = false;
            ++(*itemsInUse);
        } else {
            if (m->inUse()) {
//                qDebug() << "-- collecting it." << m << tail << m->nextFree();
#ifdef V4_USE_VALGRIND
                VALGRIND_ENABLE_ERROR_REPORTING;
#endif
                if (std::size_t(header->itemSize) == MemoryManager::align(sizeof(Heap::String)) && m->gcGetVtable()->isString) {
                    std::size_t heapBytes = static_cast<Heap::String *>(m)->retainedTextSize();
                    Q_ASSERT(*unmanagedHeapSize >= heapBytes);
//                    qDebug() << "-- it's a string holding on to" << heapBytes << "bytes";
                    *unmanagedHeapSize -= heapBytes;
                }

                if (m->gcGetVtable()->destroy)
                    m->gcGetVtable()->destroy(m);

                memset(m, 0, header->itemSize);
#ifdef V4_USE_VALGRIND
                VALGRIND_DISABLE_ERROR_REPORTING;
                VALGRIND_MEMPOOL_FREE(engine->memoryManager, m);
#endif
                Q_V4_PROFILE_DEALLOC(engine, m, header->itemSize, Profiling::SmallItem);
                ++(*itemsInUse);
            }
            // Relink all free blocks to rewrite references to any released chunk.
            tail->setNextFree(m);
            tail = m;
        }
    }
    tail->setNextFree(0);
#ifdef V4_USE_VALGRIND
    VALGRIND_ENABLE_ERROR_REPORTING;
#endif
    return isEmpty;
}

} // namespace

MemoryManager::MemoryManager(ExecutionEngine *engine)
    : m_d(new Data)
    , m_persistentValues(new PersistentValueStorage(engine))
    , m_weakValues(new PersistentValueStorage(engine))
{
#ifdef V4_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(this, 0, true);
#endif
    m_d->engine = engine;
}

Heap::Base *MemoryManager::allocData(std::size_t size, std::size_t unmanagedSize)
{
    if (m_d->aggressiveGC)
        runGC();
#ifdef DETAILED_MM_STATS
    willAllocate(size);
#endif // DETAILED_MM_STATS

    Q_ASSERT(size >= 16);
    Q_ASSERT(size % 16 == 0);

//    qDebug() << "unmanagedHeapSize:" << m_d->unmanagedHeapSize << "limit:" << m_d->unmanagedHeapSizeGCLimit << "unmanagedSize:" << unmanagedSize;
    m_d->unmanagedHeapSize += unmanagedSize;
    bool didGCRun = false;
    if (m_d->unmanagedHeapSize > m_d->unmanagedHeapSizeGCLimit) {
        runGC();

        if (m_d->unmanagedHeapSizeGCLimit <= m_d->unmanagedHeapSize)
            m_d->unmanagedHeapSizeGCLimit = std::max(m_d->unmanagedHeapSizeGCLimit, m_d->unmanagedHeapSize) * 2;
        else if (m_d->unmanagedHeapSize * 4 <= m_d->unmanagedHeapSizeGCLimit)
            m_d->unmanagedHeapSizeGCLimit /= 2;
        else if (m_d->unmanagedHeapSizeGCLimit - m_d->unmanagedHeapSize < 5 * unmanagedSize)
            // try preventing running the GC all the time when we're just below the threshold limit and manage to collect just enough to do this one allocation
            m_d->unmanagedHeapSizeGCLimit += std::max(std::size_t(8 * 1024), 5 * unmanagedSize);
        didGCRun = true;
    }

    size_t pos = size >> 4;

    // doesn't fit into a small bucket
    if (size >= MemoryManager::Data::MaxItemSize) {
        if (!didGCRun && m_d->totalLargeItemsAllocated > 8 * 1024 * 1024)
            runGC();

        // we use malloc for this
        MemoryManager::Data::LargeItem *item = static_cast<MemoryManager::Data::LargeItem *>(
                malloc(Q_V4_PROFILE_ALLOC(m_d->engine, size + sizeof(MemoryManager::Data::LargeItem),
                                          Profiling::LargeItem)));
        memset(item, 0, size + sizeof(MemoryManager::Data::LargeItem));
        item->next = m_d->largeItems;
        item->size = size;
        m_d->largeItems = item;
        m_d->totalLargeItemsAllocated += size;
        return item->heapObject();
    }

    Heap::Base *m = 0;
    Data::ChunkHeader *header = m_d->nonFullChunks[pos];
    if (header) {
        m = header->freeItems.nextFree();
        goto found;
    }

    // try to free up space, otherwise allocate
    if (!didGCRun && m_d->allocCount[pos] > (m_d->availableItems[pos] >> 1) && m_d->totalAlloc > (m_d->totalItems >> 1) && !m_d->aggressiveGC) {
        runGC();
        header = m_d->nonFullChunks[pos];
        if (header) {
            m = header->freeItems.nextFree();
            goto found;
        }
    }

    // no free item available, allocate a new chunk
    {
        // allocate larger chunks at a time to avoid excessive GC, but cap at maximum chunk size (2MB by default)
        uint shift = ++m_d->nChunks[pos];
        if (shift > m_d->maxShift)
            shift = m_d->maxShift;
        std::size_t allocSize = m_d->maxChunkSize*(size_t(1) << shift);
        allocSize = roundUpToMultipleOf(WTF::pageSize(), allocSize);
        PageAllocation allocation = PageAllocation::allocate(
                    Q_V4_PROFILE_ALLOC(m_d->engine, allocSize, Profiling::HeapPage),
                    OSAllocator::JSGCHeapPages);
        m_d->heapChunks.append(allocation);
        std::sort(m_d->heapChunks.begin(), m_d->heapChunks.end());

        header = reinterpret_cast<Data::ChunkHeader *>(allocation.base());
        header->itemSize = int(size);
        header->itemStart = reinterpret_cast<char *>(allocation.base()) + roundUpToMultipleOf(16, sizeof(Data::ChunkHeader));
        header->itemEnd = reinterpret_cast<char *>(allocation.base()) + allocation.size() - header->itemSize;

        header->nextNonFull = m_d->nonFullChunks[pos];
        m_d->nonFullChunks[pos] = header;

        Heap::Base *last = &header->freeItems;
        for (char *item = header->itemStart; item <= header->itemEnd; item += header->itemSize) {
            Heap::Base *o = reinterpret_cast<Heap::Base *>(item);
            last->setNextFree(o);
            last = o;

        }
        last->setNextFree(0);
        m = header->freeItems.nextFree();
        const size_t increase = (header->itemEnd - header->itemStart) / header->itemSize;
        m_d->availableItems[pos] += uint(increase);
        m_d->totalItems += int(increase);
#ifdef V4_USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(allocation.base(), allocSize);
        VALGRIND_MEMPOOL_ALLOC(this, header, sizeof(Data::ChunkHeader));
#endif
    }

  found:
#ifdef V4_USE_VALGRIND
    VALGRIND_MEMPOOL_ALLOC(this, m, size);
#endif
    Q_V4_PROFILE_ALLOC(m_d->engine, size, Profiling::SmallItem);

    ++m_d->allocCount[pos];
    ++m_d->totalAlloc;
    header->freeItems.setNextFree(m->nextFree());
    if (!header->freeItems.nextFree())
        m_d->nonFullChunks[pos] = header->nextNonFull;
    return m;
}

static void drainMarkStack(QV4::ExecutionEngine *engine, Value *markBase)
{
    while (engine->jsStackTop > markBase) {
        Heap::Base *h = engine->popForGC();
        Q_ASSERT (h->gcGetVtable()->markObjects);
        h->gcGetVtable()->markObjects(h, engine);
    }
}

void MemoryManager::mark()
{
    Value *markBase = m_d->engine->jsStackTop;

    m_d->engine->markObjects();

    m_persistentValues->mark(m_d->engine);

    collectFromJSStack();

    // Preserve QObject ownership rules within JavaScript: A parent with c++ ownership
    // keeps all of its children alive in JavaScript.

    // Do this _after_ collectFromStack to ensure that processing the weak
    // managed objects in the loop down there doesn't make then end up as leftovers
    // on the stack and thus always get collected.
    for (PersistentValueStorage::Iterator it = m_weakValues->begin(); it != m_weakValues->end(); ++it) {
        if (!(*it).isManaged())
            continue;
        if ((*it).managed()->d()->gcGetVtable() != QObjectWrapper::staticVTable())
            continue;
        QObjectWrapper *qobjectWrapper = static_cast<QObjectWrapper*>((*it).managed());
        if (!qobjectWrapper)
            continue;
        QObject *qobject = qobjectWrapper->object();
        if (!qobject)
            continue;
        bool keepAlive = QQmlData::keepAliveDuringGarbageCollection(qobject);

        if (!keepAlive) {
            if (QObject *parent = qobject->parent()) {
                while (parent->parent())
                    parent = parent->parent();

                keepAlive = QQmlData::keepAliveDuringGarbageCollection(parent);
            }
        }

        if (keepAlive)
            qobjectWrapper->mark(m_d->engine);

        if (m_d->engine->jsStackTop >= m_d->engine->jsStackLimit)
            drainMarkStack(m_d->engine, markBase);
    }

    drainMarkStack(m_d->engine, markBase);
}

void MemoryManager::sweep(bool lastSweep)
{
    if (m_weakValues) {
        for (PersistentValueStorage::Iterator it = m_weakValues->begin(); it != m_weakValues->end(); ++it) {
            if (Managed *m = (*it).asManaged()) {
                if (!m->markBit())
                    (*it) = Primitive::undefinedValue();
            }
        }
    }

    if (MultiplyWrappedQObjectMap *multiplyWrappedQObjects = m_d->engine->m_multiplyWrappedQObjects) {
        for (MultiplyWrappedQObjectMap::Iterator it = multiplyWrappedQObjects->begin(); it != multiplyWrappedQObjects->end();) {
            if (!it.value().isNullOrUndefined())
                it = multiplyWrappedQObjects->erase(it);
            else
                ++it;
        }
    }

    bool *chunkIsEmpty = (bool *)alloca(m_d->heapChunks.size() * sizeof(bool));
    uint itemsInUse[MemoryManager::Data::MaxItemSize/16];
    memset(itemsInUse, 0, sizeof(itemsInUse));
    memset(m_d->nonFullChunks, 0, sizeof(m_d->nonFullChunks));

    for (int i = 0; i < m_d->heapChunks.size(); ++i) {
        Data::ChunkHeader *header = reinterpret_cast<Data::ChunkHeader *>(m_d->heapChunks[i].base());
        chunkIsEmpty[i] = sweepChunk(header, &itemsInUse[header->itemSize >> 4], m_d->engine, &m_d->unmanagedHeapSize);
    }

    QVector<PageAllocation>::iterator chunkIter = m_d->heapChunks.begin();
    for (int i = 0; i < m_d->heapChunks.size(); ++i) {
        Q_ASSERT(chunkIter != m_d->heapChunks.end());
        Data::ChunkHeader *header = reinterpret_cast<Data::ChunkHeader *>(chunkIter->base());
        const size_t pos = header->itemSize >> 4;
        const size_t decrease = (header->itemEnd - header->itemStart) / header->itemSize;

        // Release that chunk if it could have been spared since the last GC run without any difference.
        if (chunkIsEmpty[i] && m_d->availableItems[pos] - decrease >= itemsInUse[pos]) {
            Q_V4_PROFILE_DEALLOC(m_d->engine, 0, chunkIter->size(), Profiling::HeapPage);
#ifdef V4_USE_VALGRIND
            VALGRIND_MEMPOOL_FREE(this, header);
#endif
            --m_d->nChunks[pos];
            m_d->availableItems[pos] -= uint(decrease);
            m_d->totalItems -= int(decrease);
            chunkIter->deallocate();
            chunkIter = m_d->heapChunks.erase(chunkIter);
            continue;
        } else if (header->freeItems.nextFree()) {
            header->nextNonFull = m_d->nonFullChunks[pos];
            m_d->nonFullChunks[pos] = header;
        }
        ++chunkIter;
    }

    Data::LargeItem *i = m_d->largeItems;
    Data::LargeItem **last = &m_d->largeItems;
    while (i) {
        Heap::Base *m = i->heapObject();
        Q_ASSERT(m->inUse());
        if (m->isMarked()) {
            m->clearMarkBit();
            last = &i->next;
            i = i->next;
            continue;
        }
        if (m->gcGetVtable()->destroy)
            m->gcGetVtable()->destroy(m);

        *last = i->next;
        free(Q_V4_PROFILE_DEALLOC(m_d->engine, i, i->size + sizeof(Data::LargeItem),
                                  Profiling::LargeItem));
        i = *last;
    }

    GCDeletable *deletable = m_d->deletable;
    m_d->deletable = 0;
    while (deletable) {
        GCDeletable *next = deletable->next;
        deletable->lastCall = lastSweep;
        delete deletable;
        deletable = next;
    }

    // some execution contexts are allocated on the stack, make sure we clear their markBit as well
    if (!lastSweep) {
        Heap::ExecutionContext *ctx = engine()->current;
        while (ctx) {
            ctx->clearMarkBit();
            ctx = ctx->parent;
        }
    }
}

bool MemoryManager::isGCBlocked() const
{
    return m_d->gcBlocked;
}

void MemoryManager::setGCBlocked(bool blockGC)
{
    m_d->gcBlocked = blockGC;
}

void MemoryManager::runGC()
{
    if (m_d->gcBlocked) {
//        qDebug() << "Not running GC.";
        return;
    }

    if (!m_d->gcStats) {
        mark();
        sweep();
    } else {
        const size_t totalMem = getAllocatedMem();

        QTime t;
        t.start();
        mark();
        int markTime = t.elapsed();
        t.restart();
        const size_t usedBefore = getUsedMem();
        int chunksBefore = m_d->heapChunks.size();
        sweep();
        const size_t usedAfter = getUsedMem();
        int sweepTime = t.elapsed();

        qDebug() << "========== GC ==========";
        qDebug() << "Marked object in" << markTime << "ms.";
        qDebug() << "Sweeped object in" << sweepTime << "ms.";
        qDebug() << "Allocated" << totalMem << "bytes in" << m_d->heapChunks.size() << "chunks.";
        qDebug() << "Used memory before GC:" << usedBefore;
        qDebug() << "Used memory after GC:" << usedAfter;
        qDebug() << "Freed up bytes:" << (usedBefore - usedAfter);
        qDebug() << "Released chunks:" << (chunksBefore - m_d->heapChunks.size());
        qDebug() << "======== End GC ========";
    }

    memset(m_d->allocCount, 0, sizeof(m_d->allocCount));
    m_d->totalAlloc = 0;
    m_d->totalLargeItemsAllocated = 0;
}

size_t MemoryManager::getUsedMem() const
{
    size_t usedMem = 0;
    for (QVector<PageAllocation>::const_iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i) {
        Data::ChunkHeader *header = reinterpret_cast<Data::ChunkHeader *>(i->base());
        for (char *item = header->itemStart; item <= header->itemEnd; item += header->itemSize) {
            Heap::Base *m = reinterpret_cast<Heap::Base *>(item);
            Q_ASSERT((qintptr) item % 16 == 0);
            if (m->inUse())
                usedMem += header->itemSize;
        }
    }
    return usedMem;
}

size_t MemoryManager::getAllocatedMem() const
{
    size_t total = 0;
    for (int i = 0; i < m_d->heapChunks.size(); ++i)
        total += m_d->heapChunks.at(i).size();
    return total;
}

size_t MemoryManager::getLargeItemsMem() const
{
    size_t total = 0;
    for (const Data::LargeItem *i = m_d->largeItems; i != 0; i = i->next)
        total += i->size;
    return total;
}

void MemoryManager::growUnmanagedHeapSizeUsage(size_t delta)
{
    m_d->unmanagedHeapSize += delta;
}

MemoryManager::~MemoryManager()
{
    delete m_persistentValues;
    delete m_weakValues;
    m_weakValues = 0;

    sweep(/*lastSweep*/true);
#ifdef V4_USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(this);
#endif
}

ExecutionEngine *MemoryManager::engine() const
{
    return m_d->engine;
}

void MemoryManager::dumpStats() const
{
#ifdef DETAILED_MM_STATS
    std::cerr << "=================" << std::endl;
    std::cerr << "Allocation stats:" << std::endl;
    std::cerr << "Requests for each chunk size:" << std::endl;
    for (int i = 0; i < m_d->allocSizeCounters.size(); ++i) {
        if (unsigned count = m_d->allocSizeCounters[i]) {
            std::cerr << "\t" << (i << 4) << " bytes chunks: " << count << std::endl;
        }
    }
#endif // DETAILED_MM_STATS
}

void MemoryManager::registerDeletable(GCDeletable *d)
{
    d->next = m_d->deletable;
    m_d->deletable = d;
}

#ifdef DETAILED_MM_STATS
void MemoryManager::willAllocate(std::size_t size)
{
    unsigned alignedSize = (size + 15) >> 4;
    QVector<unsigned> &counters = m_d->allocSizeCounters;
    if ((unsigned) counters.size() < alignedSize + 1)
        counters.resize(alignedSize + 1);
    counters[alignedSize]++;
}

#endif // DETAILED_MM_STATS

void MemoryManager::collectFromJSStack() const
{
    Value *v = m_d->engine->jsStackBase;
    Value *top = m_d->engine->jsStackTop;
    while (v < top) {
        Managed *m = v->asManaged();
        if (m && m->inUse())
            // Skip pointers to already freed objects, they are bogus as well
            m->mark(m_d->engine);
        ++v;
    }
}
QT_END_NAMESPACE
