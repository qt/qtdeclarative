/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
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
****************************************************************************/

#include "qmljs_engine.h"
#include "qv4object.h"
#include "qv4objectproto.h"
#include "qv4mm.h"
#include "PageAllocation.h"
#include "StdLibExtras.h"

#include <QTime>
#include <QVector>
#include <QVector>
#include <QMap>

#include <iostream>
#include <cstdlib>
#include <alloca.h>

using namespace QQmlJS::VM;
using namespace WTF;

static const std::size_t CHUNK_SIZE = 65536;

struct MemoryManager::Data
{
    bool enableGC;
    bool gcBlocked;
    bool scribble;
    bool aggressiveGC;
    ExecutionEngine *engine;

    enum { MaxItemSize = 256 };
    Managed *smallItems[MaxItemSize/16];
    struct Chunk {
        PageAllocation memory;
        int chunkSize;
    };

    QVector<Chunk> heapChunks;

    // statistics:
#ifdef DETAILED_MM_STATS
    QVector<unsigned> allocSizeCounters;
#endif // DETAILED_MM_STATS

    Data(bool enableGC)
        : enableGC(enableGC)
        , gcBlocked(false)
        , engine(0)
    {
        memset(smallItems, 0, sizeof(smallItems));
        scribble = qgetenv("MM_NO_SCRIBBLE").isEmpty();
        aggressiveGC = !qgetenv("MM_AGGRESSIVE_GC").isEmpty();
    }

    ~Data()
    {
        for (QVector<Chunk>::iterator i = heapChunks.begin(), ei = heapChunks.end(); i != ei; ++i)
            i->memory.deallocate();
    }
};

namespace QQmlJS { namespace VM {

bool operator<(const MemoryManager::Data::Chunk &a, const MemoryManager::Data::Chunk &b)
{
    return a.memory.base() < b.memory.base();
}

} } // namespace QQmlJS::VM

MemoryManager::MemoryManager()
    : m_d(new Data(true))
{
    setEnableGC(true);
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

    // fits into a small bucket
    assert(size < MemoryManager::Data::MaxItemSize);

    Managed *m = m_d->smallItems[pos];
    if (m)
        goto found;

    // try to free up space, otherwise allocate
    if (!m_d->aggressiveGC) {
        runGC();
        m = m_d->smallItems[pos];
        if (m)
            goto found;
    }

    // no free item available, allocate a new chunk
    {
        std::size_t allocSize = std::max(size, CHUNK_SIZE);
        allocSize = roundUpToMultipleOf(WTF::pageSize(), allocSize);
        Data::Chunk allocation;
        allocation.memory = PageAllocation::allocate(allocSize, OSAllocator::JSGCHeapPages);
        allocation.chunkSize = size;
        m_d->heapChunks.append(allocation);
        qSort(m_d->heapChunks);
        char *chunk = (char *)allocation.memory.base();
        char *end = chunk + allocation.memory.size() - size;
        Managed **last = &m_d->smallItems[pos];
        while (chunk <= end) {
            Managed *o = reinterpret_cast<Managed *>(chunk);
            o->inUse = 0;
            o->markBit = 0;
            *last = o;
            last = &o->nextFree;
            chunk += size;
        }
        *last = 0;
        m = m_d->smallItems[pos];
    }

  found:
    m_d->smallItems[pos] = m->nextFree;
    return m;
}

void MemoryManager::scribble(Managed *obj, int c, int size) const
{
    if (m_d->scribble)
        ::memset((void *)(obj + 1), c, size - sizeof(Managed));
}

std::size_t MemoryManager::mark(const QVector<Object *> &objects)
{
    std::size_t marks = 0;

    QVector<Object *> kids;
    kids.reserve(32);

    foreach (Object *o, objects) {
        if (!o)
            continue;

        Managed *obj = o;
        assert(obj->inUse);
        if (obj->markBit == 0) {
            obj->markBit = 1;
            ++marks;
            static_cast<Managed *>(o)->getCollectables(kids);
            marks += mark(kids);
            kids.resize(0);
        }
    }

    return marks;
}

std::size_t MemoryManager::sweep()
{
    std::size_t freedCount = 0;

    for (QVector<Data::Chunk>::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i)
        freedCount += sweep(reinterpret_cast<char*>(i->memory.base()), i->memory.size(), i->chunkSize);

    return freedCount;
}

std::size_t MemoryManager::sweep(char *chunkStart, std::size_t chunkSize, size_t size)
{
//    qDebug("chunkStart @ %p, size=%x, pos=%x (%x)", chunkStart, size, size>>4, m_d->smallItems[size >> 4]);
    std::size_t freedCount = 0;

    Managed **f = &m_d->smallItems[size >> 4];

    for (char *chunk = chunkStart, *chunkEnd = chunk + chunkSize - size; chunk <= chunkEnd; ) {
        Managed *m = reinterpret_cast<Managed *>(chunk);
//        qDebug("chunk @ %p, size = %lu, in use: %s, mark bit: %s",
//               chunk, m->size, (m->inUse ? "yes" : "no"), (m->markBit ? "true" : "false"));

        assert((intptr_t) chunk % 16 == 0);

        chunk = chunk + size;
        if (m->inUse) {
            if (m->markBit) {
                m->markBit = 0;
            } else {
//                qDebug() << "-- collecting it." << m << *f << &m->nextFree;
                m->~Managed();

                m->nextFree = *f;
                f = &m->nextFree;
                //scribble(m, 0x99, size);
                ++freedCount;
            }
        }
    }

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
    QVector<Object *> roots;
    collectRoots(roots);
//    std::cerr << "GC: found " << roots.size()
//              << " roots in " << t.elapsed()
//              << "ms" << std::endl;

//    t.restart();
    /*std::size_t marks =*/ mark(roots);
//    std::cerr << "GC: marked " << marks
//              << " objects in " << t.elapsed()
//              << "ms" << std::endl;

//    t.restart();
    /*std::size_t freedCount =*/ sweep();
//    std::cerr << "GC: sweep freed " << freedCount
//              << " objects in " << t.elapsed()
//              << "ms" << std::endl;
}

void MemoryManager::setEnableGC(bool enableGC)
{
    m_d->enableGC = enableGC;
}

MemoryManager::~MemoryManager()
{
    sweep();
}

static inline void add(QVector<Object *> &values, const Value &v)
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
    std::cerr << "=================" << std::endl;
    std::cerr << "Allocation stats:" << std::endl;
#ifdef DETAILED_MM_STATS
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

void MemoryManager::collectRoots(QVector<VM::Object *> &roots) const
{
    add(roots, m_d->engine->globalObject);
    add(roots, m_d->engine->exception);

    for (int i = 0; i < m_d->engine->argumentsAccessors.size(); ++i) {
        const PropertyDescriptor &pd = m_d->engine->argumentsAccessors.at(i);
        add(roots, Value::fromObject(pd.get));
        add(roots, Value::fromObject(pd.set));
    }

    for (ExecutionContext *ctxt = engine()->current; ctxt; ctxt = ctxt->parent) {
        add(roots, ctxt->thisObject);
        if (ctxt->function)
            roots.append(ctxt->function);
        for (unsigned arg = 0, lastArg = ctxt->formalCount(); arg < lastArg; ++arg)
            add(roots, ctxt->arguments[arg]);
        for (unsigned local = 0, lastLocal = ctxt->variableCount(); local < lastLocal; ++local)
            add(roots, ctxt->locals[local]);
        if (ctxt->activation)
            roots.append(ctxt->activation);
        for (ExecutionContext::With *it = ctxt->withObject; it; it = it->next)
            if (it->object)
                roots.append(it->object);
    }

    collectRootsOnStack(roots);
}

void MemoryManager::collectRootsOnStack(QVector<VM::Object *> &roots) const
{
    if (!m_d->heapChunks.count())
        return;

    Value valueOnStack = Value::undefinedValue();

    void* stackTop = 0;
#if USE(PTHREADS)
#if OS(DARWIN)
    stackTop = pthread_get_stackaddr_np(pthread_self());
#else
    pthread_attr_t attr;
    pthread_getattr_np(pthread_self(), &attr);
    size_t stackSize = 0;
    pthread_attr_getstack(&attr, &stackTop, &stackSize);
#endif
#endif

    Value* top = reinterpret_cast<Value*>(stackTop) - 1;
    Value* current = (&valueOnStack) + 1;

    char** heapChunkBoundaries = (char**)alloca(m_d->heapChunks.count() * 2 * sizeof(char*));
    char** heapChunkBoundariesEnd = heapChunkBoundaries + 2 * m_d->heapChunks.count();
    int i = 0;
    for (QVector<Data::Chunk>::Iterator it = m_d->heapChunks.begin(), end =
         m_d->heapChunks.end(); it != end; ++it) {
        heapChunkBoundaries[i++] = reinterpret_cast<char*>(it->memory.base());
        heapChunkBoundaries[i++] = reinterpret_cast<char*>(it->memory.base()) + it->memory.size();
    }

    for (; current < top; ++current) {
        Object* possibleObject = current->asObject();
        if (!possibleObject)
            continue;

        char* genericPtr = reinterpret_cast<char*>(possibleObject);
        if (genericPtr < *heapChunkBoundaries || genericPtr >= *(heapChunkBoundariesEnd - 1))
            continue;
        int index = qLowerBound(heapChunkBoundaries, heapChunkBoundariesEnd, genericPtr) - heapChunkBoundaries;
        // An odd index means the pointer is _before_ the end of a heap chunk and therefore valid.
        if (index & 1) {
            int size = m_d->heapChunks.at(index >> 1).chunkSize;
            Managed *m = possibleObject;

            if (((quintptr)m - (quintptr)heapChunkBoundaries[index-1]) % size)
                // wrongly aligned value, skip it
                continue;

            if (!m->inUse)
                // Skip pointers to already freed objects, they are bogus as well
                continue;

            roots.append(possibleObject);
        }
    }
}
