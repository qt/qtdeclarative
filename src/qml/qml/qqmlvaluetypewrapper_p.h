// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLVALUETYPEWRAPPER_P_H
#define QQMLVALUETYPEWRAPPER_P_H

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

#include <QtCore/qglobal.h>
#include <private/qtqmlglobal_p.h>

#include <private/qv4referenceobject_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4sequenceobject_p.h>
#include <private/qv4value_p.h>
#include <private/qv4referenceobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlValueType;

namespace QV4 {

namespace Heap {

#define QQmlValueTypeWrapperMembers(class, Member)

DECLARE_HEAP_OBJECT(QQmlValueTypeWrapper, ReferenceObject) {
    DECLARE_MARKOBJECTS(QQmlValueTypeWrapper);

    void init(
        const void *data, QMetaType metaType, const QMetaObject *metaObject,
        Object *object, int property, Flags flags)
    {
        ReferenceObject::init(object, property, flags);
        setMetaType(metaType);
        setMetaObject(metaObject);
        if (data)
            setData(data);
    }

    QQmlValueTypeWrapper *detached() const;

    void destroy();

    QMetaType metaType() const
    {
        Q_ASSERT(m_metaType != nullptr);
        return QMetaType(m_metaType);
    }

    void setGadgetPtr(void *gadgetPtr) { m_gadgetPtr = gadgetPtr; }
    void *gadgetPtr() const { return m_gadgetPtr; }

    const QMetaObject *metaObject() const { return m_metaObject; }

    void setData(const void *data)
    {
        const QMetaType type = metaType();
        void *gadget = gadgetPtr();
        if (gadget) {
            type.destruct(gadget);
        } else {
            gadget = ::operator new(type.sizeOf());
            setGadgetPtr(gadget);
        }
        type.construct(gadget, data);
    }

    QVariant toVariant() const;

    void *storagePointer();
    bool setVariant(const QVariant &variant);

    bool readReference();
    bool writeBack(int propertyIndex = QV4::ReferenceObject::AllProperties);

private:
    void setMetaObject(const QMetaObject *metaObject) { m_metaObject = metaObject; }
    void setMetaType(QMetaType metaType)
    {
        Q_ASSERT(metaType.isValid());
        m_metaType = metaType.iface();
    }

    void *m_gadgetPtr;
    const QtPrivate::QMetaTypeInterface *m_metaType;
    const QMetaObject *m_metaObject;
};

}

struct Q_QML_EXPORT QQmlValueTypeWrapper : public ReferenceObject
{
    V4_OBJECT2(QQmlValueTypeWrapper, ReferenceObject)
    V4_PROTOTYPE(valueTypeWrapperPrototype)
    V4_NEEDS_DESTROY

public:

    static ReturnedValue create(
            ExecutionEngine *engine, const void *data, const QMetaObject *metaObject,
            QMetaType type, Heap::Object *object, int property, Heap::ReferenceObject::Flags flags);
    static ReturnedValue create(
            ExecutionEngine *engine, Heap::QQmlValueTypeWrapper *cloneFrom, Heap::Object *object);
    static ReturnedValue create(
            ExecutionEngine *engine, const void *, const QMetaObject *metaObject, QMetaType type);

    QVariant toVariant() const;

    template<typename ValueType>
    ValueType *cast()
    {
        if (QMetaType::fromType<ValueType>() != d()->metaType())
            return nullptr;
        if (d()->isReference() && !readReferenceValue())
            return nullptr;
        return static_cast<ValueType *>(d()->gadgetPtr());
    }

    bool toGadget(void *data) const;
    bool isEqual(const QVariant& value) const;
    int typeId() const;
    QMetaType type() const;
    bool write(QObject *target, int propertyIndex) const;
    bool readReferenceValue() const { return d()->readReference(); }
    const QMetaObject *metaObject() const { return d()->metaObject(); }

    QQmlPropertyData dataForPropertyKey(PropertyKey id) const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static bool virtualIsEqualTo(Managed *m, Managed *other);
    static bool virtualHasProperty(const Managed *m, PropertyKey id);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static ReturnedValue method_toString(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);
    static bool virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value);
    static ReturnedValue lookupGetter(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static bool lookupSetter(QV4::Lookup *l, QV4::ExecutionEngine *engine,
                             QV4::Value &object, const QV4::Value &value);

    static void initProto(ExecutionEngine *v4);
    static int virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a);
};

}

QT_END_NAMESPACE

#endif // QV8VALUETYPEWRAPPER_P_H


