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

static const std::size_t CHUNK_SIZE = 1024*32;

#if OS(WINCE)
void* g_stackBase = 0;

inline bool isPageWritable(void* page)
{
    MEMORY_BASIC_INFORMATION memoryInformation;
    DWORD result = VirtualQuery(page, &memoryInformation, sizeof(memoryInformation));

    // return false on error, including ptr outside memory
    if (result != sizeof(memoryInformation))
        return false;

    DWORD protect = memoryInformation.Protect & ~(PAGE_GUARD | PAGE_NOCACHE);
    return protect == PAGE_READWRITE
        || protect == PAGE_WRITECOPY
        || protect == PAGE_EXECUTE_READWRITE
        || protect == PAGE_EXECUTE_WRITECOPY;
}

static void* getStackBase(void* previousFrame)
{
    // find the address of this stack frame by taking the address of a local variable
    bool isGrowingDownward;
    void* thisFrame = (void*)(&isGrowingDownward);

    isGrowingDownward = previousFrame < &thisFrame;
    static DWORD pageSize = 0;
    if (!pageSize) {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        pageSize = systemInfo.dwPageSize;
    }

    // scan all of memory starting from this frame, and return the last writeable page found
    register char* currentPage = (char*)((DWORD)thisFrame & ~(pageSize - 1));
    if (isGrowingDownward) {
        while (currentPage > 0) {
            // check for underflow
            if (currentPage >= (char*)pageSize)
                currentPage -= pageSize;
            else
                currentPage = 0;
            if (!isPageWritable(currentPage))
                return currentPage + pageSize;
        }
        return 0;
    } else {
        while (true) {
            // guaranteed to complete because isPageWritable returns false at end of memory
            currentPage += pageSize;
            if (!isPageWritable(currentPage))
                return currentPage;
        }
    }
}
#endif

struct MemoryManager::Data
{
    bool enableGC;
    bool gcBlocked;
    bool scribble;
    bool aggressiveGC;
    bool exactGC;
    ExecutionEngine *engine;
    quintptr *stackTop;

    enum { MaxItemSize = 512 };
    Managed *smallItems[MaxItemSize/16];
    uint nChunks[MaxItemSize/16];
    uint availableItems[MaxItemSize/16];
    uint allocCount[MaxItemSize/16];
    int totalItems;
    int totalAlloc;
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


    // statistics:
#ifdef DETAILED_MM_STATS
    QVector<unsigned> allocSizeCounters;
#endif // DETAILED_MM_STATS

    Data(bool enableGC)
        : enableGC(enableGC)
        , gcBlocked(false)
        , engine(0)
        , stackTop(0)
        , totalItems(0)
        , totalAlloc(0)
        , largeItems(0)
    {
        memset(smallItems, 0, sizeof(smallItems));
        memset(nChunks, 0, sizeof(nChunks));
        memset(availableItems, 0, sizeof(availableItems));
        memset(allocCount, 0, sizeof(allocCount));
        scribble = !qgetenv("QV4_MM_SCRIBBLE").isEmpty();
        aggressiveGC = !qgetenv("QV4_MM_AGGRESSIVE_GC").isEmpty();
        exactGC = qgetenv("QV4_MM_CONSERVATIVE_GC").isEmpty();
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


namespace QV4 {

bool operator<(const MemoryManager::Data::Chunk &a, const MemoryManager::Data::Chunk &b)
{
    return a.memory.base() < b.memory.base();
}

} // namespace QV4

MemoryManager::MemoryManager()
    : m_d(new Data(true))
    , m_persistentValues(0)
    , m_weakValues(0)
{
    setEnableGC(true);
#ifdef V4_USE_VALGRIND
    VALGRIND_CREATE_MEMPOOL(this, 0, true);
#endif

#if OS(QNX)
    // TLS is at the top of each thread's stack,
    // so the stack base for thread is the result of __tls()
    m_d->stackTop = reinterpret_cast<quintptr *>(
          (((quintptr)__tls() + __PAGESIZE - 1) & ~(__PAGESIZE - 1)));
#elif USE(PTHREADS)
#  if OS(DARWIN)
    void *st = pthread_get_stackaddr_np(pthread_self());
    m_d->stackTop = static_cast<quintptr *>(st);
#  else
    void* stackBottom = 0;
    pthread_attr_t attr;
#if HAVE(PTHREAD_NP_H) && OS(FREEBSD)
    if (pthread_attr_get_np(pthread_self(), &attr) == 0) {
#else
    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
#endif
        size_t stackSize = 0;
        pthread_attr_getstack(&attr, &stackBottom, &stackSize);
        pthread_attr_destroy(&attr);

        m_d->stackTop = static_cast<quintptr *>(stackBottom) + stackSize/sizeof(quintptr);
    } else {
        // can't scan the native stack so have to rely on exact gc
        m_d->stackTop = 0;
        m_d->exactGC = true;
    }
#  endif
#elif OS(WINCE)
    if (false && g_stackBase) {
        // This code path is disabled as we have no way of initializing it yet
        m_d->stackTop = static_cast<quintptr *>(g_stackBase);
    } else {
        int dummy;
        m_d->stackTop = static_cast<quintptr *>(getStackBase(&dummy));
    }
#elif OS(WINDOWS)
    PNT_TIB tib = (PNT_TIB)NtCurrentTeb();
    m_d->stackTop = static_cast<quintptr*>(tib->StackBase);
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

    Q_ASSERT(size >= 16);
    Q_ASSERT(size % 16 == 0);

    size_t pos = size >> 4;

    // doesn't fit into a small bucket
    if (size >= MemoryManager::Data::MaxItemSize) {
        // we use malloc for this
        MemoryManager::Data::LargeItem *item = static_cast<MemoryManager::Data::LargeItem *>(malloc(size + sizeof(MemoryManager::Data::LargeItem)));
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
        // allocate larger chunks at a time to avoid excessive GC, but cap at 64M chunks
        uint shift = ++m_d->nChunks[pos];
        if (shift > 10)
            shift = 10;
        std::size_t allocSize = CHUNK_SIZE*(size_t(1) << shift);
        allocSize = roundUpToMultipleOf(WTF::pageSize(), allocSize);
        Data::Chunk allocation;
        allocation.memory = PageAllocation::allocate(allocSize, OSAllocator::JSGCHeapPages);
        allocation.chunkSize = int(size);
        m_d->heapChunks.append(allocation);
        std::sort(m_d->heapChunks.begin(), m_d->heapChunks.end());
        char *chunk = (char *)allocation.memory.base();
        char *end = chunk + allocation.memory.size() - size;
#ifndef QT_NO_DEBUG
        memset(chunk, 0, allocation.memory.size());
#endif
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
    SafeValue *markBase = m_d->engine->jsStackTop;

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

    if (!m_d->exactGC) {
        // push all caller saved registers to the stack, so we can find the objects living in these registers
#if COMPILER(MSVC) && !OS(WINRT) // WinRT must use exact GC
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

    GCDeletable *deletable = 0;
    GCDeletable **firstDeletable = &deletable;

    for (QVector<Data::Chunk>::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i)
        sweep(reinterpret_cast<char*>(i->memory.base()), i->memory.size(), i->chunkSize, &deletable);

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

        *last = i->next;
        free(i);
        i = *last;
    }

    deletable = *firstDeletable;
    while (deletable) {
        GCDeletable *next = deletable->next;
        deletable->lastCall = lastSweep;
        delete deletable;
        deletable = next;
    }
}

void MemoryManager::sweep(char *chunkStart, std::size_t chunkSize, size_t size, GCDeletable **deletable)
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
                if (m->internalClass->vtable->collectDeletables)
                    m->internalClass->vtable->collectDeletables(m, deletable);
                m->internalClass->vtable->destroy(m);

                m->setNextFree(*f);
#ifdef V4_USE_VALGRIND
                VALGRIND_DISABLE_ERROR_REPORTING;
                VALGRIND_MEMPOOL_FREE(this, m);
#endif
                *f = m;
                SCRIBBLE(m, 0x99, size);
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
    m_d->totalAlloc = 0;
}

void MemoryManager::setEnableGC(bool enableGC)
{
    m_d->enableGC = enableGC;
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
    Q_ASSERT(i == m_d->heapChunks.count() * 2);

    for (; current < m_d->stackTop; ++current) {
        char* genericPtr = reinterpret_cast<char *>(*current);

        if (genericPtr < *heapChunkBoundaries || genericPtr > *(heapChunkBoundariesEnd - 1))
            continue;
        int index = std::lower_bound(heapChunkBoundaries, heapChunkBoundariesEnd, genericPtr) - heapChunkBoundaries;
        // An odd index means the pointer is _before_ the end of a heap chunk and therefore valid.
        Q_ASSERT(index >= 0 && index < m_d->heapChunks.count() * 2);
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
            m->mark(m_d->engine);
        }
    }
}

void MemoryManager::collectFromJSStack() const
{
    SafeValue *v = engine()->jsStackBase;
    SafeValue *top = engine()->jsStackTop;
    while (v < top) {
        Managed *m = v->asManaged();
        if (m && m->inUse)
            // Skip pointers to already freed objects, they are bogus as well
            m->mark(m_d->engine);
        ++v;
    }
}
QT_END_NAMESPACE
