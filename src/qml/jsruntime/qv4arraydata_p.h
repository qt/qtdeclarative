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
#ifndef QV4ARRAYDATA_H
#define QV4ARRAYDATA_H

#include "qv4global_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4sparsearray_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

#define V4_ARRAYDATA \
    public: \
        Q_MANAGED_CHECK \
        static const QV4::ArrayVTable static_vtbl; \
        static inline const QV4::ManagedVTable *staticVTable() { return &static_vtbl.managedVTable; } \
        template <typename T> \
        QV4::Returned<T> *asReturned() { return QV4::Returned<T>::create(this); } \

struct ArrayData;

struct ArrayVTable
{
    ManagedVTable managedVTable;
    uint type;
    ArrayData *(*reallocate)(Object *o, uint n, bool enforceAttributes);
    ReturnedValue (*get)(const ArrayData *d, uint index);
    bool (*put)(Object *o, uint index, ValueRef value);
    bool (*putArray)(Object *o, uint index, Value *values, uint n);
    bool (*del)(Object *o, uint index);
    void (*setAttribute)(Object *o, uint index, PropertyAttributes attrs);
    PropertyAttributes (*attribute)(const ArrayData *d, uint index);
    void (*push_front)(Object *o, Value *values, uint n);
    ReturnedValue (*pop_front)(Object *o);
    uint (*truncate)(Object *o, uint newLen);
    uint (*length)(const ArrayData *d);
};


struct Q_QML_EXPORT ArrayData : public Managed
{
    ArrayData(InternalClass *ic)
        : Managed(ic)
    {
    }

    enum Type {
        Simple = 0,
        Complex = 1,
        Sparse = 2,
        Custom = 3
    };

    uint alloc;
    Type type;
    PropertyAttributes *attrs;
    Value *data;

    const ArrayVTable *vtable() const { return reinterpret_cast<const ArrayVTable *>(internalClass->vtable); }
    bool isSparse() const { return this && type == Sparse; }

    uint length() const {
        if (!this)
            return 0;
        return vtable()->length(this);
    }

    bool hasAttributes() const {
        return this && attrs;
    }
    PropertyAttributes attributes(int i) const {
        Q_ASSERT(this);
        return attrs ? vtable()->attribute(this, i) : Attr_Data;
    }

    bool isEmpty(uint i) const {
        if (!this)
            return true;
        return (vtable()->get(this, i) == Primitive::emptyValue().asReturnedValue());
    }

    ReturnedValue get(uint i) const {
        if (!this)
            return Primitive::emptyValue().asReturnedValue();
        return vtable()->get(this, i);
    }
    inline Property *getProperty(uint index) const;

    static void ensureAttributes(Object *o);
    static void realloc(Object *o, Type newType, uint offset, uint alloc, bool enforceAttributes);

    static void sort(ExecutionContext *context, ObjectRef thisObject, const ValueRef comparefn, uint dataLen);
    static uint append(Object *obj, const ArrayObject *otherObj, uint n);
    static Property *insert(Object *o, uint index, bool isAccessor = false);
};

struct Q_QML_EXPORT SimpleArrayData : public ArrayData
{
    V4_ARRAYDATA

    SimpleArrayData(ExecutionEngine *engine)
        : ArrayData(engine->simpleArrayDataClass)
    {}

    uint len;
    uint offset;

    static void getHeadRoom(Object *o);
    static ArrayData *reallocate(Object *o, uint n, bool enforceAttributes);

    static void destroy(Managed *d);
    static void markObjects(Managed *d, ExecutionEngine *e);

    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(Object *o, uint index, ValueRef value);
    static bool putArray(Object *o, uint index, Value *values, uint n);
    static bool del(Object *o, uint index);
    static void setAttribute(Object *o, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(Object *o, Value *values, uint n);
    static ReturnedValue pop_front(Object *o);
    static uint truncate(Object *o, uint newLen);
    static uint length(const ArrayData *d);
};

struct Q_QML_EXPORT SparseArrayData : public ArrayData
{
    V4_ARRAYDATA

    SparseArrayData(ExecutionEngine *engine)
        : ArrayData(engine->emptyClass)
    { setVTable(staticVTable()); }

    uint freeList;
    SparseArray *sparse;

    static uint allocate(Object *o, bool doubleSlot = false);
    static void free(ArrayData *d, uint idx);

    static void destroy(Managed *d);
    static void markObjects(Managed *d, ExecutionEngine *e);

    static ArrayData *reallocate(Object *o, uint n, bool enforceAttributes);
    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(Object *o, uint index, ValueRef value);
    static bool putArray(Object *o, uint index, Value *values, uint n);
    static bool del(Object *o, uint index);
    static void setAttribute(Object *o, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(Object *o, Value *values, uint n);
    static ReturnedValue pop_front(Object *o);
    static uint truncate(Object *o, uint newLen);
    static uint length(const ArrayData *d);
};


inline Property *ArrayData::getProperty(uint index) const
{
    if (!this)
        return 0;
    if (type != Sparse) {
        const SimpleArrayData *that = static_cast<const SimpleArrayData *>(this);
        if (index >= that->len || data[index].isEmpty())
            return 0;
        return reinterpret_cast<Property *>(data + index);
    } else {
        SparseArrayNode *n = static_cast<const SparseArrayData *>(this)->sparse->findNode(index);
        if (!n)
            return 0;
        return reinterpret_cast<Property *>(data + n->value);
    }
}

}

QT_END_NAMESPACE

#endif
