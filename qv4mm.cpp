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
#include "qmljs_objects.h"
#include "qv4ecmaobjects_p.h"
#include "qv4mm.h"
#include "StackBounds.h"

#include <QTime>
#include <QVector>
#include <QLinkedList>
#include <QMap>

#include <iostream>
#include <cstdlib>

using namespace QQmlJS::VM;

static const std::size_t CHUNK_SIZE = 65536;

struct MemoryManager::Data
{
    bool enableGC;
    bool gcBlocked;
    bool scribble;
    bool aggressiveGC;
    ExecutionEngine *engine;
    StringPool *stringPool;

    MMObject *fallbackObject;
    MMObject *smallItems[16]; // 16 - 256 bytes
    QMap<size_t, MMObject *> largeItems;
    QLinkedList<QPair<char *, std::size_t> > heapChunks;

    // statistics:
#ifdef DETAILED_MM_STATS
    QVector<unsigned> allocSizeCounters;
#endif // DETAILED_MM_STATS

    Data(bool enableGC)
        : enableGC(enableGC)
        , gcBlocked(false)
        , engine(0)
        , stringPool(0)
        , fallbackObject(0)
    {
        memset(smallItems, 0, sizeof(smallItems));
        scribble = qgetenv("MM_NO_SCRIBBLE").isEmpty();
        aggressiveGC = !qgetenv("MM_AGGRESSIVE_GC").isEmpty();
    }

    ~Data()
    {
        for (QLinkedList<QPair<char *, std::size_t> >::iterator i = heapChunks.begin(), ei = heapChunks.end(); i != ei; ++i)
            free(i->first);
    }
};

MemoryManager::MemoryManager()
    : m_d(new Data(true))
{
}

MemoryManager::MMObject *MemoryManager::alloc(std::size_t size)
{
    if (m_d->aggressiveGC)
        runGC();
#ifdef DETAILED_MM_STATS
    willAllocate(size);
#endif // DETAILED_MM_STATS

    size += align(sizeof(MMInfo));

    assert(size >= 16);
    assert(size % 16 == 0);

    size_t pos = size >> 4;

    // fits into a small bucket
    if (pos < sizeof(m_d->smallItems)/sizeof(MMObject *)) {
        MMObject *m = m_d->smallItems[pos];
        if (m) {
            m_d->smallItems[pos] = m->info.next;
            m->info.inUse = 1;
            m->info.markBit = 0;
            return m;
        }
    }


    // ### use new heap space if available
    if (m_d->fallbackObject && m_d->fallbackObject->info.size >= size) {
        MMObject *m = m_d->fallbackObject;
        m_d->fallbackObject = splitItem(m, size);
        m->info.inUse = 1;
        m->info.markBit = 0;
        return m;
    }

    // use or split up a large bucket
    QMap<size_t, MMObject *>::iterator it = m_d->largeItems.lowerBound(pos);
    if (it == m_d->largeItems.end()) {
        // try to free up space, otherwise allocate
        if (!m_d->aggressiveGC || runGC() < size) {
            std::size_t allocSize = std::max(size, CHUNK_SIZE);
            char *ptr = 0;
            posix_memalign(reinterpret_cast<void**>(&ptr), 16, allocSize);
            m_d->heapChunks.append(qMakePair(ptr, allocSize));
            m_d->fallbackObject = reinterpret_cast<MMObject *>(ptr);
            m_d->fallbackObject->info.inUse = 0;
            m_d->fallbackObject->info.next = 0;
            m_d->fallbackObject->info.markBit = 0;
            m_d->fallbackObject->info.size = allocSize;
        }
        return alloc(size - sizeof(MMInfo));
    }

    MMObject *m = it.value();
    assert(m);

    if (it.key() == pos) {
        // a match, return it
        if (!m->info.next)
            m_d->largeItems.erase(it);
        else
            *it = m->info.next;
        m->info.inUse = 1;
        m->info.markBit = 0;
        return m;
    }

    // split up
    if (!m->info.next)
        m_d->largeItems.erase(it);
    else
        *it = m->info.next;
    MMObject *tail = splitItem(m, size);
    MMObject *&f = m_d->largeItems[tail->info.size];
    tail->info.next = f;
    f = tail;
    m->info.inUse = 1;
    m->info.markBit = 0;
    return m;
}

void MemoryManager::dealloc(MMObject *ptr)
{
    if (!ptr)
        return;

    assert(ptr->info.size >= 16);
    assert(ptr->info.size % 16 == 0);

//    qDebug("dealloc %p (%lu)", ptr, ptr->info.size);

    std::size_t pos = ptr->info.size >> 4;
    MMObject **f;

    // fits into a small bucket
    if (pos < sizeof(m_d->smallItems)/sizeof(MMObject *)) {
        f = &m_d->smallItems[pos];
    } else {
        f = &m_d->largeItems[pos];
    }

    ptr->info.next = *f;
    ptr->info.inUse = 0;
    ptr->info.markBit = 0;
    ptr->info.needsManagedDestructorCall = 0;
    *f = ptr;
}

MemoryManager::MMObject *MemoryManager::splitItem(MemoryManager::MMObject *m, int newSize)
{
    if (newSize - m->info.size <= sizeof(MMObject))
        return 0;
    MMObject *tail = reinterpret_cast<MMObject *>(reinterpret_cast<char *>(m) + newSize);
    tail->info.inUse = 0;
    tail->info.markBit = 0;
    tail->info.size = m->info.size - newSize;
    m->info.size = newSize;
    return tail;
}

void MemoryManager::scribble(MemoryManager::MMObject *obj, int c) const
{
    if (m_d->scribble)
        ::memset(&obj->data, c, obj->info.size - sizeof(MMInfo));
}

std::size_t MemoryManager::mark(const QVector<Object *> &objects)
{
    std::size_t marks = 0;

    QVector<Object *> kids;
    kids.reserve(32);

    foreach (Object *o, objects) {
        if (!o)
            continue;

        MMObject *obj = toObject(o);
        assert(obj->info.inUse);
        if (obj->info.markBit == 0) {
            obj->info.markBit = 1;
            ++marks;
            static_cast<Managed *>(o)->getCollectables(kids);
            marks += mark(kids);
            kids.resize(0);
        }
    }

    return marks;
}

std::size_t MemoryManager::sweep(std::size_t &largestFreedBlock)
{
    std::size_t freedCount = 0;

    for (QLinkedList<QPair<char *, std::size_t> >::iterator i = m_d->heapChunks.begin(), ei = m_d->heapChunks.end(); i != ei; ++i)
        freedCount += sweep(i->first, i->second, largestFreedBlock);

    return freedCount;
}

std::size_t MemoryManager::sweep(char *chunkStart, std::size_t chunkSize, std::size_t &largestFreedBlock)
{
//    qDebug("chunkStart @ %p", chunkStart);
    std::size_t freedCount = 0;

    for (char *chunk = chunkStart, *chunkEnd = chunk + chunkSize; chunk < chunkEnd; ) {
        MMObject *m = reinterpret_cast<MMObject *>(chunk);
//        qDebug("chunk @ %p, size = %lu, in use: %s, mark bit: %s",
//               chunk, m->info.size, (m->info.inUse ? "yes" : "no"), (m->info.markBit ? "true" : "false"));

        assert((intptr_t) chunk % 16 == 0);
        assert(m->info.size >= 16);
        assert(m->info.size % 16 == 0);

        chunk = chunk + m->info.size;
        if (m->info.inUse) {
            if (m->info.markBit) {
                m->info.markBit = 0;
            } else {
//                qDebug("-- collecting it.");
                if (m->info.needsManagedDestructorCall)
                    reinterpret_cast<VM::Managed *>(&m->data)->~Managed();
                dealloc(m);
                largestFreedBlock = std::max(largestFreedBlock, m->info.size);
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

std::size_t MemoryManager::runGC()
{
    if (!m_d->enableGC || m_d->gcBlocked) {
//        qDebug() << "Not running GC.";
        return 0;
    }

//    QTime t; t.start();

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
    std::size_t freedCount = 0, largestFreedBlock = 0;
    freedCount = sweep(largestFreedBlock);
//    std::cerr << "GC: sweep freed " << freedCount
//              << " objects in " << t.elapsed()
//              << "ms" << std::endl;

    return largestFreedBlock;
}

void MemoryManager::setEnableGC(bool enableGC)
{
    m_d->enableGC = enableGC;
}

MemoryManager::~MemoryManager()
{
    std::size_t dummy = 0;
    sweep(dummy);
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

void MemoryManager::setStringPool(StringPool *stringPool)
{
    m_d->stringPool = stringPool;
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

MemoryManagerWithoutGC::~MemoryManagerWithoutGC()
{}

void MemoryManagerWithoutGC::collectRootsOnStack(QVector<VM::Object *> &roots) const
{
    Q_UNUSED(roots);
}

MemoryManagerWithNativeStack::~MemoryManagerWithNativeStack()
{
}

void MemoryManagerWithNativeStack::collectRootsOnStack(QVector<VM::Object *> &roots) const
{
    StackBounds bounds = StackBounds::currentThreadStackBounds();
    Value* top = reinterpret_cast<Value*>(bounds.origin());
    Value* current = reinterpret_cast<Value*>(bounds.current());
    qDebug("Collecting on stack. top %p current %p\n", top, current);
    for (; current < top; ++current) {
        if (current->asObject())
            qDebug("found object %p on stack", (void*)current);
        add(roots, *current);
    }
}
