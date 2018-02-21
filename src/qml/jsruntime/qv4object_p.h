/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4_OBJECT_H
#define QV4_OBJECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4managed_p.h"
#include "qv4memberdata_p.h"
#include "qv4arraydata_p.h"
#include "qv4engine_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4value_p.h"
#include "qv4internalclass_p.h"
#include "qv4string_p.h"

QT_BEGIN_NAMESPACE


namespace QV4 {

namespace Heap {

#define ObjectMembers(class, Member) \
    Member(class, Pointer, MemberData *, memberData) \
    Member(class, Pointer, ArrayData *, arrayData)

DECLARE_EXPORTED_HEAP_OBJECT(Object, Base) {
    static void markObjects(Heap::Base *base, MarkStack *stack);
    void init() { Base::init(); }

    const Value *inlinePropertyDataWithOffset(uint indexWithOffset) const {
        Q_ASSERT(indexWithOffset >= vtable()->inlinePropertyOffset && indexWithOffset < vtable()->inlinePropertyOffset + vtable()->nInlineProperties);
        return reinterpret_cast<const Value *>(this) + indexWithOffset;
    }
    const Value *inlinePropertyData(uint index) const {
        Q_ASSERT(index < vtable()->nInlineProperties);
        return reinterpret_cast<const Value *>(this) + vtable()->inlinePropertyOffset + index;
    }
    void setInlineProperty(ExecutionEngine *e, uint index, Value v) {
        Q_ASSERT(index < vtable()->nInlineProperties);
        Value *prop = reinterpret_cast<Value *>(this) + vtable()->inlinePropertyOffset + index;
        WriteBarrier::write(e, this, prop->data_ptr(), v.asReturnedValue());
    }
    void setInlineProperty(ExecutionEngine *e, uint index, Heap::Base *b) {
        Q_ASSERT(index < vtable()->nInlineProperties);
        Value *prop = reinterpret_cast<Value *>(this) + vtable()->inlinePropertyOffset + index;
        WriteBarrier::write(e, this, prop->data_ptr(), b->asReturnedValue());
    }

    QV4::MemberData::Index writablePropertyData(uint index) {
        uint nInline = vtable()->nInlineProperties;
        if (index < nInline)
            return { this, reinterpret_cast<Value *>(this) + vtable()->inlinePropertyOffset + index};
        index -= nInline;
        return { memberData, memberData->values.values + index };
    }

    const Value *propertyData(uint index) const {
        uint nInline = vtable()->nInlineProperties;
        if (index < nInline)
            return reinterpret_cast<const Value *>(this) + vtable()->inlinePropertyOffset + index;
        index -= nInline;
        return memberData->values.data() + index;
    }
    void setProperty(ExecutionEngine *e, uint index, Value v) {
        uint nInline = vtable()->nInlineProperties;
        if (index < nInline) {
            setInlineProperty(e, index, v);
            return;
        }
        index -= nInline;
        memberData->values.set(e, index, v);
    }
    void setProperty(ExecutionEngine *e, uint index, Heap::Base *b) {
        uint nInline = vtable()->nInlineProperties;
        if (index < nInline) {
            setInlineProperty(e, index, b);
            return;
        }
        index -= nInline;
        memberData->values.set(e, index, b);
    }

    void setUsedAsProto();

    Heap::Object *prototype() const { return internalClass->prototype; }
};

}

#define V4_OBJECT2(DataClass, superClass) \
    private: \
        DataClass() Q_DECL_EQ_DELETE; \
        Q_DISABLE_COPY(DataClass) \
    public: \
        Q_MANAGED_CHECK \
        typedef QV4::Heap::DataClass Data; \
        typedef superClass SuperClass; \
        static const QV4::ObjectVTable static_vtbl; \
        static inline const QV4::VTable *staticVTable() { return &static_vtbl.vTable; } \
        V4_MANAGED_SIZE_TEST \
        QV4::Heap::DataClass *d_unchecked() const { return static_cast<QV4::Heap::DataClass *>(m()); } \
        QV4::Heap::DataClass *d() const { \
            QV4::Heap::DataClass *dptr = d_unchecked(); \
            dptr->_checkIsInitialized(); \
            return dptr; \
        } \
        Q_STATIC_ASSERT(std::is_trivial< QV4::Heap::DataClass >::value);

#define V4_PROTOTYPE(p) \
    static QV4::Object *defaultPrototype(QV4::ExecutionEngine *e) \
    { return e->p(); }

struct ObjectVTable
{
    VTable vTable;
    ReturnedValue (*call)(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    ReturnedValue (*callAsConstructor)(const FunctionObject *, const Value *argv, int argc);
    ReturnedValue (*get)(const Managed *, String *name, bool *hasProperty);
    ReturnedValue (*getIndexed)(const Managed *, uint index, bool *hasProperty);
    bool (*put)(Managed *, String *name, const Value &value);
    bool (*putIndexed)(Managed *, uint index, const Value &value);
    PropertyAttributes (*query)(const Managed *, String *name);
    PropertyAttributes (*queryIndexed)(const Managed *, uint index);
    bool (*deleteProperty)(Managed *m, String *name);
    bool (*deleteIndexedProperty)(Managed *m, uint index);
    uint (*getLength)(const Managed *m);
    void (*advanceIterator)(Managed *m, ObjectIterator *it, Value *name, uint *index, Property *p, PropertyAttributes *attributes);
    ReturnedValue (*instanceOf)(const Object *typeObject, const Value &var);
};

#define DEFINE_OBJECT_VTABLE_BASE(classname) \
const QV4::ObjectVTable classname::static_vtbl =    \
{     \
    DEFINE_MANAGED_VTABLE_INT(classname, (std::is_same<classname::SuperClass, Object>::value) ? nullptr : &classname::SuperClass::static_vtbl.vTable), \
    call,                                       \
    callAsConstructor,                          \
    get,                                        \
    getIndexed,                                 \
    put,                                        \
    putIndexed,                                 \
    query,                                      \
    queryIndexed,                               \
    deleteProperty,                             \
    deleteIndexedProperty,                      \
    getLength,                                  \
    advanceIterator,                            \
    instanceOf                                  \
}

#define DEFINE_OBJECT_VTABLE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
DEFINE_OBJECT_VTABLE_BASE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF

#define DEFINE_OBJECT_TEMPLATE_VTABLE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
template<> DEFINE_OBJECT_VTABLE_BASE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF

struct Q_QML_EXPORT Object: Managed {
    V4_OBJECT2(Object, Object)
    Q_MANAGED_TYPE(Object)
    V4_INTERNALCLASS(Object)
    V4_PROTOTYPE(objectPrototype)

    enum { NInlineProperties = 2 };

    enum {
        IsObject = true,
        GetterOffset = 0,
        SetterOffset = 1
    };

    void setInternalClass(InternalClass *ic);

    const Value *propertyData(uint index) const { return d()->propertyData(index); }

    Heap::ArrayData *arrayData() const { return d()->arrayData; }
    void setArrayData(ArrayData *a) { d()->arrayData.set(engine(), a->d()); }

    void getProperty(uint index, Property *p, PropertyAttributes *attrs) const;
    void setProperty(uint index, const Property *p);
    void setProperty(uint index, Value v) const { d()->setProperty(engine(), index, v); }
    void setProperty(uint index, Heap::Base *b) const { d()->setProperty(engine(), index, b); }
    void setProperty(ExecutionEngine *engine, uint index, Value v) const { d()->setProperty(engine, index, v); }
    void setProperty(ExecutionEngine *engine, uint index, Heap::Base *b) const { d()->setProperty(engine, index, b); }

    const ObjectVTable *vtable() const { return reinterpret_cast<const ObjectVTable *>(d()->vtable()); }
    Heap::Object *prototype() const { return d()->prototype(); }
    bool setPrototype(Object *proto);

    void getOwnProperty(String *name, PropertyAttributes *attrs, Property *p = nullptr);
    void getOwnProperty(uint index, PropertyAttributes *attrs, Property *p = nullptr);

    MemberData::Index getValueOrSetter(String *name, PropertyAttributes *attrs);
    ArrayData::Index getValueOrSetter(uint index, PropertyAttributes *attrs);

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
    static ReturnedValue getValue(const Value &thisObject, const Value &v, PropertyAttributes attrs);
    ReturnedValue getValue(const Value &v, PropertyAttributes attrs) const {
        Scope scope(this->engine());
        ScopedValue t(scope, const_cast<Object *>(this));
        return getValue(t, v, attrs);
    }

    bool putValue(uint memberIndex, const Value &value);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, const Value &value) {
        insertMember(name, value, Attr_Data|Attr_NotEnumerable);
    }
    void defineDefaultProperty(const QString &name, const Value &value);
    void defineDefaultProperty(const QString &name, ReturnedValue (*code)(const FunctionObject *, const Value *thisObject, const Value *argv, int argc), int argumentCount = 0);
    void defineDefaultProperty(String *name, ReturnedValue (*code)(const FunctionObject *, const Value *thisObject, const Value *argv, int argc), int argumentCount = 0);
    void defineAccessorProperty(const QString &name, ReturnedValue (*getter)(const FunctionObject *, const Value *, const Value *, int),
                                ReturnedValue (*setter)(const FunctionObject *, const Value *, const Value *, int));
    void defineAccessorProperty(String *name, ReturnedValue (*getter)(const FunctionObject *, const Value *, const Value *, int),
                                ReturnedValue (*setter)(const FunctionObject *, const Value *, const Value *, int));
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(const QString &name, const Value &value);
    void defineReadonlyProperty(String *name, const Value &value);

    /* Fixed: Writable: false, Enumerable: false, Configurable: true */
    void defineReadonlyConfigurableProperty(const QString &name, const Value &value);
    void defineReadonlyConfigurableProperty(String *name, const Value &value);

    void insertMember(String *s, const Value &v, PropertyAttributes attributes = Attr_Data) {
        Scope scope(engine());
        ScopedProperty p(scope);
        p->value = v;
        insertMember(s, p, attributes);
    }
    void insertMember(String *s, const Property *p, PropertyAttributes attributes);

    bool isExtensible() const { return d()->internalClass->extensible; }

    // Array handling

public:
    void copyArrayData(Object *other);

    bool setArrayLength(uint newLen);
    void setArrayLengthUnchecked(uint l);

    void arraySet(uint index, const Property *p, PropertyAttributes attributes = Attr_Data);
    void arraySet(uint index, const Value &value);

    bool arrayPut(uint index, const Value &value) {
        return arrayData()->vtable()->put(this, index, value);
    }
    bool arrayPut(uint index, const Value *values, uint n) {
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

    void push_back(const Value &v);

    ArrayData::Type arrayType() const {
        return arrayData() ? static_cast<ArrayData::Type>(d()->arrayData->type) : Heap::ArrayData::Simple;
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
    SparseArrayNode *sparseBegin() { return arrayType() == Heap::ArrayData::Sparse ? d()->arrayData->sparse->begin() : nullptr; }
    SparseArrayNode *sparseEnd() { return arrayType() == Heap::ArrayData::Sparse ? d()->arrayData->sparse->end() : nullptr; }

    inline bool protoHasArray() {
        Scope scope(engine());
        ScopedObject p(scope, this);

        while ((p = p->prototype()))
            if (p->arrayData())
                return true;

        return false;
    }

    inline ReturnedValue get(String *name, bool *hasProperty = nullptr) const
    { return vtable()->get(this, name, hasProperty); }
    inline ReturnedValue getIndexed(uint idx, bool *hasProperty = nullptr) const
    { return vtable()->getIndexed(this, idx, hasProperty); }

    // use the set variants instead, to customize throw behavior
    inline bool put(String *name, const Value &v)
    { return vtable()->put(this, name, v); }
    inline bool putIndexed(uint idx, const Value &v)
    { return vtable()->putIndexed(this, idx, v); }

    enum ThrowOnFailure {
        DoThrowOnRejection,
        DoNotThrow
    };

    // ES6: 7.3.3 Set (O, P, V, Throw)
    inline bool set(String *name, const Value &v, ThrowOnFailure shouldThrow)
    {
        bool ret = vtable()->put(this, name, v);
        // ES6: 7.3.3, 6: If success is false and Throw is true, throw a TypeError exception.
        if (!ret && shouldThrow == ThrowOnFailure::DoThrowOnRejection) {
            ExecutionEngine *e = engine();
            if (!e->hasException) { // allow a custom set impl to throw itself
                QString message = QLatin1String("Cannot assign to read-only property \"") +
                        name->toQString() + QLatin1Char('\"');
                e->throwTypeError(message);
            }
        }
        return ret;
    }

    PropertyAttributes query(String *name) const
    { return vtable()->query(this, name); }
    PropertyAttributes queryIndexed(uint index) const
    { return vtable()->queryIndexed(this, index); }
    bool deleteProperty(String *name)
    { return vtable()->deleteProperty(this, name); }
    bool deleteIndexedProperty(uint index)
    { return vtable()->deleteIndexedProperty(this, index); }
    void advanceIterator(ObjectIterator *it, Value *name, uint *index, Property *p, PropertyAttributes *attributes)
    { vtable()->advanceIterator(this, it, name, index, p, attributes); }
    uint getLength() const { return vtable()->getLength(this); }
    ReturnedValue instanceOf(const Value &var) const
    { return vtable()->instanceOf(this, var); }

protected:
    static ReturnedValue callAsConstructor(const FunctionObject *f, const Value *argv, int argc);
    static ReturnedValue call(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue get(const Managed *m, String *name, bool *hasProperty);
    static ReturnedValue getIndexed(const Managed *m, uint index, bool *hasProperty);
    static bool put(Managed *m, String *name, const Value &value);
    static bool putIndexed(Managed *m, uint index, const Value &value);
    static PropertyAttributes query(const Managed *m, String *name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *m, String *name);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static void advanceIterator(Managed *m, ObjectIterator *it, Value *name, uint *index, Property *p, PropertyAttributes *attributes);
    static uint getLength(const Managed *m);
    static ReturnedValue instanceOf(const Object *typeObject, const Value &var);

private:
    ReturnedValue internalGet(String *name, bool *hasProperty) const;
    ReturnedValue internalGetIndexed(uint index, bool *hasProperty) const;
    bool internalPut(String *name, const Value &value);
    bool internalPutIndexed(uint index, const Value &value);
    bool internalDeleteProperty(String *name);
    bool internalDeleteIndexedProperty(uint index);

    friend struct ObjectIterator;
    friend struct ObjectPrototype;
};

namespace Heap {

struct BooleanObject : Object {
    void init() { Object::init(); }
    void init(bool b) {
        Object::init();
        this->b = b;
    }

    bool b;
};

struct NumberObject : Object {
    void init() { Object::init(); }
    void init(double val) {
        Object::init();
        value = val;
    }

    double value;
};

struct ArrayObject : Object {
    enum {
        LengthPropertyIndex = 0
    };

    void init() {
        Object::init();
        commonInit();
    }

    void init(const QStringList &list);

private:
    void commonInit()
    { setProperty(internalClass->engine, LengthPropertyIndex, Primitive::fromInt32(0)); }
};

}

struct BooleanObject: Object {
    V4_OBJECT2(BooleanObject, Object)
    Q_MANAGED_TYPE(BooleanObject)
    V4_PROTOTYPE(booleanPrototype)

    bool value() const { return d()->b; }

};

struct NumberObject: Object {
    V4_OBJECT2(NumberObject, Object)
    Q_MANAGED_TYPE(NumberObject)
    V4_PROTOTYPE(numberPrototype)

    double value() const { return d()->value; }
};

struct ArrayObject: Object {
    V4_OBJECT2(ArrayObject, Object)
    Q_MANAGED_TYPE(ArrayObject)
    V4_INTERNALCLASS(ArrayObject)
    V4_PROTOTYPE(arrayPrototype)

    void init(ExecutionEngine *engine);

    using Object::getLength;
    static uint getLength(const Managed *m);

    QStringList toQStringList() const;
};

inline void Object::setArrayLengthUnchecked(uint l)
{
    if (isArrayObject())
        setProperty(Heap::ArrayObject::LengthPropertyIndex, Primitive::fromUInt32(l));
}

inline void Object::push_back(const Value &v)
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
    if (attributes.isAccessor() || (index > 0x1000 && index > 2*d()->arrayData->values.alloc)) {
        initSparseArray();
    } else {
        arrayData()->vtable()->reallocate(this, index + 1, false);
    }
    setArrayAttributes(index, attributes);
    ArrayData::insert(this, index, &p->value, attributes.isAccessor());
    if (isArrayObject() && index >= getLength())
        setArrayLengthUnchecked(index + 1);
}


inline void Object::arraySet(uint index, const Value &value)
{
    arrayCreate();
    if (index > 0x1000 && index > 2*d()->arrayData->values.alloc) {
        initSparseArray();
    }
    ArrayData::insert(this, index, &value);
    if (isArrayObject() && index >= getLength())
        setArrayLengthUnchecked(index + 1);
}


template<>
inline const ArrayObject *Value::as() const {
    return isManaged() && m()->vtable()->type == Managed::Type_ArrayObject ? static_cast<const ArrayObject *>(this) : nullptr;
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
