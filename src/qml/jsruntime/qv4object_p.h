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
#ifndef QV4_OBJECT_H
#define QV4_OBJECT_H

#include "qv4managed_p.h"
#include "qv4memberdata_p.h"
#include "qv4arraydata_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct Object : Base {
    Object(ExecutionEngine *engine)
        : Base(engine->objectClass),
          prototype(static_cast<Object *>(engine->objectPrototype.m))
    {
    }
    Object(InternalClass *internal, QV4::Object *prototype);

    const Property *propertyAt(uint index) const { return reinterpret_cast<const Property *>(memberData->data + index); }
    Property *propertyAt(uint index) { return reinterpret_cast<Property *>(memberData->data + index); }

    Heap::Object *prototype;
    MemberData *memberData;
    ArrayData *arrayData;
};

}

struct Q_QML_EXPORT Object: Managed {
    V4_OBJECT2(Object, Object)
    Q_MANAGED_TYPE(Object)

    enum {
        IsObject = true
    };

    Heap::MemberData *memberData() { return d()->memberData; }
    const Heap::MemberData *memberData() const { return d()->memberData; }
    Heap::ArrayData *arrayData() const { return d()->arrayData; }
    void setArrayData(ArrayData *a) { d()->arrayData = a->d(); }

    const Property *propertyAt(uint index) const { return d()->propertyAt(index); }
    Property *propertyAt(uint index) { return d()->propertyAt(index); }

    const ObjectVTable *vtable() const { return reinterpret_cast<const ObjectVTable *>(internalClass()->vtable); }
    Heap::Object *prototype() const { return d()->prototype; }
    bool setPrototype(Object *proto);

    Property *__getOwnProperty__(String *name, PropertyAttributes *attrs = 0);
    Property *__getOwnProperty__(uint index, PropertyAttributes *attrs = 0);

    Property *__getPropertyDescriptor__(String *name, PropertyAttributes *attrs = 0) const;
    Property *__getPropertyDescriptor__(uint index, PropertyAttributes *attrs = 0) const;

    bool hasProperty(String *name) const;
    bool hasProperty(uint index) const;

    bool hasOwnProperty(String *name) const;
    bool hasOwnProperty(uint index) const;

    bool __defineOwnProperty__(ExecutionEngine *engine, uint index, String *member, const Property *p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionEngine *engine, String *name, const Property *p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionEngine *engine, uint index, const Property *p, PropertyAttributes attrs);
    bool __defineOwnProperty__(ExecutionEngine *engine, const QString &name, const Property *p, PropertyAttributes attrs);
    bool defineOwnProperty2(ExecutionEngine *engine, uint index, const Property *p, PropertyAttributes attrs);

    //
    // helpers
    //
    void put(ExecutionEngine *engine, const QString &name, const ValueRef value);

    static ReturnedValue getValue(const ValueRef thisObject, const Property *p, PropertyAttributes attrs);
    ReturnedValue getValue(const Property *p, PropertyAttributes attrs) const {
        Scope scope(this->engine());
        ScopedValue t(scope, const_cast<Object *>(this));
        return getValue(t, p, attrs);
    }

    void putValue(Property *pd, PropertyAttributes attrs, const ValueRef value);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, ValueRef value) {
        insertMember(name, value, Attr_Data|Attr_NotEnumerable);
    }
    void defineDefaultProperty(const QString &name, ValueRef value);
    void defineDefaultProperty(const QString &name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineDefaultProperty(String *name, ReturnedValue (*code)(CallContext *), int argumentCount = 0);
    void defineAccessorProperty(const QString &name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    void defineAccessorProperty(String *name, ReturnedValue (*getter)(CallContext *), ReturnedValue (*setter)(CallContext *));
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(const QString &name, ValueRef value);
    void defineReadonlyProperty(String *name, ValueRef value);

    void ensureMemberIndex(QV4::ExecutionEngine *e, uint idx) {
        d()->memberData = MemberData::reallocate(e, d()->memberData, idx);
    }

    void insertMember(String *s, const ValueRef v, PropertyAttributes attributes = Attr_Data) {
        Scope scope(engine());
        ScopedProperty p(scope);
        p->value = *v;
        insertMember(s, p, attributes);
    }
    void insertMember(String *s, const Property *p, PropertyAttributes attributes);

    inline ExecutionEngine *engine() const { return internalClass()->engine; }

    inline bool hasAccessorProperty() const { return d()->hasAccessorProperty; }
    inline void setHasAccessorProperty() { d()->hasAccessorProperty = true; }

    bool isExtensible() const { return d()->extensible; }
    void setExtensible(bool b) { d()->extensible = b; }

    // Array handling

public:
    void copyArrayData(Object *other);

    bool setArrayLength(uint newLen);
    void setArrayLengthUnchecked(uint l);

    void arraySet(uint index, const Property *p, PropertyAttributes attributes = Attr_Data);
    void arraySet(uint index, ValueRef value);

    bool arrayPut(uint index, ValueRef value) {
        return arrayData()->vtable()->put(this, index, value);
    }
    bool arrayPut(uint index, Value *values, uint n) {
        return arrayData()->vtable()->putArray(this, index, values, n);
    }
    void setArrayAttributes(uint i, PropertyAttributes a) {
        Q_ASSERT(arrayData());
        if (d()->arrayData->attrs || a != Attr_Data) {
            ArrayData::ensureAttributes(this);
            a.resolve();
            arrayData()->vtable()->setAttribute(this, i, a);
        }
    }

    void push_back(const ValueRef v);

    ArrayData::Type arrayType() const {
        return arrayData() ? d()->arrayData->type : Heap::ArrayData::Simple;
    }
    // ### remove me
    void setArrayType(ArrayData::Type t) {
        Q_ASSERT(t != Heap::ArrayData::Simple && t != Heap::ArrayData::Sparse);
        arrayCreate();
        d()->arrayData->type = t;
    }

    inline void arrayReserve(uint n) {
        ArrayData::realloc(this, Heap::ArrayData::Simple, n, false);
    }

    void arrayCreate() {
        if (!arrayData())
            ArrayData::realloc(this, Heap::ArrayData::Simple, 0, false);
#ifdef CHECK_SPARSE_ARRAYS
        initSparseArray();
#endif
    }

    void initSparseArray();
    SparseArrayNode *sparseBegin() { return arrayType() == Heap::ArrayData::Sparse ? d()->arrayData->sparse->begin() : 0; }
    SparseArrayNode *sparseEnd() { return arrayType() == Heap::ArrayData::Sparse ? d()->arrayData->sparse->end() : 0; }

    inline bool protoHasArray() {
        Scope scope(engine());
        Scoped<Object> p(scope, this);

        while ((p = p->prototype()))
            if (p->arrayData())
                return true;

        return false;
    }
    void ensureMemberIndex(uint idx);

    inline ReturnedValue get(String *name, bool *hasProperty = 0)
    { return vtable()->get(this, name, hasProperty); }
    inline ReturnedValue getIndexed(uint idx, bool *hasProperty = 0)
    { return vtable()->getIndexed(this, idx, hasProperty); }
    inline void put(String *name, const ValueRef v)
    { vtable()->put(this, name, v); }
    inline void putIndexed(uint idx, const ValueRef v)
    { vtable()->putIndexed(this, idx, v); }
    PropertyAttributes query(String *name) const
    { return vtable()->query(this, name); }
    PropertyAttributes queryIndexed(uint index) const
    { return vtable()->queryIndexed(this, index); }
    bool deleteProperty(String *name)
    { return vtable()->deleteProperty(this, name); }
    bool deleteIndexedProperty(uint index)
    { return vtable()->deleteIndexedProperty(this, index); }
    ReturnedValue getLookup(Lookup *l)
    { return vtable()->getLookup(this, l); }
    void setLookup(Lookup *l, const ValueRef v)
    { vtable()->setLookup(this, l, v); }
    void advanceIterator(ObjectIterator *it, Heap::String **name, uint *index, Property *p, PropertyAttributes *attributes)
    { vtable()->advanceIterator(this, it, name, index, p, attributes); }
    uint getLength() const { return vtable()->getLength(this); }

    inline ReturnedValue construct(CallData *d)
    { return vtable()->construct(this, d); }
    inline ReturnedValue call(CallData *d)
    { return vtable()->call(this, d); }
protected:
    static void markObjects(Heap::Base *that, ExecutionEngine *e);
    static ReturnedValue construct(Managed *m, CallData *);
    static ReturnedValue call(Managed *m, CallData *);
    static ReturnedValue get(Managed *m, String *name, bool *hasProperty);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void put(Managed *m, String *name, const ValueRef value);
    static void putIndexed(Managed *m, uint index, const ValueRef value);
    static PropertyAttributes query(const Managed *m, String *name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *m, String *name);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static ReturnedValue getLookup(Managed *m, Lookup *l);
    static void setLookup(Managed *m, Lookup *l, const ValueRef v);
    static void advanceIterator(Managed *m, ObjectIterator *it, Heap::String **name, uint *index, Property *p, PropertyAttributes *attributes);
    static uint getLength(const Managed *m);

private:
    ReturnedValue internalGet(String *name, bool *hasProperty);
    ReturnedValue internalGetIndexed(uint index, bool *hasProperty);
    void internalPut(String *name, const ValueRef value);
    void internalPutIndexed(uint index, const ValueRef value);
    bool internalDeleteProperty(String *name);
    bool internalDeleteIndexedProperty(uint index);

    friend struct ObjectIterator;
    friend struct ObjectPrototype;
};

namespace Heap {

struct BooleanObject : Object {
    BooleanObject(InternalClass *ic, QV4::Object *prototype)
        : Object(ic, prototype)
    {
        value = Encode((bool)false);
    }

    BooleanObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->booleanClass, engine->booleanPrototype.asObject())
    {
        value = val;
    }
    Value value;
};

struct NumberObject : Object {
    NumberObject(InternalClass *ic, QV4::Object *prototype)
        : Object(ic, prototype)
    {
        value = Encode((int)0);
    }

    NumberObject(ExecutionEngine *engine, const ValueRef val)
        : Object(engine->numberClass, engine->numberPrototype.asObject())
    {
        value = val;
    }
    Value value;
};

struct ArrayObject : Object {
    enum {
        LengthPropertyIndex = 0
    };

    ArrayObject(ExecutionEngine *engine)
        : Heap::Object(engine->arrayClass, engine->arrayPrototype.asObject())
    { init(); }
    ArrayObject(ExecutionEngine *engine, const QStringList &list);
    ArrayObject(InternalClass *ic, QV4::Object *prototype)
        : Heap::Object(ic, prototype)
    { init(); }
    void init()
    { memberData->data[LengthPropertyIndex] = Primitive::fromInt32(0); }
};

}

struct BooleanObject: Object {
    V4_OBJECT2(BooleanObject, Object)
    Q_MANAGED_TYPE(BooleanObject)

    Value value() const { return d()->value; }

};

struct NumberObject: Object {
    V4_OBJECT2(NumberObject, Object)
    Q_MANAGED_TYPE(NumberObject)

    Value value() const { return d()->value; }
};

struct ArrayObject: Object {
    V4_OBJECT2(ArrayObject, Object)
    Q_MANAGED_TYPE(ArrayObject)

    void init(ExecutionEngine *engine);

    static ReturnedValue getLookup(Managed *m, Lookup *l);
    using Object::getLength;
    static uint getLength(const Managed *m);

    QStringList toQStringList() const;
};

inline void Object::setArrayLengthUnchecked(uint l)
{
    if (isArrayObject())
        memberData()->data[Heap::ArrayObject::LengthPropertyIndex] = Primitive::fromUInt32(l);
}

inline void Object::push_back(const ValueRef v)
{
    arrayCreate();

    uint idx = getLength();
    arrayReserve(idx + 1);
    arrayPut(idx, v);
    setArrayLengthUnchecked(idx + 1);
}

inline void Object::arraySet(uint index, const Property *p, PropertyAttributes attributes)
{
    // ### Clean up
    arrayCreate();
    if (attributes.isAccessor()) {
        setHasAccessorProperty();
        initSparseArray();
    } else if (index > 0x1000 && index > 2*d()->arrayData->alloc) {
        initSparseArray();
    } else {
        arrayData()->vtable()->reallocate(this, index + 1, false);
    }
    setArrayAttributes(index, attributes);
    Property *pd = ArrayData::insert(this, index, attributes.isAccessor());
    pd->value = p->value;
    if (attributes.isAccessor())
        pd->set = p->set;
    if (isArrayObject() && index >= getLength())
        setArrayLengthUnchecked(index + 1);
}


inline void Object::arraySet(uint index, ValueRef value)
{
    arrayCreate();
    if (index > 0x1000 && index > 2*d()->arrayData->alloc) {
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
    return v.toObject(e)->asReturnedValue();
}
#endif

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
