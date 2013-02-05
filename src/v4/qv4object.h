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
#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qv4global.h"
#include "qmljs_runtime.h"
#include "qmljs_engine.h"
#include "qmljs_environment.h"
#include "qv4sparsearray.h"
#include "qv4string.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed.h"
#include "qv4propertydescriptor.h"
#include "qv4propertytable.h"
#include "qv4objectiterator.h"
#include "qv4regexp.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct Value;
struct Function;
struct Object;
struct ObjectIterator;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
struct RegExpObject;
struct ErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct RegExpPrototype;
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;


struct Q_V4_EXPORT Object: Managed {
    Object *prototype;
    QScopedPointer<PropertyTable> members;
    uint memberDataSize;
    uint memberDataAlloc;
    PropertyDescriptor *memberData;

    union {
        uint arrayFreeList;
        uint arrayOffset;
    };
    uint arrayDataLen;
    uint arrayAlloc;
    PropertyDescriptor *arrayData;
    SparseArray *sparseArray;

    Object()
        : prototype(0)
        , memberDataSize(0), memberDataAlloc(0), memberData(0)
        , arrayOffset(0), arrayDataLen(0), arrayAlloc(0), sparseArray(0) { type = Type_Object; }

    virtual ~Object();

    PropertyDescriptor *__getOwnProperty__(ExecutionContext *ctx, String *name);
    PropertyDescriptor *__getOwnProperty__(ExecutionContext *ctx, uint index);
    PropertyDescriptor *__getPropertyDescriptor__(ExecutionContext *ctx, String *name);
    PropertyDescriptor *__getPropertyDescriptor__(ExecutionContext *ctx, uint index);

    Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty = 0);
    Value __get__(ExecutionContext *ctx, uint index, bool *hasProperty = 0);

    void __put__(ExecutionContext *ctx, String *name, Value value);
    void __put__(ExecutionContext *ctx, uint index, Value value);

    bool __hasProperty__(const ExecutionContext *ctx, String *name) const;
    bool __hasProperty__(const ExecutionContext *ctx, uint index) const;
    bool __delete__(ExecutionContext *ctx, String *name);
    bool __delete__(ExecutionContext *ctx, uint index);
    bool __defineOwnProperty__(ExecutionContext *ctx, PropertyDescriptor *current, const PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, String *name, const PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, const PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, const PropertyDescriptor *desc);

    //
    // helpers
    //
    void __put__(ExecutionContext *ctx, const QString &name, const Value &value);

    Value getValue(ExecutionContext *ctx, const PropertyDescriptor *p) const;
    Value getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p) const;
    Value getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p, bool *exists) const;

    void inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx);
    void inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *), int count = 0);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *, Value, Value *, int), int argumentCount = 0);
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value);
    void defineReadonlyProperty(String *name, Value value);

    PropertyDescriptor *insertMember(String *s);

    // Array handling

    static void fillDescriptor(PropertyDescriptor *pd, Value v)
    {
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->value = v;
    }

    uint allocArrayValue() {
        uint idx = arrayFreeList;
        if (arrayAlloc <= arrayFreeList)
            arrayReserve(arrayAlloc + 1);
        arrayFreeList = arrayData[arrayFreeList].value.integerValue();
        return idx;
    }

    uint allocArrayValue(Value v) {
        uint idx = allocArrayValue();
        PropertyDescriptor *pd = &arrayData[idx];
        fillDescriptor(pd, v);
        return idx;
    }
    void freeArrayValue(int idx) {
        PropertyDescriptor &pd = arrayData[idx];
        pd.type = PropertyDescriptor::Generic;
        pd.value.tag = Value::_Undefined_Type;
        pd.value.int_32 = arrayFreeList;
        arrayFreeList = idx;
    }

    PropertyDescriptor *arrayDecriptor(uint index) {
        return arrayData + index;
    }
    const PropertyDescriptor *arrayDecriptor(uint index) const {
        return arrayData + index;
    }

    void getArrayHeadRoom() {
        assert(!sparseArray && !arrayOffset);
        arrayOffset = qMax(arrayDataLen >> 2, (uint)16);
        PropertyDescriptor *newArray = new PropertyDescriptor[arrayOffset + arrayAlloc];
        memcpy(newArray + arrayOffset, arrayData, arrayDataLen*sizeof(PropertyDescriptor));
        delete [] arrayData;
        arrayData = newArray + arrayOffset;
    }

public:
    void copyArrayData(Object *other);
    void initSparse();

    uint arrayLength() const;
    bool setArrayLength(uint newLen);

    void setArrayLengthUnchecked(uint l);

    PropertyDescriptor *arrayInsert(uint index) {
        PropertyDescriptor *pd;
        if (!sparseArray && (index < 0x1000 || index < arrayDataLen + (arrayDataLen >> 2))) {
            if (index >= arrayAlloc)
                arrayReserve(index + 1);
            if (index >= arrayDataLen) {
                for (uint i = arrayDataLen; i < index; ++i) {
                    arrayData[i].type = PropertyDescriptor::Generic;
                    arrayData[i].writable = PropertyDescriptor::Undefined;
                    arrayData[i].value.tag = Value::_Undefined_Type;
                }
                arrayDataLen = index + 1;
            }
            pd = arrayDecriptor(index);
        } else {
            initSparse();
            SparseArrayNode *n = sparseArray->insert(index);
            if (n->value == UINT_MAX)
                n->value = allocArrayValue();
            pd = arrayDecriptor(n->value);
        }
        if (index >= arrayLength())
            setArrayLengthUnchecked(index + 1);
        return pd;
    }

    void arraySet(uint index, const PropertyDescriptor *pd) {
        *arrayInsert(index) = *pd;
    }

    void arraySet(uint index, Value value) {
        PropertyDescriptor *pd = arrayInsert(index);
        fillDescriptor(pd, value);
    }

    PropertyDescriptor *arrayAt(uint index) const {
        if (!sparseArray) {
            if (index >= arrayDataLen)
                return 0;
            return arrayData + index;
        } else {
            SparseArrayNode *n = sparseArray->findNode(index);
            if (!n)
                return 0;
            return arrayData + n->value;
        }
    }

    PropertyDescriptor *nonSparseArrayAt(uint index) const {
        if (sparseArray)
            return 0;
        if (index >= arrayDataLen)
            return 0;
        return arrayData + index;
    }

    void markArrayObjects() const;

    void push_back(Value v) {
        uint idx = arrayLength();
        if (!sparseArray) {
            if (idx >= arrayAlloc)
                arrayReserve(idx + 1);
            fillDescriptor(arrayData + idx, v);
            arrayDataLen = idx + 1;
        } else {
            uint idx = allocArrayValue(v);
            sparseArray->push_back(idx, arrayLength());
        }
        setArrayLengthUnchecked(idx + 1);
    }

    SparseArrayNode *sparseArrayBegin() { return sparseArray ? sparseArray->begin() : 0; }
    SparseArrayNode *sparseArrayEnd() { return sparseArray ? sparseArray->end() : 0; }

    void arrayConcat(const ArrayObject *other);
    void arraySort(ExecutionContext *context, Object *thisObject, const Value &comparefn, uint arrayDataLen);
    Value arrayIndexOf(Value v, uint fromIndex, uint arrayDataLen, ExecutionContext *ctx, Object *o);

    void arrayReserve(uint n);

protected:
    virtual void markObjects();

    friend struct ObjectIterator;
    friend struct ObjectPrototype;
};

struct ForEachIteratorObject: Object {
    ObjectIterator it;
    ForEachIteratorObject(ExecutionContext *ctx, Object *o)
        : it(ctx, o, ObjectIterator::EnumberableOnly|ObjectIterator::WithProtoChain) { type = Type_ForeachIteratorObject; }

    Value nextPropertyName() { return it.nextPropertyNameAsString(); }

protected:
    virtual void markObjects();
};

struct BooleanObject: Object {
    Value value;
    BooleanObject(const Value &value): value(value) { type = Type_BooleanObject; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(const Value &value): value(value) { type = Type_NumberObject; }
};

struct ArrayObject: Object {
    enum {
        LengthPropertyIndex = 0
    };

    ArrayObject(ExecutionContext *ctx) { init(ctx); }
    void init(ExecutionContext *context);
};

inline uint Object::arrayLength() const
{
    if (isArrayObject()) {
        // length is always the first property of an array
        Value v = memberData[ArrayObject::LengthPropertyIndex].value;
        if (v.isInteger())
            return v.integerValue();
        return Value::toUInt32(v.doubleValue());
    }
    return 0;
}

inline void Object::setArrayLengthUnchecked(uint l)
{
    if (isArrayObject()) {
        // length is always the first property of an array
        PropertyDescriptor &lengthProperty = memberData[ArrayObject::LengthPropertyIndex];
        lengthProperty.value = Value::fromUInt32(l);
    }
}


} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
