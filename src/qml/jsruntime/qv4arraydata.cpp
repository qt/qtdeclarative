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
#include "qv4arraydata_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4mm_p.h"
#include "qv4runtime_p.h"
#include "qv4argumentsobject_p.h"

using namespace QV4;

const QV4::ManagedVTable QV4::ArrayData::static_vtbl = {
    0,
    QV4::ArrayData::IsExecutionContext,
    QV4::ArrayData::IsString,
    QV4::ArrayData::IsObject,
    QV4::ArrayData::IsFunctionObject,
    QV4::ArrayData::IsErrorObject,
    QV4::ArrayData::IsArrayData,
    0,
    QV4::ArrayData::MyType,
    "ArrayData",
    Q_VTABLE_FUNCTION(QV4::ArrayData, destroy),
    0,
    isEqualTo
};

const ArrayVTable SimpleArrayData::static_vtbl =
{
    DEFINE_MANAGED_VTABLE_INT(SimpleArrayData, 0),
    Heap::ArrayData::Simple,
    SimpleArrayData::reallocate,
    SimpleArrayData::get,
    SimpleArrayData::put,
    SimpleArrayData::putArray,
    SimpleArrayData::del,
    SimpleArrayData::setAttribute,
    SimpleArrayData::push_front,
    SimpleArrayData::pop_front,
    SimpleArrayData::truncate,
    SimpleArrayData::length
};

const ArrayVTable SparseArrayData::static_vtbl =
{
    DEFINE_MANAGED_VTABLE_INT(SparseArrayData, 0),
    Heap::ArrayData::Sparse,
    SparseArrayData::reallocate,
    SparseArrayData::get,
    SparseArrayData::put,
    SparseArrayData::putArray,
    SparseArrayData::del,
    SparseArrayData::setAttribute,
    SparseArrayData::push_front,
    SparseArrayData::pop_front,
    SparseArrayData::truncate,
    SparseArrayData::length
};

Q_STATIC_ASSERT(sizeof(Heap::ArrayData) == sizeof(Heap::SimpleArrayData));
Q_STATIC_ASSERT(sizeof(Heap::ArrayData) == sizeof(Heap::SparseArrayData));

void ArrayData::realloc(Object *o, Type newType, uint requested, bool enforceAttributes)
{
    Scope scope(o->engine());
    Scoped<ArrayData> d(scope, o->arrayData());

    uint alloc = 8;
    uint toCopy = 0;
    uint offset = 0;

    if (d) {
        bool hasAttrs = d->attrs();
        enforceAttributes |= hasAttrs;

        if (requested <= d->alloc() && newType == d->type() && hasAttrs == enforceAttributes)
            return;
        if (alloc < d->alloc())
            alloc = d->alloc();

        if (d->type() < Heap::ArrayData::Sparse) {
            offset = d->d()->offset;
            toCopy = d->d()->len;
        } else {
            toCopy = d->alloc();
        }
        if (d->type() > newType)
            newType = d->type();
    }
    if (enforceAttributes && newType == Heap::ArrayData::Simple)
        newType = Heap::ArrayData::Complex;

    while (alloc < requested)
        alloc *= 2;
    size_t size = sizeof(Heap::ArrayData) + (alloc - 1)*sizeof(Value);
    if (enforceAttributes)
        size += alloc*sizeof(PropertyAttributes);

    Scoped<ArrayData> newData(scope);
    if (newType < Heap::ArrayData::Sparse) {
        Heap::SimpleArrayData *n = scope.engine->memoryManager->allocManaged<SimpleArrayData>(size);
        new (n) Heap::SimpleArrayData;
        n->offset = 0;
        n->len = d ? d->d()->len : 0;
        newData = n;
    } else {
        Heap::SparseArrayData *n = scope.engine->memoryManager->allocManaged<SparseArrayData>(size);
        new (n) Heap::SparseArrayData;
        newData = n;
    }
    newData->setAlloc(alloc);
    newData->setType(newType);
    newData->setAttrs(enforceAttributes ? reinterpret_cast<PropertyAttributes *>(newData->d()->arrayData + alloc) : 0);
    o->setArrayData(newData);

    if (d) {
        if (enforceAttributes) {
            if (d->attrs())
                memcpy(newData->attrs(), d->attrs(), sizeof(PropertyAttributes)*toCopy);
            else
                for (uint i = 0; i < toCopy; ++i)
                    newData->attrs()[i] = Attr_Data;
        }

        if (toCopy > d->d()->alloc - offset) {
            uint copyFromStart = toCopy - (d->d()->alloc - offset);
            memcpy(newData->d()->arrayData + toCopy - copyFromStart, d->d()->arrayData, sizeof(Value)*copyFromStart);
            toCopy -= copyFromStart;
        }
        memcpy(newData->d()->arrayData, d->d()->arrayData + offset, sizeof(Value)*toCopy);
    }

    if (newType != Heap::ArrayData::Sparse)
        return;

    Heap::SparseArrayData *sparse = static_cast<Heap::SparseArrayData *>(newData->d());

    uint *lastFree;
    if (d && d->type() == Heap::ArrayData::Sparse) {
        Heap::SparseArrayData *old = static_cast<Heap::SparseArrayData *>(d->d());
        sparse->sparse = old->sparse;
        old->sparse = 0;
        sparse->freeList = old->freeList;
        lastFree = &sparse->freeList;
    } else {
        sparse->sparse = new SparseArray;
        lastFree = &sparse->freeList;
        for (uint i = 0; i < toCopy; ++i) {
            if (!sparse->arrayData[i].isEmpty()) {
                SparseArrayNode *n = sparse->sparse->insert(i);
                n->value = i;
            } else {
                *lastFree = i;
                sparse->arrayData[i].tag = Value::Empty_Type;
                lastFree = &sparse->arrayData[i].uint_32;
            }
        }
    }

    if (toCopy < sparse->alloc) {
        for (uint i = toCopy; i < sparse->alloc; ++i) {
            *lastFree = i;
            sparse->arrayData[i].tag = Value::Empty_Type;
            lastFree = &sparse->arrayData[i].uint_32;
        }
        *lastFree = UINT_MAX;
    }
    // ### Could explicitly free the old data
}

Heap::ArrayData *SimpleArrayData::reallocate(Object *o, uint n, bool enforceAttributes)
{
    realloc(o, Heap::ArrayData::Simple, n, enforceAttributes);
    return o->arrayData();
}

void ArrayData::ensureAttributes(Object *o)
{
    if (o->arrayData() && o->arrayData()->attrs)
        return;

    ArrayData::realloc(o, Heap::ArrayData::Simple, 0, true);
}


void SimpleArrayData::markObjects(Heap::Base *d, ExecutionEngine *e)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(d);
    for (uint i = 0; i < dd->len; ++i)
        dd->arrayData[dd->mappedIndex(i)].mark(e);
}

ReturnedValue SimpleArrayData::get(const Heap::ArrayData *d, uint index)
{
    const Heap::SimpleArrayData *dd = static_cast<const Heap::SimpleArrayData *>(d);
    if (index >= dd->len)
        return Primitive::emptyValue().asReturnedValue();
    return dd->data(index).asReturnedValue();
}

bool SimpleArrayData::put(Object *o, uint index, const Value &value)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    Q_ASSERT(index >= dd->len || !dd->attrs || !dd->attrs[index].isAccessor());
    // ### honour attributes
    dd->data(index) = value;
    if (index >= dd->len) {
        if (dd->attrs)
            dd->attrs[index] = Attr_Data;
        dd->len = index + 1;
    }
    return true;
}

bool SimpleArrayData::del(Object *o, uint index)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    if (index >= dd->len)
        return true;

    if (!dd->attrs || dd->attrs[index].isConfigurable()) {
        dd->data(index) = Primitive::emptyValue();
        if (dd->attrs)
            dd->attrs[index] = Attr_Data;
        return true;
    }
    if (dd->data(index).isEmpty())
        return true;
    return false;
}

void SimpleArrayData::setAttribute(Object *o, uint index, PropertyAttributes attrs)
{
    o->arrayData()->attrs[index] = attrs;
}

void SimpleArrayData::push_front(Object *o, const Value *values, uint n)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    Q_ASSERT(!dd->attrs);
    if (dd->len + n > dd->alloc) {
        realloc(o, Heap::ArrayData::Simple, dd->len + n, false);
        Q_ASSERT(o->d()->arrayData->type == Heap::ArrayData::Simple);
        dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    }
    dd->offset = (dd->offset - n) % dd->alloc;
    dd->len += n;
    for (uint i = 0; i < n; ++i)
        dd->data(i) = values[i].asReturnedValue();
}

ReturnedValue SimpleArrayData::pop_front(Object *o)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    Q_ASSERT(!dd->attrs);
    if (!dd->len)
        return Encode::undefined();

    ReturnedValue v = dd->data(0).isEmpty() ? Encode::undefined() : dd->data(0).asReturnedValue();
    dd->offset = (dd->offset + 1) % dd->alloc;
    --dd->len;
    return v;
}

uint SimpleArrayData::truncate(Object *o, uint newLen)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    if (dd->len < newLen)
        return newLen;

    if (!dd->attrs) {
        dd->len = newLen;
        return newLen;
    }

    while (dd->len > newLen) {
        if (!dd->data(dd->len - 1).isEmpty() && !dd->attrs[dd->len - 1].isConfigurable())
            return dd->len;
        --dd->len;
    }
    return dd->len;
}

uint SimpleArrayData::length(const Heap::ArrayData *d)
{
    return d->len;
}

bool SimpleArrayData::putArray(Object *o, uint index, const Value *values, uint n)
{
    Heap::SimpleArrayData *dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    if (index + n > dd->alloc) {
        reallocate(o, index + n + 1, false);
        dd = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
    }
    for (uint i = dd->len; i < index; ++i)
        dd->data(i) = Primitive::emptyValue();
    for (uint i = 0; i < n; ++i)
        dd->data(index + i) = values[i];
    dd->len = qMax(dd->len, index + n);
    return true;
}

void SparseArrayData::free(Heap::ArrayData *d, uint idx)
{
    Q_ASSERT(d && d->type == Heap::ArrayData::Sparse);
    Value *v = d->arrayData + idx;
    if (d->attrs && d->attrs[idx].isAccessor()) {
        // double slot, free both. Order is important, so we have a double slot for allocation again afterwards.
        v[1].tag = Value::Empty_Type;
        v[1].uint_32 = d->freeList;
        v[0].tag = Value::Empty_Type;
        v[0].uint_32 = idx + 1;
    } else {
        v->tag = Value::Empty_Type;
        v->uint_32 = d->freeList;
    }
    d->freeList = idx;
    if (d->attrs)
        d->attrs[idx].clear();
}


void SparseArrayData::markObjects(Heap::Base *d, ExecutionEngine *e)
{
    Heap::SparseArrayData *dd = static_cast<Heap::SparseArrayData *>(d);
    uint l = dd->alloc;
    for (uint i = 0; i < l; ++i)
        dd->arrayData[i].mark(e);
}

Heap::ArrayData *SparseArrayData::reallocate(Object *o, uint n, bool enforceAttributes)
{
    realloc(o, Heap::ArrayData::Sparse, n, enforceAttributes);
    return o->arrayData();
}

// double slots are required for accessor properties
uint SparseArrayData::allocate(Object *o, bool doubleSlot)
{
    Q_ASSERT(o->d()->arrayData->type == Heap::ArrayData::Sparse);
    Heap::SparseArrayData *dd = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    if (doubleSlot) {
        uint *last = &dd->freeList;
        while (1) {
            if (*last == UINT_MAX) {
                reallocate(o, dd->alloc + 2, true);
                dd = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
                last = &dd->freeList;
                Q_ASSERT(*last != UINT_MAX);
            }

            Q_ASSERT(dd->arrayData[*last].uint_32 != *last);
            if (dd->arrayData[*last].uint_32 == (*last + 1)) {
                // found two slots in a row
                uint idx = *last;
                *last = dd->arrayData[*last + 1].uint_32;
                dd->attrs[idx] = Attr_Accessor;
                return idx;
            }
            last = &dd->arrayData[*last].uint_32;
        }
    } else {
        if (dd->freeList == UINT_MAX) {
            reallocate(o, dd->alloc + 1, false);
            dd = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
        }
        uint idx = dd->freeList;
        Q_ASSERT(idx != UINT_MAX);
        dd->freeList = dd->arrayData[idx].uint_32;
        if (dd->attrs)
            dd->attrs[idx] = Attr_Data;
        return idx;
    }
}

ReturnedValue SparseArrayData::get(const Heap::ArrayData *d, uint index)
{
    const Heap::SparseArrayData *s = static_cast<const Heap::SparseArrayData *>(d);
    index = s->mappedIndex(index);
    if (index == UINT_MAX)
        return Primitive::emptyValue().asReturnedValue();
    return s->arrayData[index].asReturnedValue();
}

bool SparseArrayData::put(Object *o, uint index, const Value &value)
{
    if (value.isEmpty())
        return true;

    Heap::SparseArrayData *s = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    SparseArrayNode *n = s->sparse->insert(index);
    Q_ASSERT(n->value == UINT_MAX || !s->attrs || !s->attrs[n->value].isAccessor());
    if (n->value == UINT_MAX)
        n->value = allocate(o);
    s = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    s->arrayData[n->value] = value;
    if (s->attrs)
        s->attrs[n->value] = Attr_Data;
    return true;
}

bool SparseArrayData::del(Object *o, uint index)
{
    Heap::SparseArrayData *dd = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);

    SparseArrayNode *n = dd->sparse->findNode(index);
    if (!n)
        return true;

    uint pidx = n->value;
    Q_ASSERT(!dd->arrayData[pidx].isEmpty());

    bool isAccessor = false;
    if (dd->attrs) {
        if (!dd->attrs[pidx].isConfigurable())
            return false;

        isAccessor = dd->attrs[pidx].isAccessor();
        dd->attrs[pidx] = Attr_Data;
    }

    if (isAccessor) {
        // free up both indices
        dd->arrayData[pidx + 1].tag = Value::Empty_Type;
        dd->arrayData[pidx + 1].uint_32 = dd->freeList;
        dd->arrayData[pidx].tag = Value::Undefined_Type;
        dd->arrayData[pidx].uint_32 = pidx + 1;
    } else {
        dd->arrayData[pidx].tag = Value::Empty_Type;
        dd->arrayData[pidx].uint_32 = dd->freeList;
    }

    dd->freeList = pidx;
    dd->sparse->erase(n);
    return true;
}

void SparseArrayData::setAttribute(Object *o, uint index, PropertyAttributes attrs)
{
    Heap::SparseArrayData *d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    SparseArrayNode *n = d->sparse->insert(index);
    if (n->value == UINT_MAX) {
        n->value = allocate(o, attrs.isAccessor());
        d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    }
    else if (attrs.isAccessor() != d->attrs[n->value].isAccessor()) {
        // need to convert the slot
        free(o->arrayData(), n->value);
        n->value = allocate(o, attrs.isAccessor());
        d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    }
    d->attrs[n->value] = attrs;
}

void SparseArrayData::push_front(Object *o, const Value *values, uint n)
{
    Heap::SparseArrayData *d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    Q_ASSERT(!d->attrs);
    for (int i = n - 1; i >= 0; --i) {
        uint idx = allocate(o);
        d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
        d->arrayData[idx] = values[i];
        d->sparse->push_front(idx);
    }
}

ReturnedValue SparseArrayData::pop_front(Object *o)
{
    Heap::SparseArrayData *d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    Q_ASSERT(!d->attrs);
    uint idx = d->sparse->pop_front();
    ReturnedValue v;
    if (idx != UINT_MAX) {
        v = d->arrayData[idx].asReturnedValue();
        free(o->arrayData(), idx);
    } else {
        v = Encode::undefined();
    }
    return v;
}

uint SparseArrayData::truncate(Object *o, uint newLen)
{
    Heap::SparseArrayData *d = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    SparseArrayNode *begin = d->sparse->lowerBound(newLen);
    if (begin != d->sparse->end()) {
        SparseArrayNode *it = d->sparse->end()->previousNode();
        while (1) {
            if (d->attrs) {
                if (!d->attrs[it->value].isConfigurable()) {
                    newLen = it->key() + 1;
                    break;
                }
            }
            free(o->arrayData(), it->value);
            bool brk = (it == begin);
            SparseArrayNode *prev = it->previousNode();
            d->sparse->erase(it);
            if (brk)
                break;
            it = prev;
        }
    }
    return newLen;
}

uint SparseArrayData::length(const Heap::ArrayData *d)
{
    const Heap::SparseArrayData *dd = static_cast<const Heap::SparseArrayData *>(d);
    if (!dd->sparse)
        return 0;
    SparseArrayNode *n = dd->sparse->end();
    n = n->previousNode();
    return n ? n->key() + 1 : 0;
}

bool SparseArrayData::putArray(Object *o, uint index, const Value *values, uint n)
{
    for (uint i = 0; i < n; ++i)
        put(o, index + i, values[i]);
    return true;
}


uint ArrayData::append(Object *obj, ArrayObject *otherObj, uint n)
{
    Q_ASSERT(!obj->d()->arrayData || !obj->d()->arrayData->attrs);

    if (!n)
        return obj->getLength();

    Scope scope(obj->engine());
    Scoped<ArrayData> other(scope, otherObj->arrayData());

    if (other && other->isSparse())
        obj->initSparseArray();
    else
        obj->arrayCreate();

    uint oldSize = obj->getLength();

    if (!other || ArgumentsObject::isNonStrictArgumentsObject(otherObj)) {
        ScopedValue v(scope);
        for (uint i = 0; i < n; ++i)
            obj->arraySet(oldSize + i, (v = otherObj->getIndexed(i)));
    } else if (other && other->isSparse()) {
        Heap::SparseArrayData *os = static_cast<Heap::SparseArrayData *>(other->d());
        if (other->hasAttributes()) {
            ScopedValue v(scope);
            for (const SparseArrayNode *it = os->sparse->begin();
                 it != os->sparse->end(); it = it->nextNode()) {
                v = otherObj->getValue(reinterpret_cast<Property *>(os->arrayData + it->value), other->d()->attrs[it->value]);
                obj->arraySet(oldSize + it->key(), v);
            }
        } else {
            for (const SparseArrayNode *it = other->d()->sparse->begin();
                 it != os->sparse->end(); it = it->nextNode())
                obj->arraySet(oldSize + it->key(), os->arrayData[it->value]);
        }
    } else {
        Heap::SimpleArrayData *os = static_cast<Heap::SimpleArrayData *>(other->d());
        uint toCopy = n;
        uint chunk = toCopy;
        if (chunk > os->alloc - os->offset)
            chunk -= os->alloc - os->offset;
        obj->arrayPut(oldSize, os->arrayData + os->offset, chunk);
        toCopy -= chunk;
        if (toCopy)
            obj->arrayPut(oldSize + chunk, os->arrayData, toCopy);
    }

    return oldSize + n;
}

Property *ArrayData::insert(Object *o, uint index, bool isAccessor)
{
    if (!isAccessor && o->d()->arrayData->type != Heap::ArrayData::Sparse) {
        Heap::SimpleArrayData *d = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
        if (index < 0x1000 || index < d->len + (d->len >> 2)) {
            if (index >= d->alloc) {
                o->arrayReserve(index + 1);
                d = static_cast<Heap::SimpleArrayData *>(o->d()->arrayData);
            }
            if (index >= d->len) {
                // mark possible hole in the array
                for (uint i = d->len; i < index; ++i)
                    d->data(i) = Primitive::emptyValue();
                d->len = index + 1;
            }
            return reinterpret_cast<Property *>(d->arrayData + d->mappedIndex(index));
        }
    }

    o->initSparseArray();
    Heap::SparseArrayData *s = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    SparseArrayNode *n = s->sparse->insert(index);
    if (n->value == UINT_MAX)
        n->value = SparseArrayData::allocate(o, isAccessor);
    s = static_cast<Heap::SparseArrayData *>(o->d()->arrayData);
    return reinterpret_cast<Property *>(s->arrayData + n->value);
}


class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(ExecutionEngine *engine, Object *thisObject, const Value &comparefn)
        : m_engine(engine), thisObject(thisObject), m_comparefn(comparefn) {}

    bool operator()(Value v1, Value v2) const;

private:
    ExecutionEngine *m_engine;
    Object *thisObject;
    const Value &m_comparefn;
};


bool ArrayElementLessThan::operator()(Value v1, Value v2) const
{
    Scope scope(m_engine);

    if (v1.isUndefined() || v1.isEmpty())
        return false;
    if (v2.isUndefined() || v2.isEmpty())
        return true;
    ScopedObject o(scope, m_comparefn);
    if (o) {
        Scope scope(o->engine());
        ScopedValue result(scope);
        ScopedCallData callData(scope, 2);
        callData->thisObject = Primitive::undefinedValue();
        callData->args[0] = v1;
        callData->args[1] = v2;
        result = Runtime::callValue(scope.engine, m_comparefn, callData);

        return result->toNumber() < 0;
    }
    ScopedString p1s(scope, v1.toString(scope.engine));
    ScopedString p2s(scope, v2.toString(scope.engine));
    return p1s->toQString() < p2s->toQString();
}

template <typename RandomAccessIterator, typename T, typename LessThan>
void sortHelper(RandomAccessIterator start, RandomAccessIterator end, const T &t, LessThan lessThan)
{
top:
    int span = int(end - start);
    if (span < 2)
        return;

    --end;
    RandomAccessIterator low = start, high = end - 1;
    RandomAccessIterator pivot = start + span / 2;

    if (lessThan(*end, *start))
        qSwap(*end, *start);
    if (span == 2)
        return;

    if (lessThan(*pivot, *start))
        qSwap(*pivot, *start);
    if (lessThan(*end, *pivot))
        qSwap(*end, *pivot);
    if (span == 3)
        return;

    qSwap(*pivot, *end);

    while (low < high) {
        while (low < high && lessThan(*low, *end))
            ++low;

        while (high > low && lessThan(*end, *high))
            --high;

        if (low < high) {
            qSwap(*low, *high);
            ++low;
            --high;
        } else {
            break;
        }
    }

    if (lessThan(*low, *end))
        ++low;

    qSwap(*end, *low);
    sortHelper(start, low, t, lessThan);

    start = low + 1;
    ++end;
    goto top;
}


void ArrayData::sort(ExecutionEngine *engine, Object *thisObject, const Value &comparefn, uint len)
{
    if (!len)
        return;

    Scope scope(engine);
    Scoped<ArrayData> arrayData(scope, thisObject->arrayData());

    if (!arrayData || !arrayData->length())
        return;

    if (!(comparefn.isUndefined() || comparefn.asObject())) {
        engine->throwTypeError();
        return;
    }

    // The spec says the sorting goes through a series of get,put and delete operations.
    // this implies that the attributes don't get sorted around.

    if (arrayData->type() == Heap::ArrayData::Sparse) {
        // since we sort anyway, we can simply iterate over the entries in the sparse
        // array and append them one by one to a regular one.
        Scoped<SparseArrayData> sparse(scope, static_cast<Heap::SparseArrayData *>(arrayData->d()));

        if (!sparse->sparse()->nEntries())
            return;

        thisObject->setArrayData(0);
        ArrayData::realloc(thisObject, Heap::ArrayData::Simple, sparse->sparse()->nEntries(), sparse->attrs() ? true : false);
        Heap::SimpleArrayData *d = static_cast<Heap::SimpleArrayData *>(thisObject->d()->arrayData);

        SparseArrayNode *n = sparse->sparse()->begin();
        uint i = 0;
        if (sparse->attrs()) {
            while (n != sparse->sparse()->end()) {
                if (n->value >= len)
                    break;

                PropertyAttributes a = sparse->attrs() ? sparse->attrs()[n->value] : Attr_Data;
                d->data(i) = thisObject->getValue(reinterpret_cast<Property *>(sparse->arrayData() + n->value), a);
                d->attrs[i] = a.isAccessor() ? Attr_Data : a;

                n = n->nextNode();
                ++i;
            }
        } else {
            while (n != sparse->sparse()->end()) {
                if (n->value >= len)
                    break;
                d->data(i) = sparse->arrayData()[n->value];
                n = n->nextNode();
                ++i;
            }
        }
        d->len = i;
        if (len > i)
            len = i;
        if (n != sparse->sparse()->end()) {
            // have some entries outside the sort range that we need to ignore when sorting
            thisObject->initSparseArray();
            while (n != sparse->sparse()->end()) {
                PropertyAttributes a = sparse->attrs() ? sparse->attrs()[n->value] : Attr_Data;
                thisObject->arraySet(n->value, reinterpret_cast<Property *>(sparse->arrayData() + n->value), a);

                n = n->nextNode();
            }

        }
    } else {
        Heap::SimpleArrayData *d = static_cast<Heap::SimpleArrayData *>(thisObject->d()->arrayData);
        if (len > d->len)
            len = d->len;

        // sort empty values to the end
        for (uint i = 0; i < len; i++) {
            if (d->data(i).isEmpty()) {
                while (--len > i)
                    if (!d->data(len).isEmpty())
                        break;
                Q_ASSERT(!d->attrs || !d->attrs[len].isAccessor());
                d->data(i) = d->data(len);
                d->data(len) = Primitive::emptyValue();
            }
        }

        if (!len)
            return;
    }


    ArrayElementLessThan lessThan(engine, thisObject, comparefn);

    Value *begin = thisObject->arrayData()->arrayData;
    sortHelper(begin, begin + len, *begin, lessThan);

#ifdef CHECK_SPARSE_ARRAYS
    thisObject->initSparseArray();
#endif

}
