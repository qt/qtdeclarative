/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qv4arraydata_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include "qv4mm_p.h"

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
    SimpleArrayData::Simple,
    SimpleArrayData::reallocate,
    SimpleArrayData::get,
    SimpleArrayData::put,
    SimpleArrayData::putArray,
    SimpleArrayData::del,
    SimpleArrayData::setAttribute,
    SimpleArrayData::attribute,
    SimpleArrayData::push_front,
    SimpleArrayData::pop_front,
    SimpleArrayData::truncate,
    SimpleArrayData::length
};

const ArrayVTable SparseArrayData::static_vtbl =
{
    DEFINE_MANAGED_VTABLE_INT(SparseArrayData, 0),
    ArrayData::Sparse,
    SparseArrayData::reallocate,
    SparseArrayData::get,
    SparseArrayData::put,
    SparseArrayData::putArray,
    SparseArrayData::del,
    SparseArrayData::setAttribute,
    SparseArrayData::attribute,
    SparseArrayData::push_front,
    SparseArrayData::pop_front,
    SparseArrayData::truncate,
    SparseArrayData::length
};


void ArrayData::realloc(Object *o, Type newType, uint offset, uint alloc, bool enforceAttributes)
{
    ArrayData *d = o->arrayData();

    uint oldAlloc = 0;
    uint toCopy = 0;
    if (alloc < 8)
        alloc = 8;

    if (d) {
        bool hasAttrs = d->attrs();
        enforceAttributes |= hasAttrs;

        if (!offset && alloc <= d->alloc() && newType == d->type() && hasAttrs == enforceAttributes)
            return;

        oldAlloc = d->alloc();
        if (d->type() < Sparse) {
            offset = qMax(offset, static_cast<SimpleArrayData *>(d)->offset());
            toCopy = static_cast<SimpleArrayData *>(d)->len();
        } else {
            Q_ASSERT(!offset);
            toCopy = d->alloc();
            newType = Sparse;
        }
    }
    if (enforceAttributes && newType == Simple)
        newType = Complex;

    alloc = qMax(alloc, 2*oldAlloc) + offset;
    size_t size = alloc*sizeof(Value);
    if (enforceAttributes)
        size += alloc*sizeof(PropertyAttributes);

    if (newType < Sparse) {
        size += sizeof(SimpleArrayData::Data);
        SimpleArrayData *newData = static_cast<SimpleArrayData *>(o->engine()->memoryManager->allocManaged(size));
        new (newData->d()) SimpleArrayData::Data(o->engine());
        newData->setAlloc(alloc - offset);
        newData->setType(newType);
        newData->setArrayData(reinterpret_cast<Value *>(newData->d() + 1) + offset);
        newData->setAttrs(enforceAttributes ? reinterpret_cast<PropertyAttributes *>(newData->arrayData() + alloc) + offset : 0);
        newData->offset() = offset;
        newData->len() = d ? static_cast<SimpleArrayData *>(d)->len() : 0;
        o->setArrayData(newData);
    } else {
        size += sizeof(SparseArrayData::Data);
        SparseArrayData *newData = static_cast<SparseArrayData *>(o->engine()->memoryManager->allocManaged(size));
        new (newData->d()) SparseArrayData::Data(o->engine());
        newData->setAlloc(alloc);
        newData->setType(newType);
        newData->setArrayData(reinterpret_cast<Value *>(newData->d() + 1));
        newData->setAttrs(enforceAttributes ? reinterpret_cast<PropertyAttributes *>(newData->arrayData() + alloc) : 0);
        o->setArrayData(newData);
    }

    if (d) {
        memcpy(o->arrayData()->arrayData(), d->arrayData(), sizeof(Value)*toCopy);
        if (enforceAttributes) {
            if (d->attrs())
                memcpy(o->arrayData()->attrs(), d->attrs(), sizeof(PropertyAttributes)*toCopy);
            else
                for (uint i = 0; i < toCopy; ++i)
                    o->arrayData()->attrs()[i] = Attr_Data;
        }
    }

    if (newType != Sparse)
        return;

    SparseArrayData *newData = static_cast<SparseArrayData *>(o->arrayData());
    if (d && d->type() == Sparse) {
        SparseArrayData *old = static_cast<SparseArrayData *>(d);
        newData->setSparse(old->sparse());
        old->setSparse(0);
        newData->freeList() = old->freeList();
    } else {
        newData->setSparse(new SparseArray);
        uint *lastFree = &newData->freeList();
        for (uint i = 0; i < toCopy; ++i) {
            if (!newData->arrayData()[i].isEmpty()) {
                SparseArrayNode *n = newData->sparse()->insert(i);
                n->value = i;
            } else {
                *lastFree = i;
                newData->arrayData()[i].tag = Value::Empty_Type;
                lastFree = &newData->arrayData()[i].uint_32;
            }
        }
    }

    uint *lastFree = &newData->freeList();
    for (uint i = toCopy; i < newData->alloc(); ++i) {
        *lastFree = i;
        newData->arrayData()[i].tag = Value::Empty_Type;
        lastFree = &newData->arrayData()[i].uint_32;
    }
    *lastFree = newData->alloc();

    // ### Could explicitly free the old data
}


void SimpleArrayData::getHeadRoom(Object *o)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    Q_ASSERT(dd);
    Q_ASSERT(!dd->offset());
    uint offset = qMax(dd->len() >> 2, (uint)16);
    realloc(o, Simple, offset, 0, false);
}

ArrayData *SimpleArrayData::reallocate(Object *o, uint n, bool enforceAttributes)
{
    realloc(o, Simple, 0, n, enforceAttributes);
    return o->arrayData();
}

void ArrayData::ensureAttributes(Object *o)
{
    if (o->arrayData() && o->arrayData()->attrs())
        return;

    ArrayData::realloc(o, Simple, 0, 0, true);
}


void SimpleArrayData::markObjects(Managed *d, ExecutionEngine *e)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    uint l = dd->len();
    for (uint i = 0; i < l; ++i)
        dd->arrayData()[i].mark(e);
}

ReturnedValue SimpleArrayData::get(const ArrayData *d, uint index)
{
    const SimpleArrayData *dd = static_cast<const SimpleArrayData *>(d);
    if (index >= dd->len())
        return Primitive::emptyValue().asReturnedValue();
    return dd->arrayData()[index].asReturnedValue();
}

bool SimpleArrayData::put(Object *o, uint index, ValueRef value)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    Q_ASSERT(index >= dd->len() || !dd->attrs() || !dd->attrs()[index].isAccessor());
    // ### honour attributes
    dd->arrayData()[index] = value;
    if (index >= dd->len()) {
        if (dd->attrs())
            dd->attrs()[index] = Attr_Data;
        dd->len() = index + 1;
    }
    return true;
}

bool SimpleArrayData::del(Object *o, uint index)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    if (index >= dd->len())
        return true;

    if (!dd->attrs() || dd->attrs()[index].isConfigurable()) {
        dd->arrayData()[index] = Primitive::emptyValue();
        if (dd->attrs())
            dd->attrs()[index] = Attr_Data;
        return true;
    }
    if (dd->arrayData()[index].isEmpty())
        return true;
    return false;
}

void SimpleArrayData::setAttribute(Object *o, uint index, PropertyAttributes attrs)
{
    o->arrayData()->attrs()[index] = attrs;
}

PropertyAttributes SimpleArrayData::attribute(const ArrayData *d, uint index)
{
    return d->attrs()[index];
}

void SimpleArrayData::push_front(Object *o, Value *values, uint n)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    Q_ASSERT(!dd->attrs());
    for (int i = n - 1; i >= 0; --i) {
        if (!dd->offset()) {
            getHeadRoom(o);
            dd = static_cast<SimpleArrayData *>(o->arrayData());
        }


        --dd->offset();
        --dd->arrayData();
        ++dd->len();
        ++dd->alloc();
        *dd->arrayData() = values[i].asReturnedValue();
    }

}

ReturnedValue SimpleArrayData::pop_front(Object *o)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    Q_ASSERT(!dd->attrs());
    if (!dd->len())
        return Encode::undefined();

    ReturnedValue v = dd->arrayData()[0].isEmpty() ? Encode::undefined() : dd->arrayData()[0].asReturnedValue();
    ++dd->offset();
    ++dd->arrayData();
    --dd->len();
    --dd->alloc();
    return v;
}

uint SimpleArrayData::truncate(Object *o, uint newLen)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    if (dd->len() < newLen)
        return newLen;

    if (dd->attrs()) {
        Value *it = dd->arrayData() + dd->len();
        const Value *begin = dd->arrayData() + newLen;
        while (--it >= begin) {
            if (!it->isEmpty() && !dd->attrs()[it - dd->arrayData()].isConfigurable()) {
                newLen = it - dd->arrayData() + 1;
                break;
            }
            *it = Primitive::emptyValue();
        }
    }
    dd->len() = newLen;
    return newLen;
}

uint SimpleArrayData::length(const ArrayData *d)
{
    return static_cast<const SimpleArrayData *>(d)->len();
}

bool SimpleArrayData::putArray(Object *o, uint index, Value *values, uint n)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(o->arrayData());
    if (index + n > dd->alloc()) {
        reallocate(o, index + n + 1, false);
        dd = static_cast<SimpleArrayData *>(o->arrayData());
    }
    for (uint i = dd->len(); i < index; ++i)
        dd->arrayData()[i] = Primitive::emptyValue();
    for (uint i = 0; i < n; ++i)
        dd->arrayData()[index + i] = values[i];
    dd->len() = qMax(dd->len(), index + n);
    return true;
}

void SparseArrayData::free(ArrayData *d, uint idx)
{
    Q_ASSERT(d && d->type() == ArrayData::Sparse);
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    Value *v = dd->arrayData() + idx;
    if (dd->attrs() && dd->attrs()[idx].isAccessor()) {
        // double slot, free both. Order is important, so we have a double slot for allocation again afterwards.
        v[1].tag = Value::Empty_Type;
        v[1].uint_32 = dd->freeList();
        v[0].tag = Value::Empty_Type;
        v[0].uint_32 = idx + 1;
    } else {
        v->tag = Value::Empty_Type;
        v->uint_32 = dd->freeList();
    }
    dd->freeList() = idx;
    if (dd->attrs())
        dd->attrs()[idx].clear();
}


void SparseArrayData::destroy(Managed *d)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    delete dd->sparse();
}

void SparseArrayData::markObjects(Managed *d, ExecutionEngine *e)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    uint l = dd->alloc();
    for (uint i = 0; i < l; ++i)
        dd->arrayData()[i].mark(e);
}

ArrayData *SparseArrayData::reallocate(Object *o, uint n, bool enforceAttributes)
{
    realloc(o, Sparse, 0, n, enforceAttributes);
    return o->arrayData();
}

// double slots are required for accessor properties
uint SparseArrayData::allocate(Object *o, bool doubleSlot)
{
    Q_ASSERT(o->arrayData()->type() == ArrayData::Sparse);
    SparseArrayData *dd = static_cast<SparseArrayData *>(o->arrayData());
    if (doubleSlot) {
        uint *last = &dd->freeList();
        while (1) {
            if (*last + 1 >= dd->alloc()) {
                reallocate(o, o->arrayData()->alloc() + 2, true);
                dd = static_cast<SparseArrayData *>(o->arrayData());
                last = &dd->freeList();
            }

            if (dd->arrayData()[*last].uint_32 == (*last + 1)) {
                // found two slots in a row
                uint idx = *last;
                *last = dd->arrayData()[*last + 1].uint_32;
                o->arrayData()->attrs()[idx] = Attr_Accessor;
                return idx;
            }
            last = &dd->arrayData()[*last].uint_32;
        }
    } else {
        if (dd->alloc() == dd->freeList()) {
            reallocate(o, o->arrayData()->alloc() + 2, false);
            dd = static_cast<SparseArrayData *>(o->arrayData());
        }
        uint idx = dd->freeList();
        dd->freeList() = dd->arrayData()[idx].uint_32;
        if (dd->attrs())
            dd->attrs()[idx] = Attr_Data;
        return idx;
    }
}

ReturnedValue SparseArrayData::get(const ArrayData *d, uint index)
{
    SparseArrayNode *n = static_cast<const SparseArrayData *>(d)->sparse()->findNode(index);
    if (!n)
        return Primitive::emptyValue().asReturnedValue();
    return d->arrayData()[n->value].asReturnedValue();
}

bool SparseArrayData::put(Object *o, uint index, ValueRef value)
{
    if (value->isEmpty())
        return true;

    SparseArrayNode *n = static_cast<SparseArrayData *>(o->arrayData())->sparse()->insert(index);
    Q_ASSERT(n->value == UINT_MAX || !o->arrayData()->attrs() || !o->arrayData()->attrs()[n->value].isAccessor());
    if (n->value == UINT_MAX)
        n->value = allocate(o);
    o->arrayData()->arrayData()[n->value] = value;
    if (o->arrayData()->attrs())
        o->arrayData()->attrs()[n->value] = Attr_Data;
    return true;
}

bool SparseArrayData::del(Object *o, uint index)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(o->arrayData());
    SparseArrayNode *n = dd->sparse()->findNode(index);
    if (!n)
        return true;

    uint pidx = n->value;
    Q_ASSERT(!dd->arrayData()[pidx].isEmpty());

    bool isAccessor = false;
    if (dd->attrs()) {
        if (!dd->attrs()[pidx].isConfigurable())
            return false;

        isAccessor = dd->attrs()[pidx].isAccessor();
        dd->attrs()[pidx] = Attr_Data;
    }

    if (isAccessor) {
        // free up both indices
        dd->arrayData()[pidx + 1].tag = Value::Undefined_Type;
        dd->arrayData()[pidx + 1].uint_32 = static_cast<SparseArrayData *>(dd)->freeList();
        dd->arrayData()[pidx].tag = Value::Undefined_Type;
        dd->arrayData()[pidx].uint_32 = pidx + 1;
    } else {
        dd->arrayData()[pidx].tag = Value::Undefined_Type;
        dd->arrayData()[pidx].uint_32 = static_cast<SparseArrayData *>(dd)->freeList();
    }

    dd->freeList() = pidx;
    dd->sparse()->erase(n);
    return true;
}

void SparseArrayData::setAttribute(Object *o, uint index, PropertyAttributes attrs)
{
    SparseArrayData *d = static_cast<SparseArrayData *>(o->arrayData());
    SparseArrayNode *n = d->sparse()->insert(index);
    if (n->value == UINT_MAX) {
        n->value = allocate(o, attrs.isAccessor());
        d = static_cast<SparseArrayData *>(o->arrayData());
    }
    else if (attrs.isAccessor() != d->attrs()[n->value].isAccessor()) {
        // need to convert the slot
        free(d, n->value);
        n->value = allocate(o, attrs.isAccessor());
    }
    o->arrayData()->attrs()[n->value] = attrs;
}

PropertyAttributes SparseArrayData::attribute(const ArrayData *d, uint index)
{
    SparseArrayNode *n = static_cast<const SparseArrayData *>(d)->sparse()->insert(index);
    if (!n)
        return PropertyAttributes();
    return d->attrs()[n->value];
}

void SparseArrayData::push_front(Object *o, Value *values, uint n)
{
    Q_ASSERT(!o->arrayData()->attrs());
    for (int i = n - 1; i >= 0; --i) {
        uint idx = allocate(o);
        o->arrayData()->arrayData()[idx] = values[i];
        static_cast<SparseArrayData *>(o->arrayData())->sparse()->push_front(idx);
    }
}

ReturnedValue SparseArrayData::pop_front(Object *o)
{
    Q_ASSERT(!o->arrayData()->attrs());
    uint idx = static_cast<SparseArrayData *>(o->arrayData())->sparse()->pop_front();
    ReturnedValue v;
    if (idx != UINT_MAX) {
        v = o->arrayData()->arrayData()[idx].asReturnedValue();
        free(o->arrayData(), idx);
    } else {
        v = Encode::undefined();
    }
    return v;
}

uint SparseArrayData::truncate(Object *o, uint newLen)
{
    SparseArrayData *d = static_cast<SparseArrayData *>(o->arrayData());
    SparseArrayNode *begin = d->sparse()->lowerBound(newLen);
    if (begin != d->sparse()->end()) {
        SparseArrayNode *it = d->sparse()->end()->previousNode();
        while (1) {
            if (d->attrs()) {
                if (!d->attrs()[it->value].isConfigurable()) {
                    newLen = it->key() + 1;
                    break;
                }
            }
            free(d, it->value);
            bool brk = (it == begin);
            SparseArrayNode *prev = it->previousNode();
            static_cast<SparseArrayData *>(d)->sparse()->erase(it);
            if (brk)
                break;
            it = prev;
        }
    }
    return newLen;
}

uint SparseArrayData::length(const ArrayData *d)
{
    const SparseArrayData *dd = static_cast<const SparseArrayData *>(d);
    if (!dd->sparse())
        return 0;
    SparseArrayNode *n = dd->sparse()->end();
    n = n->previousNode();
    return n ? n->key() + 1 : 0;
}

bool SparseArrayData::putArray(Object *o, uint index, Value *values, uint n)
{
    for (uint i = 0; i < n; ++i)
        put(o, index + i, values[i]);
    return true;
}


uint ArrayData::append(Object *obj, const ArrayObject *otherObj, uint n)
{
    Q_ASSERT(!obj->arrayData()->hasAttributes());

    if (!n)
        return obj->getLength();

    const ArrayData *other = otherObj->arrayData();

    if (other->isSparse())
        obj->initSparseArray();
    else
        obj->arrayCreate();

    uint oldSize = obj->getLength();

    if (other->isSparse()) {
        if (otherObj->hasAccessorProperty() && other->hasAttributes()) {
            Scope scope(obj->engine());
            ScopedValue v(scope);
            for (const SparseArrayNode *it = static_cast<const SparseArrayData *>(other)->sparse()->begin();
                 it != static_cast<const SparseArrayData *>(other)->sparse()->end(); it = it->nextNode()) {
                v = otherObj->getValue(reinterpret_cast<Property *>(other->arrayData() + it->value), other->attrs()[it->value]);
                obj->arraySet(oldSize + it->key(), v);
            }
        } else {
            for (const SparseArrayNode *it = static_cast<const SparseArrayData *>(other)->sparse()->begin();
                 it != static_cast<const SparseArrayData *>(other)->sparse()->end(); it = it->nextNode())
                obj->arraySet(oldSize + it->key(), ValueRef(other->arrayData()[it->value]));
        }
    } else {
        obj->arrayPut(oldSize, other->arrayData(), n);
    }

    return oldSize + n;
}

Property *ArrayData::insert(Object *o, uint index, bool isAccessor)
{
    if (!isAccessor && o->arrayData()->type() != ArrayData::Sparse) {
        SimpleArrayData *d = static_cast<SimpleArrayData *>(o->arrayData());
        if (index < 0x1000 || index < d->len() + (d->len() >> 2)) {
            if (index >= o->arrayData()->alloc()) {
                o->arrayReserve(index + 1);
                d = static_cast<SimpleArrayData *>(o->arrayData());
            }
            if (index >= d->len()) {
                // mark possible hole in the array
                for (uint i = d->len(); i < index; ++i)
                    d->arrayData()[i] = Primitive::emptyValue();
                d->len() = index + 1;
            }
            return reinterpret_cast<Property *>(o->arrayData()->arrayData() + index);
        }
    }

    o->initSparseArray();
    SparseArrayNode *n = static_cast<SparseArrayData *>(o->arrayData())->sparse()->insert(index);
    if (n->value == UINT_MAX)
        n->value = SparseArrayData::allocate(o, isAccessor);
    return reinterpret_cast<Property *>(o->arrayData()->arrayData() + n->value);
}


class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(ExecutionContext *context, Object *thisObject, const ValueRef comparefn)
        : m_context(context), thisObject(thisObject), m_comparefn(comparefn) {}

    bool operator()(Value v1, Value v2) const;

private:
    ExecutionContext *m_context;
    Object *thisObject;
    const ValueRef m_comparefn;
};


bool ArrayElementLessThan::operator()(Value v1, Value v2) const
{
    Scope scope(m_context);

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
        result = Runtime::callValue(m_context, m_comparefn, callData);

        return result->toNumber() < 0;
    }
    ScopedString p1s(scope, v1.toString(m_context));
    ScopedString p2s(scope, v2.toString(m_context));
    return p1s->toQString() < p2s->toQString();
}

void ArrayData::sort(ExecutionContext *context, Object *thisObject, const ValueRef comparefn, uint len)
{
    if (!len)
        return;

    if (!thisObject->arrayData()->length())
        return;

    if (!(comparefn->isUndefined() || comparefn->asObject())) {
        context->throwTypeError();
        return;
    }

    // The spec says the sorting goes through a series of get,put and delete operations.
    // this implies that the attributes don't get sorted around.

    if (thisObject->arrayData()->type() == ArrayData::Sparse) {
        // since we sort anyway, we can simply iterate over the entries in the sparse
        // array and append them one by one to a regular one.
        SparseArrayData *sparse = static_cast<SparseArrayData *>(thisObject->arrayData());

        if (!sparse->sparse()->nEntries())
            return;

        thisObject->setArrayData(0);
        ArrayData::realloc(thisObject, ArrayData::Simple, 0, sparse->sparse()->nEntries(), sparse->attrs() ? true : false);
        SimpleArrayData *d = static_cast<SimpleArrayData *>(thisObject->arrayData());

        SparseArrayNode *n = sparse->sparse()->begin();
        uint i = 0;
        if (sparse->attrs()) {
            while (n != sparse->sparse()->end()) {
                if (n->value >= len)
                    break;

                PropertyAttributes a = sparse->attrs() ? sparse->attrs()[n->value] : Attr_Data;
                d->arrayData()[i] = thisObject->getValue(reinterpret_cast<Property *>(sparse->arrayData() + n->value), a);
                d->attrs()[i] = a.isAccessor() ? Attr_Data : a;

                n = n->nextNode();
                ++i;
            }
        } else {
            while (n != sparse->sparse()->end()) {
                if (n->value >= len)
                    break;
                d->arrayData()[i] = sparse->arrayData()[n->value];
                n = n->nextNode();
                ++i;
            }
        }
        d->len() = i;
        if (len > i)
            len = i;
        if (n != sparse->sparse()->end()) {
            // have some entries outside the sort range that we need to ignore when sorting
            thisObject->initSparseArray();
            while (n != sparse->sparse()->end()) {
                PropertyAttributes a = sparse->attrs() ? sparse->attrs()[n->value] : Attr_Data;
                thisObject->arraySet(n->value, *reinterpret_cast<Property *>(sparse->arrayData() + n->value), a);

                n = n->nextNode();
            }

        }
        // ### explicitly delete sparse
    } else {
        SimpleArrayData *d = static_cast<SimpleArrayData *>(thisObject->arrayData());
        if (len > d->len())
            len = d->len();

        // sort empty values to the end
        for (uint i = 0; i < len; i++) {
            if (thisObject->arrayData()->arrayData()[i].isEmpty()) {
                while (--len > i)
                    if (!thisObject->arrayData()->arrayData()[len].isEmpty())
                        break;
                Q_ASSERT(!thisObject->arrayData()->attrs() || !thisObject->arrayData()->attrs()[len].isAccessor());
                thisObject->arrayData()->arrayData()[i] = thisObject->arrayData()->arrayData()[len];
                thisObject->arrayData()->arrayData()[len] = Primitive::emptyValue();
            }
        }

        if (!len)
            return;
    }


    ArrayElementLessThan lessThan(context, thisObject, comparefn);

    Value *begin = thisObject->arrayData()->arrayData();
    std::sort(begin, begin + len, lessThan);

#ifdef CHECK_SPARSE_ARRAYS
    thisObject->initSparseArray();
#endif

}
