/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

QT_BEGIN_NAMESPACE

using namespace QV4;
using namespace WTF;

struct MemoryManager::Data
{
    bool gcBlocked;
    bool aggressiveGC;
    bool gcStats;
    ExecutionEngine *engine;

    enum { MaxItemSize = 512 };
    Managed *smallItems[MaxItemSize/16];
    uint nChunks[MaxItemSize/16];
    uint availableItems[MaxItemSize/16];
    uint allocCount[MaxItemSize/16];
    int totalItems;
    int totalAlloc;
    uint maxShift;
    std::size_t maxChunkSize;
    struct Chunk {
        PageAllocation memory;
        int chunkSize;
    };

    QVector<Chunk> heapChunks;


    struct LargeItem {
        LargeItem *next;
        void *data;

        Managed *managed() {
            return reinterpret_cast<Managed *>(&data);
        }
    };

    LargeItem *largeItems;

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
        , largeItems(0)
        , deletable(0)
    {
        memset(smallItems, 0, sizeof(smallItems));
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
        for (QVector<Chunk>::iterator i = heapChunks.begin(), ei = heapChunks.end(); i != ei; ++i)
            i->memory.deallocate();
    }
};


namespace QV4 {

bool operator<(const MemoryManager::Data::Chunk &a, const MemoryManager::Data::Chunk &b)
{
    return a.memory.base() < b.memory.base();
}

} // namespace QV4

MemoryManager::MemoryManager()
    : m_d(new Data)
    , m_persistentValues(0)
    , m_weakValues(0)
{
#ifdef V4_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(this, 0, true);
#endif
}

Managed *MemoryManager::alloc(std::size_t size)
{
    if (m_d->aggressiveGC)
        runGC();
#ifdef DETAILED_MM_STATS
    willAllocate(size);
#endif // DETAILED_MM_STATS

    Q_ASSERT(size >= 16);
    Q_ASSERT(size % 16 == 0);

    size_t pos = size >> 4;

    // doesn't fit into a small bucket
    if (size >= MemoryManager::Data::MaxItemSize) {
        // we use malloc for this
        MemoryManager::Data::LargeItem *item = static_cast<MemoryManager::Data::LargeItem *>(malloc(size + sizeof(MemoryManager::Data::LargeItem)));
        memset(item, 0, size + sizeof(MemoryManager::Data::LargeItem));
        item->next = m_d->largeItems;
        m_d->largeItems = item;
        return item->managed();
    }

    Managed *m = m_d->smallItems[pos];
    if (m)
        goto found;

    // try to free up space, otherwise allocate
    if (m_d->allocCount[pos] > (m_d->availableItems[pos] >> 1) && m_d->totalAlloc > (m_d->totalItems >> 1) && !m_d->aggressiveGC) {
        runGC();
        m = m_d->smallItems[pos];
        if (m)
            goto found;
    }

    // no free item available, allocate a new chunk
    {
        // allocate larger chunks at a time to avoid excessive GC, but cap at maximum chunk size (2MB by default)
        uint shift = ++m_d->nChunks[pos];
        if (shift > m_d->maxShift)
            shift = m_d->maxShift;
        std::size_t allocSize = m_d->maxChunkSize*(size_t(1) << shift);
        allocSize = roundUpToMultipleOf(WTF::pageSize(), allocSize);
        Data::Chunk allocation;
        allocation.memory = PageAllocation::allocate(allocSize, OSAllocator::JSGCHeapPages);
        allocation.chunkSize = int(size);
        m_d->heapChunks.append(allocation);
        std::sort(m_d->heapChunks.begin(), m_d->heapChunks.end());
        char *chunk = (char *)allocation.memory.base();
        char *end = chunk + allocation.memory.size() - size;

        Managed **last = &m_d->smallItems[pos];
        while (chunk <= end) {
            Managed *o = reinterpret_cast<Managed *>(chunk);
            o->_data = 0;
            *last = o;
            last = o->nextFreeRef();
            chunk += size;
        }
        *last = 0;
        m = m_d->smallItems[pos];
        const size_t increase = allocation.memory.size()/size - 1;
        m_d->availableItems[pos] += uint(increase);
        m_d->totalItems += int(increase);
#ifdef V4_USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(allocation.memory, allocation.chunkSize);
#endif
    }

  found:
#ifdef V4_USE_VALGRIND
    VALGRIND_MEMPOOL_ALLOC(this, m, size);
#endif

    ++m_d->allocCount[pos];
    ++m_d->totalAlloc;
    m_d->smallItems[pos] = m->nextFree();
    return m;
}

void MemoryManager::mark()
{
    Value *markBase = m_d->engine->jsStackTop;

    m_d->engine->markObjects();

    PersistentValuePrivate *persistent = m_persistentValues;
    while (persistent) {
        if (!persistent->refcount) {
            PersistentValuePrivate *n = persistent->next;
            persistent->removeFromList();
            delete persistent;
            persistent = n;
            continue;
        }
        persistent->value.mark(m_d->engine);
        persistent = persistent->next;
    }

    collectFromJSStack();

    // Preserve QObject ownership rules within JavaScript: A parent with c++ ownership
    // keeps all of its children alive in JavaScript.

    // Do this _after_ collectFromStack to ensure that processing the weak
    // managed objects in the loop down there doesn't make then end up as leftovers
    // on the stack and thus always get collected.
    for (PersistentValuePrivate *weak = m_weakValues; weak; weak = weak->next) {
        if (!weak->refcount)
            continue;
        Returned<QObjectWrapper> *qobjectWrapper = weak->value.as<QObjectWrapper>();
        if (!qobjectWrapper)
            continue;
        QObject *qobject = qobjectWrapper->getPointer()->object();
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
            qobjectWrapper->getPointer()->mark(m_d->engine);
    }

    // now that we marked all roots, start marking recursively and popping from the mark stack
    while (m_d->engine->jsStackTop > markBase) {
        Managed *m = m_d->engine->popForGC();
        Q_ASSERT (m->internalClass->vtable->markObjects);
        m->internalClass->vtable->markObjects(m, m_d->engine);
    }
}

void MemoryManager::sweep(bool lastSweep)
{
    PersistentValuePrivate *weak = m_weakValues;
    while (weak) {
        if (!weak->refcount) {
            PersistentValuePrivate *n = weak->next;
            weak->removeFromList();
            delete weak;
            weak = n;
            continue;
        }
        if (Managed *m = weak->value.asManaged()) {
            if (!m->markBit) {
                weak->value = Primitive::undefinedValue();
                PersistentValuePrivate *n = weak->next;
                weak->removeFromList();
                weak = n;
                continue;
            }
        }
        weak = weak->next;
    }

    if (MultiplyWrappedQObjectMap *multiplyWrappedQObjects = m_d->engine->m_multiplyWrappedQObjects) {
        for (MultiplyWrappedQObjectMap::Iterator it = multiplyWrappedQObjects->begin(); it != multiplyWrappedQObjects->end();) {
            if (!it.value()->markBit)
                it = multiplyWrappedQObjects->erase(it);
            else
                ++it;
        }
    }

    for (QVector<Data::Chunk>::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i)
        sweep(reinterpret_cast<char*>(i->memory.base()), i->memory.size(), i->chunkSize);

    Data::LargeItem *i = m_d->largeItems;
    Data::LargeItem **last = &m_d->largeItems;
    while (i) {
        Managed *m = i->managed();
        Q_ASSERT(m->inUse);
        if (m->markBit) {
            m->markBit = 0;
            last = &i->next;
            i = i->next;
            continue;
        }
        if (m->internalClass->vtable->destroy)
            m->internalClass->vtable->destroy(m);

        *last = i->next;
        free(i);
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
}

void MemoryManager::sweep(char *chunkStart, std::size_t chunkSize, size_t size)
{
//    qDebug("chunkStart @ %p, size=%x, pos=%x (%x)", chunkStart, size, size>>4, m_d->smallItems[size >> 4]);
    Managed **f = &m_d->smallItems[size >> 4];

#ifdef V4_USE_VALGRIND
    VALGRIND_DISABLE_ERROR_REPORTING;
#endif
    for (char *chunk = chunkStart, *chunkEnd = chunk + chunkSize - size; chunk <= chunkEnd; chunk += size) {
        Managed *m = reinterpret_cast<Managed *>(chunk);
//        qDebug("chunk @ %p, size = %lu, in use: %s, mark bit: %s",
//               chunk, m->size, (m->inUse ? "yes" : "no"), (m->markBit ? "true" : "false"));

        Q_ASSERT((qintptr) chunk % 16 == 0);

        if (m->inUse) {
            if (m->markBit) {
                m->markBit = 0;
            } else {
//                qDebug() << "-- collecting it." << m << *f << m->nextFree();
#ifdef V4_USE_VALGRIND
                VALGRIND_ENABLE_ERROR_REPORTING;
#endif
                if (m->internalClass->vtable->destroy)
                    m->internalClass->vtable->destroy(m);

                memset(m, 0, size);
                m->setNextFree(*f);
#ifdef V4_USE_VALGRIND
                VALGRIND_DISABLE_ERROR_REPORTING;
                VALGRIND_MEMPOOL_FREE(this, m);
#endif
                *f = m;
            }
        }
    }
#ifdef V4_USE_VALGRIND
    VALGRIND_ENABLE_ERROR_REPORTING;
#endif
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
        int totalMem = 0;
        for (int i = 0; i < m_d->heapChunks.size(); ++i)
            totalMem += m_d->heapChunks.at(i).memory.size();

        QTime t;
        t.start();
        mark();
        int markTime = t.elapsed();
        t.restart();
        int usedBefore = getUsedMem();
        sweep();
        int usedAfter = getUsedMem();
        int sweepTime = t.elapsed();

        qDebug() << "========== GC ==========";
        qDebug() << "Marked object in" << markTime << "ms.";
        qDebug() << "Sweeped object in" << sweepTime << "ms.";
        qDebug() << "Allocated" << totalMem << "bytes in" << m_d->heapChunks.size() << "chunks.";
        qDebug() << "Used memory before GC:" << usedBefore;
        qDebug() << "Used memory after GC:" << usedAfter;
        qDebug() << "Freed up bytes:" << (usedBefore - usedAfter);
        qDebug() << "======== End GC ========";
    }

    memset(m_d->allocCount, 0, sizeof(m_d->allocCount));
    m_d->totalAlloc = 0;
}

uint MemoryManager::getUsedMem()
{
    uint usedMem = 0;
    for (QVector<Data::Chunk>::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i) {
        char *chunkStart = reinterpret_cast<char *>(i->memory.base());
        char *chunkEnd = chunkStart + i->memory.size() - i->chunkSize;
        for (char *chunk = chunkStart; chunk <= chunkEnd; chunk += i->chunkSize) {
            Managed *m = reinterpret_cast<Managed *>(chunk);
            Q_ASSERT((qintptr) chunk % 16 == 0);
            if (m->inUse)
                usedMem += i->chunkSize;
        }
    }
    return usedMem;
}

MemoryManager::~MemoryManager()
{
    PersistentValuePrivate *persistent = m_persistentValues;
    while (persistent) {
        PersistentValuePrivate *n = persistent->next;
        persistent->value = Primitive::undefinedValue();
        persistent->engine = 0;
        persistent->prev = 0;
        persistent->next = 0;
        persistent = n;
    }

    sweep(/*lastSweep*/true);
#ifdef V4_USE_VALGRIND
    VALGRIND_DESTROY_MEMPOOL(this);
#endif
}

void MemoryManager::setExecutionEngine(ExecutionEngine *engine)
{
    m_d->engine = engine;
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

ExecutionEngine *MemoryManager::engine() const
{
    return m_d->engine;
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
    Value *v = engine()->jsStackBase;
    Value *top = engine()->jsStackTop;
    while (v < top) {
        Managed *m = v->asManaged();
        if (m && m->inUse)
            // Skip pointers to already freed objects, they are bogus as well
            m->mark(m_d->engine);
        ++v;
    }
}
QT_END_NAMESPACE
