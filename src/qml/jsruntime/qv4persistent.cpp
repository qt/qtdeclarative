// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4persistent_p.h"
#include <private/qv4mm_p.h>
#include "qv4object_p.h"
#include "qv4qobjectwrapper_p.h"
#include "PageAllocation.h"

using namespace QV4;

namespace {

struct Page;
struct Header {
    WTF::PageAllocation alloc;
    ExecutionEngine *engine;
    Page **prev;
    Page *next;
    int refCount;
    int freeList;
};

static const int kEntriesPerPage = int((WTF::pageSize() - sizeof(Header)) / sizeof(Value));

struct Page {
    Header header;
    Value values[1]; // Really kEntriesPerPage, but keep the compiler happy
};

Page *getPage(const Value *val) {
   return reinterpret_cast<Page *>(reinterpret_cast<quintptr>(val) & ~((quintptr)(WTF::pageSize() - 1)));
}

QML_NEARLY_ALWAYS_INLINE void insertInFront(PersistentValueStorage *storage, Page *p)
{
    p->header.next = reinterpret_cast<Page *>(storage->firstPage);
    p->header.prev = reinterpret_cast<Page **>(&storage->firstPage);
    if (p->header.next)
        p->header.next->header.prev = &p->header.next;
    storage->firstPage = p;
}

QML_NEARLY_ALWAYS_INLINE void unlink(Page *p)
{
    if (p->header.prev)
        *p->header.prev = p->header.next;
    if (p->header.next)
        p->header.next->header.prev = p->header.prev;
}

Page *allocatePage(PersistentValueStorage *storage)
{
    PageAllocation page = WTF::PageAllocation::allocate(WTF::pageSize());
    Page *p = reinterpret_cast<Page *>(page.base());

    Q_ASSERT(!((quintptr)p & (WTF::pageSize() - 1)));

    p->header.engine = storage->engine;
    p->header.alloc = page;
    p->header.refCount = 0;
    p->header.freeList = 0;
    insertInFront(storage, p);
    for (int i = 0; i < kEntriesPerPage - 1; ++i) {
        p->values[i] = Encode(i + 1);
    }
    p->values[kEntriesPerPage - 1] = Encode(-1);

    return p;
}


}


PersistentValueStorage::Iterator::Iterator(void *p, int idx)
    : p(p), index(idx)
{
    Page *page = static_cast<Page *>(p);
    if (page)
        ++page->header.refCount;
}

PersistentValueStorage::Iterator::Iterator(const PersistentValueStorage::Iterator &o)
    : p(o.p), index(o.index)
{
    Page *page = static_cast<Page *>(p);
    if (page)
        ++page->header.refCount;
}

PersistentValueStorage::Iterator &PersistentValueStorage::Iterator::operator=(const PersistentValueStorage::Iterator &o)
{
    Page *page = static_cast<Page *>(p);
    if (page && !--page->header.refCount)
        freePage(p);
    p = o.p;
    index = o.index;
    page = static_cast<Page *>(p);
    if (page)
        ++page->header.refCount;

    return *this;
}

PersistentValueStorage::Iterator::~Iterator()
{
    Page *page = static_cast<Page *>(p);
    if (page && !--page->header.refCount)
        freePage(page);
}

PersistentValueStorage::Iterator &PersistentValueStorage::Iterator::operator++() {
    while (p) {
        while (index < kEntriesPerPage - 1) {
            ++index;
            if (!static_cast<Page *>(p)->values[index].isEmpty())
                return *this;
        }
        index = -1;
        Page *next = static_cast<Page *>(p)->header.next;
        if (!--static_cast<Page *>(p)->header.refCount)
            freePage(p);
        p = next;
        if (next)
            ++next->header.refCount;
    }
    index = 0;
    return *this;
}

Value &PersistentValueStorage::Iterator::operator *()
{
    return static_cast<Page *>(p)->values[index];
}

PersistentValueStorage::PersistentValueStorage(ExecutionEngine *engine)
    : engine(engine),
      firstPage(nullptr)
{
}

PersistentValueStorage::~PersistentValueStorage()
{
    clearFreePageHint();
    Page *p = static_cast<Page *>(firstPage);
    while (p) {
        for (int i = 0; i < kEntriesPerPage; ++i) {
            if (!p->values[i].isEmpty())
                p->values[i] = Encode::undefined();
        }
        Page *n = p->header.next;
        p->header.engine = nullptr;
        p->header.prev = nullptr;
        p->header.next = nullptr;
        Q_ASSERT(p->header.refCount);
        p = n;
    }
}

Value *PersistentValueStorage::allocate()
{
    Page *p = static_cast<Page *>(freePageHint);
    if (p && p->header.freeList == -1)
        p = static_cast<Page *>(firstPage);
    while (p) {
        if (p->header.freeList != -1)
            break;
        p = p->header.next;
    }
    if (!p)
        p = allocatePage(this);

    Value *v = p->values + p->header.freeList;
    p->header.freeList = v->int_32();

    if (p->header.freeList != -1 && p != freePageHint) {
        if (auto oldHint = static_cast<Page *>(freePageHint)) {
            oldHint->header.refCount--;
            // no need to free - if the old page were unused,
            // we would have used it to serve the allocation
            Q_ASSERT(oldHint->header.refCount);
        }
        freePageHint = p;
        p->header.refCount++;
    }

    ++p->header.refCount;

    v->setRawValue(Encode::undefined());

    return v;
}

void PersistentValueStorage::freeUnchecked(Value *v)
{
    Q_ASSERT(v);
    Page *p = getPage(v);

    *v = Encode(p->header.freeList);
    p->header.freeList = v - p->values;
    if (!--p->header.refCount)
        freePage(p);
}

void PersistentValueStorage::mark(MarkStack *markStack)
{
    Page *p = static_cast<Page *>(firstPage);
    while (p) {
        for (int i = 0; i < kEntriesPerPage; ++i) {
            if (Managed *m = p->values[i].as<Managed>())
                m->mark(markStack);
        }

        p = p->header.next;
    }
}

void PersistentValueStorage::clearFreePageHint()
{
    if (!freePageHint)
        return;
    auto page = static_cast<Page *>(freePageHint);
    if (!--page->header.refCount)
        freePage(page);
    freePageHint = nullptr;

}

ExecutionEngine *PersistentValueStorage::getEngine(const Value *v)
{
    return getPage(v)->header.engine;
}

void PersistentValueStorage::freePage(void *page)
{
    Page *p = static_cast<Page *>(page);
    unlink(p);
    p->header.alloc.deallocate();
}


PersistentValue::PersistentValue(const PersistentValue &other)
    : val(nullptr)
{
    if (other.val)
        set(other.engine(), *other.val);
}

PersistentValue::PersistentValue(ExecutionEngine *engine, const Value &value)
{
    set(engine, value);
}

PersistentValue::PersistentValue(ExecutionEngine *engine, ReturnedValue value)
{
    set(engine, value);
}

PersistentValue::PersistentValue(ExecutionEngine *engine, Object *object)
    : val(nullptr)
{
    if (!object)
        return;
    set(engine, *object);
}

PersistentValue &PersistentValue::operator=(const PersistentValue &other)
{
    if (!val) {
        if (!other.val)
            return *this;
        val = other.engine()->memoryManager->m_persistentValues->allocate();
    }
    if (!other.val) {
        *val = Encode::undefined();
        return *this;
    }

    Q_ASSERT(engine() == other.engine());

    *val = *other.val;
    return *this;
}

PersistentValue &PersistentValue::operator=(const WeakValue &other)
{
    if (!val && !other.valueRef())
        return *this;
    if (!other.valueRef()) {
        *val = Encode::undefined();
        return *this;
    }

    Q_ASSERT(!engine() || engine() == other.engine());

    set(other.engine(), *other.valueRef());
    return *this;
}

PersistentValue &PersistentValue::operator=(Object *object)
{
    if (!object) {
        PersistentValueStorage::free(val);
        return *this;
    }
    set(object->engine(), *object);
    return *this;
}

void PersistentValue::set(ExecutionEngine *engine, const Value &value)
{
    if (!val)
        val = engine->memoryManager->m_persistentValues->allocate();
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack){
        if (QV4::WriteBarrier::isInsertionBarrier && value.isManaged())
            value.heapObject()->mark(stack);
    });
    *val = value;
}

void PersistentValue::set(ExecutionEngine *engine, ReturnedValue value)
{
    if (!val)
        val = engine->memoryManager->m_persistentValues->allocate();
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack){
        if constexpr (!QV4::WriteBarrier::isInsertionBarrier)
            return;
        auto val = Value::fromReturnedValue(value);
        if (val.isManaged())
            val.heapObject()->mark(stack);
    });
    *val = value;
}

void PersistentValue::set(ExecutionEngine *engine, Heap::Base *obj)
{
    if (!val)
        val = engine->memoryManager->m_persistentValues->allocate();
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *stack){
        if constexpr (QV4::WriteBarrier::isInsertionBarrier)
            obj->mark(stack);
    });

    *val = obj;
}

WeakValue::WeakValue(const WeakValue &other)
    : val(nullptr)
{
    if (other.val) {
        allocVal(other.engine());
        *val = *other.val;
    }
}

WeakValue::WeakValue(ExecutionEngine *engine, const Value &value)
{
    allocVal(engine);
    *val = value;
}

WeakValue &WeakValue::operator=(const WeakValue &other)
{
    if (!val) {
        if (!other.val)
            return *this;
        allocVal(other.engine());
    }
    if (!other.val) {
        *val = Encode::undefined();
        return *this;
    }

    Q_ASSERT(engine() == other.engine());

    *val = *other.val;
    return *this;
}

WeakValue::~WeakValue()
{
    free();
}

/*
   WeakValue::set shold normally not mark objects, after all a weak value
   is not supposed to keep an object alive.
   However, if we are past GCState::HandleQObjectWrappers, nothing will
   reset weak values referencing unmarked values, but those values will
   still be swept.
   That lead to stale pointers, and potentially to crashes. To avoid this,
   we mark the objects here (they might still get collected in the next gc
   run).
   This is especially important due to the way we handle QObjectWrappers.
 */
void WeakValue::set(ExecutionEngine *engine, const Value &value)
{
    if (!val)
        allocVal(engine);
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *ms) {
        if (engine->memoryManager->gcStateMachine->state <= GCState::HandleQObjectWrappers)
            return;
        if (auto *h = value.heapObject())
            h->mark(ms);
    });
    *val = value;
}

void WeakValue::set(ExecutionEngine *engine, ReturnedValue value)
{
    if (!val)
        allocVal(engine);
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *ms) {
        if (engine->memoryManager->gcStateMachine->state <= GCState::HandleQObjectWrappers)
            return;
        if (auto *h = QV4::Value::fromReturnedValue(value).heapObject())
            h->mark(ms);
    });

    *val = value;
}

void WeakValue::set(ExecutionEngine *engine, Heap::Base *obj)
{
    if (!val)
        allocVal(engine);
    QV4::WriteBarrier::markCustom(engine, [&](QV4::MarkStack *ms) {
        if (engine->memoryManager->gcStateMachine->state <= GCState::HandleQObjectWrappers)
            return;
        obj->mark(ms);
    });
    *val = obj;
}

void WeakValue::allocVal(ExecutionEngine *engine)
{
    val = engine->memoryManager->m_weakValues->allocate();
}

void WeakValue::markOnce(MarkStack *markStack)
{
    if (!val)
        return;
    val->mark(markStack);
}

void WeakValue::free()
{
    if (!val)
        return;

    ExecutionEngine *e = engine();
    if (e && val->as<QObjectWrapper>()) {
        // Some QV4::QObjectWrapper Value will be freed in WeakValue::~WeakValue() before MemoryManager::sweep() is being called,
        // in this case we will never have a chance to call detroyObject() on those QV4::QObjectWrapper objects.
        // Here we don't free these Value immediately, instead we keep track of them to free them later in MemoryManager::sweep()
        e->memoryManager->m_pendingFreedObjectWrapperValue.push_back(val);
    } else {
        PersistentValueStorage::free(val);
    }

    val = nullptr;
}

