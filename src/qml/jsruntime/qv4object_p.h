/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"
#include "qv4string_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4internalclass_p.h"
#include "qv4sparsearray_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

#ifdef _WIN32_WCE
#undef assert
#define assert(x)
#endif // _WIN32_WCE

QT_BEGIN_NAMESPACE

namespace QV4 {

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

struct Q_QML_EXPORT Object: Managed {
    Q_MANAGED
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

    enum {
        InlinePropertySize = 4
    };
    Property inlineProperties[InlinePropertySize];

    Object(ExecutionEngine *engine);
    Object(InternalClass *internalClass);
    ~Object();

    Object *prototype() const { return internalClass->prototype; }
    bool setPrototype(Object *proto);

    Property *__getOwnProperty__(const StringRef name, PropertyAttributes *attrs = 0);
    Property *__getOwnProperty__(uint index, PropertyAttributes *attrs = 0);

    Property *__getPropertyDescriptor__(const StringRef name, PropertyAttributes *attrs = 0) const;
    Property *__getPropertyDescriptor__(uint index, PropertyAttributes *attrs = 0) const;

    bool __hasProperty__(const StringRef name) const;
    bool __hasProperty__(uint index) const;

    bool __defineOwnProperty__(ExecutionContext *ctx, Property *current, const StringRef member, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, const StringRef name, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, const Property &p, PropertyAttributes attrs);

    //
    // helpers
    //
    void put(ExecutionContext *ctx, const QString &name, const ValueRef value);

    static ReturnedValue getValue(const ValueRef thisObject, const Property *p, PropertyAttributes attrs);
    ReturnedValue getValue(const Property *p, PropertyAttributes attrs) const {
        Scope scope(this->engine());
        ScopedValue t(scope, const_cast<Object *>(this));
        return getValue(t, p, attrs);
    }

    void putValue(Property *pd, PropertyAttributes attrs, const ValueRef value);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(const StringRef name, ValueRef value);
    void defineDefaultProperty(const QString &name, ValueRef value);
    void defineDefaultProperty(const QString &name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineDefaultProperty(const StringRef name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineAccessorProperty(const QString &name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    void defineAccessorProperty(const StringRef name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(const QString &name, ValueRef value);
    void defineReadonlyProperty(const StringRef name, ValueRef value);

    Property *insertMember(const StringRef s, PropertyAttributes attributes);

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

    uint allocArrayValue(const ValueRef v) {
        uint idx = allocArrayValue();
        Property *pd = &arrayData[idx];
        pd->value = *v;
        return idx;
    }
    void freeArrayValue(int idx) {
        Property &pd = arrayData[idx];
        pd.value.tag = Value::Empty_Type;
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

    Property *arrayInsert(uint index, PropertyAttributes attributes = Attr_Data);

    void arraySet(uint index, const Property *pd);
    void arraySet(uint index, ValueRef value);

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

    void push_back(const ValueRef v);

    SparseArrayNode *sparseArrayBegin() { return sparseArray ? sparseArray->begin() : 0; }
    SparseArrayNode *sparseArrayEnd() { return sparseArray ? sparseArray->end() : 0; }

    void arrayConcat(const ArrayObject *other);
    void arraySort(ExecutionContext *context, ObjectRef thisObject, const ValueRef comparefn, uint arrayDataLen);
    ReturnedValue arrayIndexOf(const ValueRef v, uint fromIndex, uint arrayDataLen, ExecutionContext *ctx, Object *o);

    void arrayReserve(uint n);
    void ensureArrayAttributes();

    inline bool protoHasArray() {
        Scope scope(engine());
        Scoped<Object> p(scope, this);

        while ((p = p->prototype()))
            if (p->arrayDataLen)
                return true;

        return false;
    }
    void ensureMemberIndex(uint idx);

    inline ReturnedValue get(const StringRef name, bool *hasProperty = 0)
    { return internalClass->vtable->get(this, name, hasProperty); }
    inline ReturnedValue getIndexed(uint idx, bool *hasProperty = 0)
    { return internalClass->vtable->getIndexed(this, idx, hasProperty); }
    inline void put(const StringRef name, const ValueRef v)
    { internalClass->vtable->put(this, name, v); }
    inline void putIndexed(uint idx, const ValueRef v)
    { internalClass->vtable->putIndexed(this, idx, v); }
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
    static void destroy(Managed *that);
    static void markObjects(Managed *that, ExecutionEngine *e);
    static ReturnedValue get(Managed *m, const StringRef name, bool *hasProperty);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void put(Managed *m, const StringRef name, const ValueRef value);
    static void putIndexed(Managed *m, uint index, const ValueRef value);
    static PropertyAttributes query(const Managed *m, StringRef name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *m, const StringRef name);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static ReturnedValue getLookup(Managed *m, Lookup *l);
    static void setLookup(Managed *m, Lookup *l, const ValueRef v);
    static Property *advanceIterator(Managed *m, ObjectIterator *it, StringRef name, uint *index, PropertyAttributes *attributes);


private:
    ReturnedValue internalGet(const StringRef name, bool *hasProperty);
    ReturnedValue internalGetIndexed(uint index, bool *hasProperty);
    void internalPut(const StringRef name, const ValueRef value);
    void internalPutIndexed(uint index, const ValueRef value);
    bool internalDeleteProperty(const StringRef name);
    bool internalDeleteIndexedProperty(uint index);

    friend struct ObjectIterator;
    friend struct ObjectPrototype;
};

struct BooleanObject: Object {
    Q_MANAGED
    SafeValue value;
    BooleanObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->booleanClass) {
        type = Type_BooleanObject;
        value = val;
    }
protected:
    BooleanObject(InternalClass *ic)
        : Object(ic) {
        setVTable(&static_vtbl);
        type = Type_BooleanObject;
        value = Encode(false);
    }
};

struct NumberObject: Object {
    Q_MANAGED
    SafeValue value;
    NumberObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->numberClass) {
        type = Type_NumberObject;
        value = val;
    }
protected:
    NumberObject(InternalClass *ic)
        : Object(ic) {
        setVTable(&static_vtbl);
        type = Type_NumberObject;
        value = Encode((int)0);
    }
};

struct ArrayObject: Object {
    Q_MANAGED
    enum {
        LengthPropertyIndex = 0
    };

    ArrayObject(ExecutionEngine *engine) : Object(engine->arrayClass) { init(engine); }
    ArrayObject(ExecutionEngine *engine, const QStringList &list);
    ArrayObject(InternalClass *ic) : Object(ic) { init(ic->engine); }

    void init(ExecutionEngine *engine);

    QStringList toQStringList() const;
};

inline uint Object::arrayLength() const
{
    if (isArrayObject()) {
        if (memberData[ArrayObject::LengthPropertyIndex].value.isInteger())
            return memberData[ArrayObject::LengthPropertyIndex].value.integerValue();
        return Primitive::toUInt32(memberData[ArrayObject::LengthPropertyIndex].value.doubleValue());
    }
    return 0;
}

inline void Object::setArrayLengthUnchecked(uint l)
{
    if (isArrayObject()) {
        // length is always the first property of an array
        Property &lengthProperty = memberData[ArrayObject::LengthPropertyIndex];
        lengthProperty.value = Primitive::fromUInt32(l);
    }
}

inline void Object::push_back(const ValueRef v)
{
    uint idx = arrayLength();
    if (!sparseArray) {
        if (idx >= arrayAlloc)
            arrayReserve(idx + 1);
        arrayData[idx].value = *v;
        arrayDataLen = idx + 1;
    } else {
        uint idx = allocArrayValue(v);
        sparseArray->push_back(idx, arrayLength());
    }
    setArrayLengthUnchecked(idx + 1);
}

inline Property *Object::arrayInsert(uint index, PropertyAttributes attributes) {
    if (attributes.isAccessor())
        hasAccessorProperty = 1;

    Property *pd;
    if (!sparseArray && (index < 0x1000 || index < arrayDataLen + (arrayDataLen >> 2))) {
        if (index >= arrayAlloc)
            arrayReserve(index + 1);
        if (index >= arrayDataLen) {
            // mark possible hole in the array
            for (uint i = arrayDataLen; i < index; ++i) {
                arrayData[i].value = Primitive::emptyValue();
                if (arrayAttributes)
                    arrayAttributes[i].clear();
            }
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

inline void Object::arraySet(uint index, ValueRef value)
{
    Property *pd = arrayInsert(index);
    pd->value = *value;
}

inline void Object::arraySet(uint index, const Property *pd)
{
    *arrayInsert(index) = *pd;
}

template<>
inline Object *value_cast(const Value &v) {
    return v.asObject();
}

template<>
inline ArrayObject *value_cast(const Value &v) {
    return v.asArrayObject();
}

template<>
inline ReturnedValue value_convert<Object>(ExecutionContext *ctx, const Value &v)
{
    return v.toObject(ctx)->asReturnedValue();
}

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
