/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "qv4arraydata_p.h"
#include "qv4object_p.h"
#include "qv4functionobject_p.h"

using namespace QV4;

const ArrayVTable SimpleArrayData::static_vtbl =
{
    DEFINE_MANAGED_VTABLE_INT(SimpleArrayData),
    SimpleArrayData::Simple,
    SimpleArrayData::reserve,
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
    DEFINE_MANAGED_VTABLE_INT(SparseArrayData),
    ArrayData::Sparse,
    SparseArrayData::reserve,
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


void SimpleArrayData::getHeadRoom(ArrayData *d)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    Q_ASSERT(dd);
    Q_ASSERT(!dd->offset);
    dd->offset = qMax(dd->len >> 2, (uint)16);
    SafeValue *newArray = new SafeValue[dd->offset + dd->alloc];
    memcpy(newArray + dd->offset, dd->data, dd->len*sizeof(SafeValue));
    delete [] dd->data;
    dd->data = newArray + dd->offset;
    if (dd->attrs) {
        PropertyAttributes *newAttrs = new PropertyAttributes[dd->offset + dd->alloc];
        memcpy(newAttrs + dd->offset, dd->attrs, dd->len*sizeof(PropertyAttributes));
        delete [] dd->attrs;
        dd->attrs = newAttrs + dd->offset;
    }
}

void SimpleArrayData::reserve(ArrayData *d, uint n)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    if (n < 8)
        n = 8;
    if (n <= dd->alloc)
        return;

    dd->alloc = qMax(n, 2*dd->alloc);
    SafeValue *newArrayData = new SafeValue[dd->alloc + dd->offset];
    if (dd->data) {
        memcpy(newArrayData + dd->offset, dd->data, sizeof(SafeValue)*dd->len);
        delete [] (dd->data - dd->offset);
    }
    dd->data = newArrayData + dd->offset;

    if (dd->attrs) {
        PropertyAttributes *newAttrs = new PropertyAttributes[dd->alloc];
        memcpy(newAttrs, dd->attrs, sizeof(PropertyAttributes)*dd->len);
        delete [] (dd->attrs - dd->offset);

        dd->attrs = newAttrs;
    }
}

void ArrayData::ensureAttributes()
{
    if (attrs)
        return;

    uint off = 0;
    if (type == Simple) {
        type = Complex;
        off = static_cast<SimpleArrayData *>(this)->offset;
    }
    attrs = new PropertyAttributes[alloc + off];
    attrs += off;
    for (uint i = 0; i < alloc; ++i)
        attrs[i] = Attr_Data;
}


void SimpleArrayData::destroy(Managed *d)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    delete [] (dd->data - dd->offset);
    if (dd->attrs)
        delete [] (dd->attrs - dd->offset);
}

void SimpleArrayData::markObjects(Managed *d, ExecutionEngine *e)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    uint l = dd->len;
    for (uint i = 0; i < l; ++i)
        dd->data[i].mark(e);
}

ReturnedValue SimpleArrayData::get(const ArrayData *d, uint index)
{
    const SimpleArrayData *dd = static_cast<const SimpleArrayData *>(d);
    if (index >= dd->len)
        return Primitive::emptyValue().asReturnedValue();
    return dd->data[index].asReturnedValue();
}

bool SimpleArrayData::put(ArrayData *d, uint index, ValueRef value)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    Q_ASSERT(index >= dd->len || !dd->attrs || !dd->attrs[index].isAccessor());
    // ### honour attributes
    dd->data[index] = value;
    if (index >= dd->len) {
        if (dd->attrs)
            dd->attrs[index] = Attr_Data;
        dd->len = index + 1;
    }
    return true;
}

bool SimpleArrayData::del(ArrayData *d, uint index)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    if (index >= dd->len)
        return true;

    if (!dd->attrs || dd->attrs[index].isConfigurable()) {
        dd->data[index] = Primitive::emptyValue();
        if (dd->attrs)
            dd->attrs[index] = Attr_Data;
        return true;
    }
    if (dd->data[index].isEmpty())
        return true;
    return false;
}

void SimpleArrayData::setAttribute(ArrayData *d, uint index, PropertyAttributes attrs)
{
    d->attrs[index] = attrs;
}

PropertyAttributes SimpleArrayData::attribute(const ArrayData *d, uint index)
{
    return d->attrs[index];
}

void SimpleArrayData::push_front(ArrayData *d, SafeValue *values, uint n)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    Q_ASSERT(!dd->attrs);
    for (int i = n - 1; i >= 0; --i) {
        if (!dd->offset)
            getHeadRoom(dd);

        --dd->offset;
        --dd->data;
        ++dd->len;
        ++dd->alloc;
        *dd->data = values[i].asReturnedValue();
    }

}

ReturnedValue SimpleArrayData::pop_front(ArrayData *d)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    Q_ASSERT(!dd->attrs);
    if (!dd->len)
        return Encode::undefined();

    ReturnedValue v = dd->data[0].isEmpty() ? Encode::undefined() : dd->data[0].asReturnedValue();
    ++dd->offset;
    ++dd->data;
    --dd->len;
    --dd->alloc;
    return v;
}

uint SimpleArrayData::truncate(ArrayData *d, uint newLen)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    if (dd->len < newLen)
        return newLen;

    if (dd->attrs) {
        SafeValue *it = dd->data + dd->len;
        const SafeValue *begin = dd->data + newLen;
        while (--it >= begin) {
            if (!it->isEmpty() && !dd->attrs[it - dd->data].isConfigurable()) {
                newLen = it - dd->data + 1;
                break;
            }
            *it = Primitive::emptyValue();
        }
    }
    dd->len = newLen;
    return newLen;
}

uint SimpleArrayData::length(const ArrayData *d)
{
    return static_cast<const SimpleArrayData *>(d)->len;
}

bool SimpleArrayData::putArray(ArrayData *d, uint index, SafeValue *values, uint n)
{
    SimpleArrayData *dd = static_cast<SimpleArrayData *>(d);
    if (index + n > dd->alloc)
        reserve(dd, index + n + 1);
    for (uint i = dd->len; i < index; ++i)
        dd->data[i] = Primitive::emptyValue();
    for (uint i = 0; i < n; ++i)
        dd->data[index + i] = values[i];
    dd->len = qMax(dd->len, index + n);
    return true;
}

void SparseArrayData::free(ArrayData *d, uint idx)
{
    Q_ASSERT(d && d->type == ArrayData::Sparse);
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    SafeValue *v = dd->data + idx;
    if (dd->attrs && dd->attrs[idx].isAccessor()) {
        // double slot, free both. Order is important, so we have a double slot for allocation again afterwards.
        v[1].tag = Value::Empty_Type;
        v[1].uint_32 = dd->freeList;
        v[0].tag = Value::Empty_Type;
        v[0].uint_32 = idx + 1;
    } else {
        v->tag = Value::Empty_Type;
        v->uint_32 = dd->freeList;
    }
    dd->freeList = idx;
    if (dd->attrs)
        dd->attrs[idx].clear();
}


void SparseArrayData::destroy(Managed *d)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    delete dd->sparse;
    delete [] dd->data;
    if (dd->attrs)
        delete [] dd->attrs;
}

void SparseArrayData::markObjects(Managed *d, ExecutionEngine *e)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    uint l = dd->alloc;
    for (uint i = 0; i < l; ++i)
        dd->data[i].mark(e);
}

void SparseArrayData::reserve(ArrayData *d, uint n)
{
    if (n < 8)
        n = 8;
    if (n <= d->alloc)
        return;

    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    uint oldAlloc = dd->alloc;
    dd->alloc = qMax(n, 2*dd->alloc);
    SafeValue *newArrayData = new SafeValue[dd->alloc];
    if (dd->data) {
        memcpy(newArrayData, dd->data, sizeof(SafeValue)*oldAlloc);
        delete [] dd->data;
    }
    dd->data = newArrayData;
    if (dd->attrs) {
        PropertyAttributes *newAttrs = new PropertyAttributes[dd->alloc];
        memcpy(newAttrs, dd->attrs, sizeof(PropertyAttributes)*oldAlloc);
        delete [] dd->attrs;
        dd->attrs = newAttrs;
    }
    for (uint i = oldAlloc; i < dd->alloc; ++i)
        dd->data[i] = Primitive::fromInt32(i + 1);
}

// double slots are required for accessor properties
uint SparseArrayData::allocate(ArrayData *d, bool doubleSlot)
{
    Q_ASSERT(d->type == ArrayData::Sparse);
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    if (doubleSlot) {
        uint *last = &dd->freeList;
        while (1) {
            if (*last + 1 >= dd->alloc) {
                reserve(d, d->alloc + 2);
                last = &dd->freeList;
            }

            if (dd->data[*last].uint_32 == (*last + 1)) {
                // found two slots in a row
                uint idx = *last;
                *last = dd->data[*last + 1].uint_32;
                d->attrs[idx] = Attr_Accessor;
                return idx;
            }
            last = &dd->data[*last].uint_32;
        }
    } else {
        if (dd->alloc == dd->freeList)
            reserve(d, d->alloc + 2);
        uint idx = dd->freeList;
        dd->freeList = dd->data[idx].uint_32;
        if (dd->attrs)
            dd->attrs[idx] = Attr_Data;
        return idx;
    }
}

ReturnedValue SparseArrayData::get(const ArrayData *d, uint index)
{
    SparseArrayNode *n = static_cast<const SparseArrayData *>(d)->sparse->findNode(index);
    if (!n)
        return Primitive::emptyValue().asReturnedValue();
    return d->data[n->value].asReturnedValue();
}

bool SparseArrayData::put(ArrayData *d, uint index, ValueRef value)
{
    if (value->isEmpty())
        return true;

    SparseArrayNode *n = static_cast<SparseArrayData *>(d)->sparse->insert(index);
    Q_ASSERT(n->value == UINT_MAX || !d->attrs || !d->attrs[n->value].isAccessor());
    if (n->value == UINT_MAX)
        n->value = allocate(d);
    d->data[n->value] = value;
    if (d->attrs)
        d->attrs[n->value] = Attr_Data;
    return true;
}

bool SparseArrayData::del(ArrayData *d, uint index)
{
    SparseArrayData *dd = static_cast<SparseArrayData *>(d);
    SparseArrayNode *n = dd->sparse->findNode(index);
    if (!n)
        return true;

    uint pidx = n->value;
    Q_ASSERT(!dd->data[pidx].isEmpty());

    bool isAccessor = false;
    if (dd->attrs) {
        if (!dd->attrs[pidx].isConfigurable())
            return false;

        isAccessor = dd->attrs[pidx].isAccessor();
        dd->attrs[pidx] = Attr_Data;
    }

    if (isAccessor) {
        // free up both indices
        d->data[pidx + 1].tag = Value::Undefined_Type;
        d->data[pidx + 1].uint_32 = static_cast<SparseArrayData *>(d)->freeList;
        d->data[pidx].tag = Value::Undefined_Type;
        d->data[pidx].uint_32 = pidx + 1;
    } else {
        d->data[pidx].tag = Value::Undefined_Type;
        d->data[pidx].uint_32 = static_cast<SparseArrayData *>(d)->freeList;
    }

    static_cast<SparseArrayData *>(d)->freeList = pidx;
    static_cast<SparseArrayData *>(d)->sparse->erase(n);
    return true;
}

void SparseArrayData::setAttribute(ArrayData *d, uint index, PropertyAttributes attrs)
{
    SparseArrayNode *n = static_cast<SparseArrayData *>(d)->sparse->insert(index);
    if (n->value == UINT_MAX)
        n->value = allocate(d, attrs.isAccessor());
    else if (attrs.isAccessor() != d->attrs[n->value].isAccessor()) {
        // need to convert the slot
        free(d, n->value);
        n->value = allocate(d, attrs.isAccessor());
    }
    d->attrs[n->value] = attrs;
}

PropertyAttributes SparseArrayData::attribute(const ArrayData *d, uint index)
{
    SparseArrayNode *n = static_cast<const SparseArrayData *>(d)->sparse->insert(index);
    if (!n)
        return PropertyAttributes();
    return d->attrs[n->value];
}

void SparseArrayData::push_front(ArrayData *d, SafeValue *values, uint n)
{
    Q_ASSERT(!d->attrs);
    for (int i = n - 1; i >= 0; --i) {
        uint idx = allocate(d);
        d->data[idx] = values[i];
        static_cast<SparseArrayData *>(d)->sparse->push_front(idx);
    }
}

ReturnedValue SparseArrayData::pop_front(ArrayData *d)
{
    Q_ASSERT(!d->attrs);
    uint idx = static_cast<SparseArrayData *>(d)->sparse->pop_front();
    ReturnedValue v;
    if (idx != UINT_MAX) {
        v = d->data[idx].asReturnedValue();
        free(d, idx);
    } else {
        v = Encode::undefined();
    }
    return v;
}

uint SparseArrayData::truncate(ArrayData *d, uint newLen)
{
    SparseArrayNode *begin = static_cast<SparseArrayData *>(d)->sparse->lowerBound(newLen);
    if (begin != static_cast<SparseArrayData *>(d)->sparse->end()) {
        SparseArrayNode *it = static_cast<SparseArrayData *>(d)->sparse->end()->previousNode();
        while (1) {
            if (d->attrs) {
                if (!d->attrs[it->value].isConfigurable()) {
                    newLen = it->key() + 1;
                    break;
                }
            }
            free(d, it->value);
            bool brk = (it == begin);
            SparseArrayNode *prev = it->previousNode();
            static_cast<SparseArrayData *>(d)->sparse->erase(it);
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
    if (!dd->sparse)
        return 0;
    SparseArrayNode *n = dd->sparse->end();
    n = n->previousNode();
    return n ? n->key() + 1 : 0;
}

bool SparseArrayData::putArray(ArrayData *d, uint index, SafeValue *values, uint n)
{
    for (uint i = 0; i < n; ++i)
        put(d, index + i, values[i]);
    return true;
}


uint ArrayData::append(Object *obj, const ArrayObject *otherObj, uint n)
{
    Q_ASSERT(!obj->arrayData->hasAttributes());

    if (!n)
        return obj->getLength();

    const ArrayData *other = otherObj->arrayData;

    if (other->isSparse())
        obj->initSparseArray();

    uint oldSize = obj->getLength();

    if (other->isSparse()) {
        if (otherObj->hasAccessorProperty && other->hasAttributes()) {
            Scope scope(obj->engine());
            ScopedValue v(scope);
            for (const SparseArrayNode *it = static_cast<const SparseArrayData *>(other)->sparse->begin();
                 it != static_cast<const SparseArrayData *>(other)->sparse->end(); it = it->nextNode()) {
                v = otherObj->getValue(reinterpret_cast<Property *>(other->data + it->value), other->attrs[it->value]);
                obj->arraySet(oldSize + it->key(), v);
            }
        } else {
            for (const SparseArrayNode *it = static_cast<const SparseArrayData *>(other)->sparse->begin();
                 it != static_cast<const SparseArrayData *>(other)->sparse->end(); it = it->nextNode())
                obj->arraySet(oldSize + it->key(), other->data[it->value]);
        }
    } else {
        obj->arrayData->put(oldSize, other->data, n);
    }

    return oldSize + n;
}

Property *ArrayData::insert(Object *o, uint index, bool isAccessor)
{
    if (!isAccessor && o->arrayData->type != ArrayData::Sparse) {
        SimpleArrayData *d = static_cast<SimpleArrayData *>(o->arrayData);
        if (index < 0x1000 || index < d->len + (d->len >> 2)) {
            if (index >= o->arrayData->alloc) {
                o->arrayReserve(index + 1);
                d = static_cast<SimpleArrayData *>(o->arrayData);
            }
            if (index >= d->len) {
                // mark possible hole in the array
                for (uint i = d->len; i < index; ++i)
                    d->data[i] = Primitive::emptyValue();
                d->len = index + 1;
            }
            return reinterpret_cast<Property *>(o->arrayData->data + index);
        }
    }

    o->initSparseArray();
    SparseArrayNode *n = static_cast<SparseArrayData *>(o->arrayData)->sparse->insert(index);
    if (n->value == UINT_MAX)
        n->value = SparseArrayData::allocate(o->arrayData, isAccessor);
    return reinterpret_cast<Property *>(o->arrayData->data + n->value);
}

void ArrayData::sort(ExecutionContext *context, ObjectRef thisObject, const ValueRef comparefn, uint len)
{
    if (!len)
        return;

    if (!thisObject->arrayData->length())
        return;

    if (!(comparefn->isUndefined() || comparefn->asObject())) {
        context->throwTypeError();
        return;
    }

    // The spec says the sorting goes through a series of get,put and delete operations.
    // this implies that the attributes don't get sorted around.

    if (thisObject->arrayData->type == ArrayData::Sparse) {
        // since we sort anyway, we can simply iterate over the entries in the sparse
        // array and append them one by one to a regular one.
        SparseArrayData *sparse = static_cast<SparseArrayData *>(thisObject->arrayData);

        if (!sparse->sparse->nEntries())
            return;

        SimpleArrayData *d = new (context->engine->memoryManager) SimpleArrayData(context->engine);
        thisObject->arrayData = d;
        d->vtable()->reserve(d, sparse->sparse->nEntries());

        SparseArrayNode *n = sparse->sparse->begin();
        uint i = 0;
        if (sparse->attrs) {
            d->ensureAttributes();
            while (n != sparse->sparse->end()) {
                if (n->value >= len)
                    break;

                PropertyAttributes a = sparse->attrs ? sparse->attrs[n->value] : Attr_Data;
                d->data[i] = thisObject->getValue(reinterpret_cast<Property *>(sparse->data + n->value), a);
                d->attrs[i] = a.isAccessor() ? Attr_Data : a;

                n = n->nextNode();
                ++i;
            }
        } else {
            while (n != sparse->sparse->end()) {
                if (n->value >= len)
                    break;
                d->data[i] = sparse->data[n->value];
                n = n->nextNode();
                ++i;
            }
        }
        d->len = i;
        if (len > i)
            len = i;
        if (n != sparse->sparse->end()) {
            // have some entries outside the sort range that we need to ignore when sorting
            thisObject->initSparseArray();
            while (n != sparse->sparse->end()) {
                PropertyAttributes a = sparse->attrs ? sparse->attrs[n->value] : Attr_Data;
                thisObject->arraySet(n->value, *reinterpret_cast<Property *>(sparse->data + n->value), a);

                n = n->nextNode();
            }

        }
        // ### explicitly delete sparse
    } else {
        SimpleArrayData *d = static_cast<SimpleArrayData *>(thisObject->arrayData);
        if (len > d->len)
            len = d->len;

        // sort empty values to the end
        for (uint i = 0; i < len; i++) {
            if (thisObject->arrayData->data[i].isEmpty()) {
                while (--len > i)
                    if (!thisObject->arrayData->data[len].isEmpty())
                        break;
                Q_ASSERT(!thisObject->arrayData->attrs || !thisObject->arrayData->attrs[len].isAccessor());
                thisObject->arrayData->data[i] = thisObject->arrayData->data[len];
                thisObject->arrayData->data[len] = Primitive::emptyValue();
            }
        }

        if (!len)
            return;
    }


    ArrayElementLessThan lessThan(context, thisObject, comparefn);

    SafeValue *begin = thisObject->arrayData->data;
    std::sort(begin, begin + len, lessThan);

#ifdef CHECK_SPARSE_ARRAYS
    thisObject->initSparseArray();
#endif

}
