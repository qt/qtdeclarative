/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#include "PageAllocation.h"
#include "StdLibExtras.h"

#include <QTime>
#include <QVector>
#include <QVector>
#include <QMap>

#include <iostream>
#include <cstdlib>
#include "qv4alloca_p.h"

#ifdef V4_USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

using namespace QQmlJS::VM;
using namespace WTF;

static const std::size_t CHUNK_SIZE = 1024*32;

struct MemoryManager::Data
{
    bool enableGC;
    bool gcBlocked;
    bool scribble;
    bool aggressiveGC;
    ExecutionEngine *engine;
    quintptr *stackTop;

    enum { MaxItemSize = 512 };
    Managed *smallItems[MaxItemSize/16];
    uint nChunks[MaxItemSize/16];
    uint availableItems[MaxItemSize/16];
    uint allocCount[MaxItemSize/16];
    struct Chunk {
        PageAllocation memory;
        int chunkSize;
    };

    QVector<Chunk> heapChunks;
    QHash<Managed *, uint> protectedObject;

    // statistics:
#ifdef DETAILED_MM_STATS
    QVector<unsigned> allocSizeCounters;
#endif // DETAILED_MM_STATS

    Data(bool enableGC)
        : enableGC(enableGC)
        , gcBlocked(false)
        , engine(0)
        , stackTop(0)
    {
        memset(smallItems, 0, sizeof(smallItems));
        memset(nChunks, 0, sizeof(nChunks));
        memset(availableItems, 0, sizeof(availableItems));
        memset(allocCount, 0, sizeof(allocCount));
        scribble = !qgetenv("MM_SCRIBBLE").isEmpty();
        aggressiveGC = !qgetenv("MM_AGGRESSIVE_GC").isEmpty();
    }

    ~Data()
    {
        for (QVector<Chunk>::iterator i = heapChunks.begin(), ei = heapChunks.end(); i != ei; ++i)
            i->memory.deallocate();
    }
};

#define SCRIBBLE(obj, c, size) \
    if (m_d->scribble) \
        ::memset((void *)(obj + 1), c, size - sizeof(Managed));


namespace QQmlJS { namespace VM {

bool operator<(const MemoryManager::Data::Chunk &a, const MemoryManager::Data::Chunk &b)
{
    return a.memory.base() < b.memory.base();
}

} } // namespace QQmlJS::VM

MemoryManager::MemoryManager()
    : m_d(new Data(true))
    , m_contextList(0)
    , m_persistentValues(0)
{
    setEnableGC(true);
#ifdef V4_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(this, 0, true);
#endif

#if USE(PTHREADS)
#  if OS(DARWIN)
    void *st = pthread_get_stackaddr_np(pthread_self());
    m_d->stackTop = static_cast<quintptr *>(st);
#  else
    void* stackBottom = 0;
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);
    size_t stackSize = 0;
    pthread_attr_getstack(&attr, &stackBottom, &stackSize);
    pthread_attr_destroy(&attr);

    m_d->stackTop = static_cast<quintptr *>(stackBottom) + stackSize/sizeof(quintptr);
#  endif
#elif OS(WINDOWS)
#  if COMPILER(MSVC)
    PNT_TIB tib = (PNT_TIB)NtCurrentTeb();
    m_d->stackTop = static_cast<quintptr*>(tib->StackBase);
#  else
#    error "Unsupported compiler: no way to get the top-of-stack."
#  endif
#else
#  error "Unsupported platform: no way to get the top-of-stack."
#endif

}

Managed *MemoryManager::alloc(std::size_t size)
{
    if (m_d->aggressiveGC)
        runGC();
#ifdef DETAILED_MM_STATS
    willAllocate(size);
#endif // DETAILED_MM_STATS

    assert(size >= 16);
    assert(size % 16 == 0);

    size_t pos = size >> 4;
    ++m_d->allocCount[pos];

    // fits into a small bucket
    assert(size < MemoryManager::Data::MaxItemSize);

    Managed *m = m_d->smallItems[pos];
    if (m)
        goto found;

    // try to free up space, otherwise allocate
    if (m_d->allocCount[pos] > (m_d->availableItems[pos] >> 1) && !m_d->aggressiveGC) {
        runGC();
        m = m_d->smallItems[pos];
        if (m)
            goto found;
    }

    // no free item available, allocate a new chunk
    {
        // allocate larger chunks at a time to avoid excessive GC, but cap at 64M chunks
        uint shift = ++m_d->nChunks[pos];
        if (shift > 10)
            shift = 10;
        std::size_t allocSize = CHUNK_SIZE*(1 << shift)*size;
        allocSize = roundUpToMultipleOf(WTF::pageSize(), allocSize);
        Data::Chunk allocation;
        allocation.memory = PageAllocation::allocate(allocSize, OSAllocator::JSGCHeapPages);
        allocation.chunkSize = size;
        m_d->heapChunks.append(allocation);
        qSort(m_d->heapChunks);
        char *chunk = (char *)allocation.memory.base();
        char *end = chunk + allocation.memory.size() - size;
        memset(chunk, 0, allocation.memory.size());
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
        m_d->availableItems[pos] += allocation.memory.size()/size - 1;
#ifdef V4_USE_VALGRIND
        VALGRIND_MAKE_MEM_NOACCESS(allocation.memory, allocation.chunkSize);
#endif
    }

  found:
#ifdef V4_USE_VALGRIND
    VALGRIND_MEMPOOL_ALLOC(this, m, size);
#endif

    m_d->smallItems[pos] = m->nextFree();
    return m;
}

void MemoryManager::mark()
{
    m_d->engine->markObjects();

    for (QHash<Managed *, uint>::const_iterator it = m_d->protectedObject.begin(); it != m_d->protectedObject.constEnd(); ++it)
        it.key()->mark();

    PersistentValuePrivate *persistent = m_persistentValues;
    PersistentValuePrivate **last = &m_persistentValues;
    while (persistent) {
        if (!persistent->refcount) {
            *last = persistent->next;
            PersistentValuePrivate *n = persistent->next;
            delete persistent;
            persistent = n;
            continue;
        }
        if (Managed *m = persistent->value.asManaged())
            m->mark();
        last = &persistent->next;
        persistent = persistent->next;
    }

    // push all caller saved registers to the stack, so we can find the objects living in these registers
#if COMPILER(MSVC)
#  if CPU(X86_64)
    HANDLE thread = GetCurrentThread();
    WOW64_CONTEXT ctxt;
    /*bool success =*/ Wow64GetThreadContext(thread, &ctxt);
#  elif CPU(X86)
    HANDLE thread = GetCurrentThread();
    CONTEXT ctxt;
    /*bool success =*/ GetThreadContext(thread, &ctxt);
#  endif // CPU
#elif COMPILER(CLANG) || COMPILER(GCC)
#  if CPU(X86_64)
    quintptr regs[5];
    asm(
        "mov %%rbp, %0\n"
        "mov %%r12, %1\n"
        "mov %%r13, %2\n"
        "mov %%r14, %3\n"
        "mov %%r15, %4\n"
        : "=m" (regs[0]), "=m" (regs[1]), "=m" (regs[2]), "=m" (regs[3]), "=m" (regs[4])
        :
        :
    );
#  endif // CPU
#endif // COMPILER

    collectFromStack();
}

std::size_t MemoryManager::sweep()
{
    std::size_t freedCount = 0;

    for (QVector<Data::Chunk>::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i)
        freedCount += sweep(reinterpret_cast<char*>(i->memory.base()), i->memory.size(), i->chunkSize);

    ExecutionContext *ctx = m_contextList;
    ExecutionContext **n = &m_contextList;
    while (ctx) {
        ExecutionContext *next = ctx->next;
        if (!ctx->marked) {
            free(ctx);
            *n = next;
        } else {
            ctx->marked = false;
            n = &ctx->next;
        }
        ctx = next;
    }

    return freedCount;
}

std::size_t MemoryManager::sweep(char *chunkStart, std::size_t chunkSize, size_t size)
{
//    qDebug("chunkStart @ %p, size=%x, pos=%x (%x)", chunkStart, size, size>>4, m_d->smallItems[size >> 4]);
    std::size_t freedCount = 0;

    Managed **f = &m_d->smallItems[size >> 4];

#ifdef V4_USE_VALGRIND
    VALGRIND_DISABLE_ERROR_REPORTING;
#endif
    for (char *chunk = chunkStart, *chunkEnd = chunk + chunkSize - size; chunk <= chunkEnd; chunk += size) {
        Managed *m = reinterpret_cast<Managed *>(chunk);
//        qDebug("chunk @ %p, size = %lu, in use: %s, mark bit: %s",
//               chunk, m->size, (m->inUse ? "yes" : "no"), (m->markBit ? "true" : "false"));

        assert((intptr_t) chunk % 16 == 0);

        if (m->inUse) {
            if (m->markBit) {
                m->markBit = 0;
            } else {
//                qDebug() << "-- collecting it." << m << *f << m->nextFree();
#ifdef V4_USE_VALGRIND
                VALGRIND_ENABLE_ERROR_REPORTING;
#endif
                m->vtbl->destroy(m);

                m->setNextFree(*f);
#ifdef V4_USE_VALGRIND
                VALGRIND_DISABLE_ERROR_REPORTING;
                VALGRIND_MEMPOOL_FREE(this, m);
#endif
                *f = m;
                SCRIBBLE(m, 0x99, size);
                ++freedCount;
            }
        }
    }
#ifdef V4_USE_VALGRIND
    VALGRIND_ENABLE_ERROR_REPORTING;
#endif

    return freedCount;
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
    if (!m_d->enableGC || m_d->gcBlocked) {
//        qDebug() << "Not running GC.";
        return;
    }

//    QTime t; t.start();

//    qDebug() << ">>>>>>>>runGC";

    mark();
//    std::cerr << "GC: marked " << marks
//              << " objects in " << t.elapsed()
//              << "ms" << std::endl;

//    t.restart();
    /*std::size_t freedCount =*/ sweep();
//    std::cerr << "GC: sweep freed " << freedCount
//              << " objects in " << t.elapsed()
//              << "ms" << std::endl;
    memset(m_d->allocCount, 0, sizeof(m_d->allocCount));
}

void MemoryManager::setEnableGC(bool enableGC)
{
    m_d->enableGC = enableGC;
}

MemoryManager::~MemoryManager()
{
    PersistentValuePrivate *persistent = m_persistentValues;
    while (persistent) {
        if (Managed *m = persistent->value.asManaged())
            persistent->value = Value::undefinedValue();
        persistent->engine = 0;
        PersistentValuePrivate *n = persistent->next;
        persistent->next = 0;
        persistent = n;
    }

    sweep();
}

void MemoryManager::protect(Managed *m)
{
    ++m_d->protectedObject[m];
}

void MemoryManager::unprotect(Managed *m)
{
    if (!--m_d->protectedObject[m])
        m_d->protectedObject.remove(m);
}

static inline void add(QVector<Managed *> &values, const Value &v)
{
    if (Object *o = v.asObject())
        values.append(o);
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

void MemoryManager::collectFromStack() const
{
    quintptr valueOnStack = 0;

    if (!m_d->heapChunks.count())
        return;

    quintptr *current = (&valueOnStack) + 1;
//    qDebug() << "collectFromStack";// << top << current << &valueOnStack;

#if V4_USE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(current, (m_d->stackTop - current)*sizeof(quintptr));
#endif

    char** heapChunkBoundaries = (char**)alloca(m_d->heapChunks.count() * 2 * sizeof(char*));
    char** heapChunkBoundariesEnd = heapChunkBoundaries + 2 * m_d->heapChunks.count();
    int i = 0;
    for (QVector<Data::Chunk>::Iterator it = m_d->heapChunks.begin(), end =
         m_d->heapChunks.end(); it != end; ++it) {
        heapChunkBoundaries[i++] = reinterpret_cast<char*>(it->memory.base()) - 1;
        heapChunkBoundaries[i++] = reinterpret_cast<char*>(it->memory.base()) + it->memory.size() - it->chunkSize;
    }
    assert(i == m_d->heapChunks.count() * 2);

    for (; current < m_d->stackTop; ++current) {
        char* genericPtr =
#if QT_POINTER_SIZE == 8
                reinterpret_cast<char *>((*current) & ~(quint64(Value::Type_Mask) << Value::Tag_Shift));
#else
                reinterpret_cast<char *>(*current);
#endif

        if (genericPtr < *heapChunkBoundaries || genericPtr > *(heapChunkBoundariesEnd - 1))
            continue;
        int index = qLowerBound(heapChunkBoundaries, heapChunkBoundariesEnd, genericPtr) - heapChunkBoundaries;
        // An odd index means the pointer is _before_ the end of a heap chunk and therefore valid.
        assert(index >= 0 && index < m_d->heapChunks.count() * 2);
        if (index & 1) {
            int size = m_d->heapChunks.at(index >> 1).chunkSize;
            Managed *m = reinterpret_cast<Managed *>(genericPtr);
//            qDebug() << "   inside" << size;

            if (((quintptr)m - (quintptr)heapChunkBoundaries[index-1] - 1   ) % size)
                // wrongly aligned value, skip it
                continue;

            if (!m->inUse)
                // Skip pointers to already freed objects, they are bogus as well
                continue;

//            qDebug() << "       marking";
            m->mark();
        }
    }
}
