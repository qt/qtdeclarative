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
#include "qv4arraydata_p.h"
#include "qv4memberdata_p.h"

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
    V4_OBJECT
    Q_MANAGED_TYPE(Object)
    enum {
        IsObject = true
    };
    Members memberData;

    ArrayData *arrayData;

    Property *propertyAt(uint index) const { return reinterpret_cast<Property *>(memberData.data() + index); }

    Object(ExecutionEngine *engine);
    Object(InternalClass *internalClass);
    ~Object();

    const ObjectVTable *vtable() const { return reinterpret_cast<const ObjectVTable *>(internalClass->vtable); }
    Object *prototype() const { return internalClass->prototype; }
    bool setPrototype(Object *proto);

    Property *__getOwnProperty__(const StringRef name, PropertyAttributes *attrs = 0);
    Property *__getOwnProperty__(uint index, PropertyAttributes *attrs = 0);

    Property *__getPropertyDescriptor__(const StringRef name, PropertyAttributes *attrs = 0) const;
    Property *__getPropertyDescriptor__(uint index, PropertyAttributes *attrs = 0) const;

    bool hasProperty(const StringRef name) const;
    bool hasProperty(uint index) const;

    bool hasOwnProperty(const StringRef name) const;
    bool hasOwnProperty(uint index) const;

    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, const StringRef member, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, const StringRef name, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, const Property &p, PropertyAttributes attrs);
    bool defineOwnProperty2(ExecutionContext *ctx, uint index, const Property &p, PropertyAttributes attrs);

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
    void defineDefaultProperty(const StringRef name, ValueRef value) {
        insertMember(name, value, Attr_Data|Attr_NotEnumerable);
    }
    void defineDefaultProperty(const QString &name, ValueRef value);
    void defineDefaultProperty(const QString &name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineDefaultProperty(const StringRef name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineAccessorProperty(const QString &name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    void defineAccessorProperty(const StringRef name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(const QString &name, ValueRef value);
    void defineReadonlyProperty(const StringRef name, ValueRef value);

    void insertMember(const StringRef s, const ValueRef v, PropertyAttributes attributes = Attr_Data) {
        insertMember(s, Property(*v), attributes);
    }
    void insertMember(const StringRef s, const Property &p, PropertyAttributes attributes);

    inline ExecutionEngine *engine() const { return internalClass->engine; }

    // Array handling

public:
    void copyArrayData(Object *other);

    bool setArrayLength(uint newLen);
    void setArrayLengthUnchecked(uint l);

    void arraySet(uint index, const Property &p, PropertyAttributes attributes = Attr_Data);
    void arraySet(uint index, ValueRef value);

    bool arrayPut(uint index, ValueRef value) {
        return arrayData->vtable()->put(this, index, value);
    }
    bool arrayPut(uint index, Value *values, uint n) {
        return arrayData->vtable()->putArray(this, index, values, n);
    }
    void setArrayAttributes(uint i, PropertyAttributes a) {
        Q_ASSERT(arrayData);
        if (arrayData->attrs || a != Attr_Data) {
            ArrayData::ensureAttributes(this);
            a.resolve();
            arrayData->vtable()->setAttribute(this, i, a);
        }
    }

    void push_back(const ValueRef v);

    ArrayData::Type arrayType() const {
        return arrayData ? arrayData->type : ArrayData::Simple;
    }
    // ### remove me
    void setArrayType(ArrayData::Type t) {
        Q_ASSERT(t != ArrayData::Simple && t != ArrayData::Sparse);
        arrayCreate();
        arrayData->type = t;
    }

    inline void arrayReserve(uint n) {
        ArrayData::realloc(this, ArrayData::Simple, 0, n, false);
    }

    void arrayCreate() {
        if (!arrayData)
            ArrayData::realloc(this, ArrayData::Simple, 0, 0, false);
#ifdef CHECK_SPARSE_ARRAYS
        initSparseArray();
#endif
    }

    void initSparseArray();
    SparseArrayNode *sparseBegin() { return arrayType() == ArrayData::Sparse ? static_cast<SparseArrayData *>(arrayData)->sparse->begin() : 0; }
    SparseArrayNode *sparseEnd() { return arrayType() == ArrayData::Sparse ? static_cast<SparseArrayData *>(arrayData)->sparse->end() : 0; }

    inline bool protoHasArray() {
        Scope scope(engine());
        Scoped<Object> p(scope, this);

        while ((p = p->prototype()))
            if (p->arrayData)
                return true;

        return false;
    }
    void ensureMemberIndex(uint idx);

    inline ReturnedValue get(const StringRef name, bool *hasProperty = 0)
    { return vtable()->get(this, name, hasProperty); }
    inline ReturnedValue getIndexed(uint idx, bool *hasProperty = 0)
    { return vtable()->getIndexed(this, idx, hasProperty); }
    inline void put(const StringRef name, const ValueRef v)
    { vtable()->put(this, name, v); }
    inline void putIndexed(uint idx, const ValueRef v)
    { vtable()->putIndexed(this, idx, v); }
    PropertyAttributes query(StringRef name) const
    { return vtable()->query(this, name); }
    PropertyAttributes queryIndexed(uint index) const
    { return vtable()->queryIndexed(this, index); }
    bool deleteProperty(const StringRef name)
    { return vtable()->deleteProperty(this, name); }
    bool deleteIndexedProperty(uint index)
    { return vtable()->deleteIndexedProperty(this, index); }
    ReturnedValue getLookup(Lookup *l)
    { return vtable()->getLookup(this, l); }
    void setLookup(Lookup *l, const ValueRef v)
    { vtable()->setLookup(this, l, v); }
    void advanceIterator(ObjectIterator *it, StringRef name, uint *index, Property *p, PropertyAttributes *attributes)
    { vtable()->advanceIterator(this, it, name, index, p, attributes); }
    uint getLength() const { return vtable()->getLength(this); }

    inline ReturnedValue construct(CallData *d)
    { return vtable()->construct(this, d); }
    inline ReturnedValue call(CallData *d)
    { return vtable()->call(this, d); }
protected:
    static void destroy(Managed *that);
    static void markObjects(Managed *that, ExecutionEngine *e);
    static ReturnedValue construct(Managed *m, CallData *);
    static ReturnedValue call(Managed *m, CallData *);
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
    static void advanceIterator(Managed *m, ObjectIterator *it, StringRef name, uint *index, Property *p, PropertyAttributes *attributes);
    static uint getLength(const Managed *m);

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
    V4_OBJECT
    Q_MANAGED_TYPE(BooleanObject)
    Value value;
    BooleanObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->booleanClass) {
        value = val;
    }
protected:
    BooleanObject(InternalClass *ic)
        : Object(ic) {
        Q_ASSERT(internalClass->vtable == staticVTable());
        value = Encode(false);
    }
};

struct NumberObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(NumberObject)
    Value value;
    NumberObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->numberClass) {
        value = val;
    }
protected:
    NumberObject(InternalClass *ic)
        : Object(ic) {
        Q_ASSERT(internalClass->vtable == staticVTable());
        value = Encode((int)0);
    }
};

struct ArrayObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(ArrayObject)
    enum {
        LengthPropertyIndex = 0
    };

    ArrayObject(ExecutionEngine *engine) : Object(engine->arrayClass) { init(engine); }
    ArrayObject(ExecutionEngine *engine, const QStringList &list);
    ArrayObject(InternalClass *ic) : Object(ic) { init(ic->engine); }

    void init(ExecutionEngine *engine);

    static ReturnedValue getLookup(Managed *m, Lookup *l);
    using Object::getLength;
    static uint getLength(const Managed *m);

    QStringList toQStringList() const;
};

inline void Object::setArrayLengthUnchecked(uint l)
{
    if (isArrayObject())
        memberData[ArrayObject::LengthPropertyIndex] = Primitive::fromUInt32(l);
}

inline void Object::push_back(const ValueRef v)
{
    arrayCreate();

    uint idx = getLength();
    arrayReserve(idx + 1);
    arrayPut(idx, v);
    setArrayLengthUnchecked(idx + 1);
}

inline void Object::arraySet(uint index, const Property &p, PropertyAttributes attributes)
{
    // ### Clean up
    arrayCreate();
    if (attributes.isAccessor()) {
        hasAccessorProperty = 1;
        initSparseArray();
    } else if (index > 0x1000 && index > 2*arrayData->alloc) {
        initSparseArray();
    } else {
        arrayData->vtable()->reallocate(this, index + 1, false);
    }
    setArrayAttributes(index, attributes);
    Property *pd = ArrayData::insert(this, index, attributes.isAccessor());
    pd->value = p.value;
    if (attributes.isAccessor())
        pd->set = p.set;
    if (isArrayObject() && index >= getLength())
        setArrayLengthUnchecked(index + 1);
}


inline void Object::arraySet(uint index, ValueRef value)
{
    arrayCreate();
    if (index > 0x1000 && index > 2*arrayData->alloc) {
        initSparseArray();
    }
    Property *pd = ArrayData::insert(this, index);
    pd->value = value ? *value : Primitive::undefinedValue();
    if (isArrayObject() && index >= getLength())
        setArrayLengthUnchecked(index + 1);
}

template<>
inline Object *value_cast(const Value &v) {
    return v.asObject();
}

template<>
inline ArrayObject *value_cast(const Value &v) {
    return v.asArrayObject();
}

#ifndef V4_BOOTSTRAP
template<>
inline ReturnedValue value_convert<Object>(ExecutionEngine *e, const Value &v)
{
    return v.toObject(e->currentContext())->asReturnedValue();
}
#endif

struct ObjectRef : public ManagedRef
{
    DEFINE_REF_METHODS(Object, Managed)

    static ObjectRef fromValuePointer(Value *s) {
        ObjectRef r;
        r.ptr = s;
        if (sizeof(void *) == 8)
            r.ptr->val = 0;
        else
            *r.ptr = Value::fromManaged(0);
        return r;
    }
};

DEFINE_REF(ArrayObject, Object);

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
