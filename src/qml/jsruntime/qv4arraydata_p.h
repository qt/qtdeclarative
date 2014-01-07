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

struct ArrayData;

struct ArrayVTable
{
    uint type;
    void (*freeData)(ArrayData *d);
    void (*reserve)(ArrayData *d, uint n);
    ReturnedValue (*get)(const ArrayData *d, uint index);
    bool (*put)(ArrayData *d, uint index, ValueRef value);
    bool (*putArray)(ArrayData *d, uint index, SafeValue *values, uint n);
    bool (*del)(ArrayData *d, uint index);
    void (*setAttribute)(ArrayData *d, uint index, PropertyAttributes attrs);
    PropertyAttributes (*attribute)(const ArrayData *d, uint index);
    void (*push_front)(ArrayData *d, SafeValue *values, uint n);
    ReturnedValue (*pop_front)(ArrayData *d);
    uint (*truncate)(ArrayData *d, uint newLen);
};


struct Q_QML_EXPORT ArrayData
{
    ArrayData()
        : vtable(&static_vtbl)
        , offset(0)
        , len(0)
        , alloc(0)
        , type(0)
        , attrs(0)
        , data(0)
    {
    }

    enum Type {
        Simple = 0,
        Complex = 1,
        Sparse = 2,
        Custom = 3
    };

    const ArrayVTable *vtable;
    uint offset;
    uint len;
    uint alloc;
    uint type;
    PropertyAttributes *attrs;
    Property *data;

    bool isSparse() const { return this && type == Sparse; }

    uint length() const {
        return this ? len : 0;
    }
    void setLength(uint l) {
        Q_ASSERT(this);
        len = l;
    }

    bool hasAttributes() const {
        return this && attrs;
    }
    void ensureAttributes();
    PropertyAttributes attributes(int i) const {
        Q_ASSERT(this);
        return attrs ? vtable->attribute(this, i) : Attr_Data;
    }
    void setAttributes(uint i, PropertyAttributes a) {
        Q_ASSERT(this);
        if (attrs || a != Attr_Data) {
            ensureAttributes();
            a.resolve();
            vtable->setAttribute(this, i, a);
        }
    }

    bool isEmpty(uint i) const {
        if (!this)
            return true;
        return (vtable->get(this, i) == Primitive::emptyValue().asReturnedValue());
    }


    inline void free() {
        vtable->freeData(this);
    }

    inline void push_front(SafeValue *values, uint nValues) {
        vtable->push_front(this, values, nValues);
    }
    inline ReturnedValue pop_front() {
        return vtable->pop_front(this);
    }
    inline uint push_back(uint l, uint n, SafeValue *values) {
        vtable->putArray(this, l, values, n);
        return len;
    }
    inline bool deleteIndex(uint index) {
        return vtable->del(this, index);
    }
    inline uint truncate(uint newLen) {
        if (!this || len < newLen)
            return newLen;
        return vtable->truncate(this, newLen);
    }
    bool put(uint index, ValueRef value) {
        return vtable->put(this, index, value);
    }
    bool put(uint index, SafeValue *values, uint n) {
        return vtable->putArray(this, index, values, n);
    }
    ReturnedValue get(uint i) const {
        if (!this)
            return Primitive::emptyValue().asReturnedValue();
        return vtable->get(this, i);
    }
    inline Property *getProperty(uint index) const;



    static void sort(ExecutionContext *context, ObjectRef thisObject, const ValueRef comparefn, uint dataLen);
    static uint append(Object *o, const ArrayObject *otherObj, uint n);
    static Property *insert(Object *o, uint index);
    void markObjects(ExecutionEngine *e);

    static void getHeadRoom(ArrayData *d);
    static void reserve(ArrayData *d, uint n);

    static void freeData(ArrayData *d);
    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(ArrayData *d, uint index, ValueRef value);
    static bool putArray(ArrayData *d, uint index, SafeValue *values, uint n);
    static bool del(ArrayData *d, uint index);
    static void setAttribute(ArrayData *d, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(ArrayData *d, SafeValue *values, uint n);
    static ReturnedValue pop_front(ArrayData *d);
    static uint truncate(ArrayData *d, uint newLen);

    static const ArrayVTable static_vtbl;
};

struct Q_QML_EXPORT SparseArrayData : public ArrayData
{
    SparseArrayData()
        : freeList(0)
        , sparse(0)
    { vtable = &static_vtbl; }

    uint freeList;
    SparseArray *sparse;

    static uint allocate(ArrayData *d);
    static void free(ArrayData *d, uint idx);

    static void freeData(ArrayData *d);
    static void reserve(ArrayData *d, uint n);
    static ReturnedValue get(const ArrayData *d, uint index);
    static bool put(ArrayData *d, uint index, ValueRef value);
    static bool putArray(ArrayData *d, uint index, SafeValue *values, uint n);
    static bool del(ArrayData *d, uint index);
    static void setAttribute(ArrayData *d, uint index, PropertyAttributes attrs);
    static PropertyAttributes attribute(const ArrayData *d, uint index);
    static void push_front(ArrayData *d, SafeValue *values, uint n);
    static ReturnedValue pop_front(ArrayData *d);
    static uint truncate(ArrayData *d, uint newLen);

    static const ArrayVTable static_vtbl;
};


inline Property *ArrayData::getProperty(uint index) const
{
    if (!this)
        return 0;
    if (type != Sparse) {
        if (index >= len || data[index].value.isEmpty())
            return 0;
        return data + index;
    } else {
        SparseArrayNode *n = static_cast<const SparseArrayData *>(this)->sparse->findNode(index);
        if (!n)
            return 0;
        return data + n->value;
    }
}

}

QT_END_NAMESPACE

#endif
