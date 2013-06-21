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

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"
#include "qv4sparsearray_p.h"
#include "qv4string_p.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4internalclass_p.h"
#include "qv4objectiterator_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Value;
struct Function;
struct Lookup;
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
struct CallContext;
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

typedef Value (*PropertyEnumeratorFunction)(Object *object);
typedef PropertyAttributes (*PropertyQueryFunction)(const Object *object, String *name);

struct Q_QML_EXPORT Object: Managed {

    class ExternalResource {
    public:
        virtual ~ExternalResource() {}
    };

    Object *prototype;
    uint memberDataAlloc;
    Property *memberData;

    union {
        uint arrayFreeList;
        uint arrayOffset;
    };
    uint arrayDataLen;
    uint arrayAlloc;
    PropertyAttributes *arrayAttributes;
    Property *arrayData;
    SparseArray *sparseArray;
    ExternalResource *externalResource;

    enum {
        InlinePropertySize = 4
    };
    Property inlineProperties[InlinePropertySize];

    Object(ExecutionEngine *engine);
    Object(ExecutionContext *context);
    Object(ExecutionEngine *engine, InternalClass *internalClass);
    ~Object();

    Property *__getOwnProperty__(String *name, PropertyAttributes *attrs = 0);
    Property *__getOwnProperty__(uint index, PropertyAttributes *attrs = 0);

    Property *__getPropertyDescriptor__(String *name, PropertyAttributes *attrs = 0) const;
    Property *__getPropertyDescriptor__(uint index, PropertyAttributes *attrs = 0) const;

    bool __hasProperty__(String *name) const;
    bool __hasProperty__(uint index) const {
        return __getPropertyDescriptor__(index);
    }

    bool __defineOwnProperty__(ExecutionContext *ctx, Property *current, String *member, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, String *name, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, const Property &p, PropertyAttributes attrs);

    //
    // helpers
    //
    void put(ExecutionContext *ctx, const QString &name, const Value &value);

    static Value getValue(const Value &thisObject, ExecutionContext *ctx, const Property *p, PropertyAttributes attrs);
    Value getValue(ExecutionContext *ctx, const Property *p, PropertyAttributes attrs) const {
        return getValue(Value::fromObject(const_cast<Object *>(this)), ctx, p, attrs);
    }

    void putValue(ExecutionContext *ctx, Property *pd, PropertyAttributes attrs, const Value &value);

    void inplaceBinOp(ExecutionContext *ctx, BinOp op, String *name, const Value &rhs);
    void inplaceBinOp(ExecutionContext *ctx, BinOp op, const Value &index, const Value &rhs);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value value);
    void defineDefaultProperty(ExecutionEngine *engine, const QString &name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(SimpleCallContext *), int count = 0);
    void defineDefaultProperty(ExecutionEngine *engine, const QString &name, Value (*code)(SimpleCallContext *), int count = 0);
    void defineAccessorProperty(ExecutionEngine *engine, const QString &name, Value (*getter)(SimpleCallContext *), Value (*setter)(SimpleCallContext *));
    void defineAccessorProperty(String *name, Value (*getter)(SimpleCallContext *), Value (*setter)(SimpleCallContext *));
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value);
    void defineReadonlyProperty(String *name, Value value);

    Property *insertMember(String *s, PropertyAttributes attributes);

    inline ExecutionEngine *engine() const { return internalClass->engine; }

    // Array handling

    uint allocArrayValue() {
        uint idx = arrayFreeList;
        if (arrayAlloc <= arrayFreeList)
            arrayReserve(arrayAlloc + 1);
        arrayFreeList = arrayData[arrayFreeList].value.uint_32;
        if (arrayAttributes)
            arrayAttributes[idx].setType(PropertyAttributes::Data);
        return idx;
    }

    uint allocArrayValue(Value v) {
        uint idx = allocArrayValue();
        Property *pd = &arrayData[idx];
        pd->value = v;
        return idx;
    }
    void freeArrayValue(int idx) {
        Property &pd = arrayData[idx];
        pd.value.tag = Value::_Empty_Type;
        pd.value.int_32 = arrayFreeList;
        arrayFreeList = idx;
        if (arrayAttributes)
            arrayAttributes[idx].clear();
    }

    void getArrayHeadRoom() {
        assert(!sparseArray && !arrayOffset);
        arrayOffset = qMax(arrayDataLen >> 2, (uint)16);
        Property *newArray = new Property[arrayOffset + arrayAlloc];
        memcpy(newArray + arrayOffset, arrayData, arrayDataLen*sizeof(Property));
        delete [] arrayData;
        arrayData = newArray + arrayOffset;
        if (arrayAttributes) {
            PropertyAttributes *newAttrs = new PropertyAttributes[arrayOffset + arrayAlloc];
            memcpy(newAttrs + arrayOffset, arrayAttributes, arrayDataLen*sizeof(PropertyAttributes));
            delete [] arrayAttributes;
            arrayAttributes = newAttrs + arrayOffset;
        }
    }

public:
    void copyArrayData(Object *other);
    void initSparse();

    uint arrayLength() const;
    bool setArrayLength(uint newLen);

    void setArrayLengthUnchecked(uint l);

    Property *arrayInsert(uint index, PropertyAttributes attributes = Attr_Data) {

        Property *pd;
        if (!sparseArray && (index < 0x1000 || index < arrayDataLen + (arrayDataLen >> 2))) {
            if (index >= arrayAlloc)
                arrayReserve(index + 1);
            if (index >= arrayDataLen) {
                ensureArrayAttributes();
                for (uint i = arrayDataLen; i < index; ++i)
                    arrayAttributes[i].clear();
                arrayDataLen = index + 1;
            }
            pd = arrayData + index;
        } else {
            initSparse();
            SparseArrayNode *n = sparseArray->insert(index);
            if (n->value == UINT_MAX)
                n->value = allocArrayValue();
            pd = arrayData + n->value;
        }
        if (index >= arrayLength())
            setArrayLengthUnchecked(index + 1);
        if (arrayAttributes || attributes != Attr_Data) {
            if (!arrayAttributes)
                ensureArrayAttributes();
            attributes.resolve();
            arrayAttributes[pd - arrayData] = attributes;
        }
        return pd;
    }

    void arraySet(uint index, const Property *pd) {
        *arrayInsert(index) = *pd;
    }

    void arraySet(uint index, Value value) {
        Property *pd = arrayInsert(index);
        pd->value = value;
    }

    uint propertyIndexFromArrayIndex(uint index) const
    {
        if (!sparseArray) {
            if (index >= arrayDataLen)
                return UINT_MAX;
            return index;
        } else {
            SparseArrayNode *n = sparseArray->findNode(index);
            if (!n)
                return UINT_MAX;
            return n->value;
        }
    }

    Property *arrayAt(uint index) const {
        uint pidx = propertyIndexFromArrayIndex(index);
        if (pidx == UINT_MAX)
            return 0;
        return arrayData + pidx;
    }

    Property *nonSparseArrayAt(uint index) const {
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
            arrayData[idx].value = v;
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
    void ensureArrayAttributes();

    inline Value get(String *name)
    { return vtbl->get(this, engine()->current, name, 0); }
    inline Value getIndexed(uint idx)
    { return vtbl->getIndexed(this, engine()->current, idx, 0); }
    inline void put(String *name, const Value &v)
    { vtbl->put(this, engine()->current, name, v); }
    inline void putIndexed(uint idx, const Value &v)
    { vtbl->putIndexed(this, engine()->current, idx, v); }
    using Managed::get;
    using Managed::getIndexed;
    using Managed::put;
    using Managed::putIndexed;
    using Managed::query;
    using Managed::queryIndexed;
    using Managed::deleteProperty;
    using Managed::deleteIndexedProperty;
    using Managed::getLookup;
    using Managed::setLookup;
    using Managed::advanceIterator;
protected:
    static const ManagedVTable static_vtbl;
    static void destroy(Managed *that);
    static void markObjects(Managed *that);
    static Value get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty);
    static Value getIndexed(Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty);
    static void put(Managed *m, ExecutionContext *ctx, String *name, const Value &value);
    static void putIndexed(Managed *m, ExecutionContext *ctx, uint index, const Value &value);
    static PropertyAttributes query(const Managed *m, String *name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *m, String *name);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static void getLookup(Managed *m, ExecutionContext *ctx, Lookup *l, Value *result);
    static void setLookup(Managed *m, ExecutionContext *ctx, Lookup *l, const Value &v);
    static Property *advanceIterator(Managed *m, ObjectIterator *it, String **name, uint *index, PropertyAttributes *attributes);


private:
    Value internalGet(ExecutionContext *ctx, String *name, bool *hasProperty);
    Value internalGetIndexed(ExecutionContext *ctx, uint index, bool *hasProperty);
    void internalPut(ExecutionContext *ctx, String *name, const Value &value);
    void internalPutIndexed(ExecutionContext *ctx, uint index, const Value &value);
    bool internalDeleteProperty(String *name);
    bool internalDeleteIndexedProperty(uint index);

    friend struct ObjectIterator;
    friend struct ObjectPrototype;
};

struct ForEachIteratorObject: Object {
    Q_MANAGED
    ObjectIterator it;
    ForEachIteratorObject(ExecutionContext *ctx, Object *o)
        : Object(ctx->engine), it(o, ObjectIterator::EnumerableOnly|ObjectIterator::WithProtoChain) {
        vtbl = &static_vtbl;
        type = Type_ForeachIteratorObject;
    }

    Value nextPropertyName() { return it.nextPropertyNameAsString(); }

protected:
    static void markObjects(Managed *that);
};

struct BooleanObject: Object {
    Value value;
    BooleanObject(ExecutionEngine *engine, const Value &value): Object(engine), value(value) { type = Type_BooleanObject; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(ExecutionEngine *engine, const Value &value): Object(engine), value(value) { type = Type_NumberObject; }
};

struct ArrayObject: Object {
    enum {
        LengthPropertyIndex = 0
    };

    ArrayObject(ExecutionEngine *engine) : Object(engine) { init(engine); }
    ArrayObject(ExecutionEngine *engine, const QStringList &list);
    void init(ExecutionEngine *engine);

    QStringList toQStringList() const;
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
        Property &lengthProperty = memberData[ArrayObject::LengthPropertyIndex];
        lengthProperty.value = Value::fromUInt32(l);
    }
}

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
