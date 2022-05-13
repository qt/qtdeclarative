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

#include <private/qv4value_p.h>
#include <private/qv4object_p.h>
#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE

class QQmlValueType;

namespace QV4 {

namespace Heap {

struct QQmlValueTypeWrapper : Object {
    void init() { Object::init(); }
    void destroy();

    void setValueType(QQmlValueType *valueType)
    {
        Q_ASSERT(valueType != nullptr);
        m_valueType = valueType;
    }

    QQmlValueType *valueType() const
    {
        Q_ASSERT(m_valueType != nullptr);
        return m_valueType;
    }

    void setGadgetPtr(void *gadgetPtr) const
    {
        m_gadgetPtr = gadgetPtr;
    }

    void *gadgetPtr() const
    {
        return m_gadgetPtr;
    }

    void setMetaObject(const QMetaObject *metaObject)
    {
        m_metaObject = metaObject;
    }
    const QMetaObject *metaObject() const
    {
        return m_metaObject;
    }

    void setData(const void *data) const;
    void setValue(const QVariant &value) const;
    QVariant toVariant() const;

private:
    mutable void *m_gadgetPtr;
    QQmlValueType *m_valueType;
    const QMetaObject *m_metaObject;
};

struct QQmlValueTypeReference : QQmlValueTypeWrapper
{
    void init() {
        QQmlValueTypeWrapper::init();
        object.init();
    }
    void destroy() {
        object.destroy();
        QQmlValueTypeWrapper::destroy();
    }

    void writeBack() {
        const QMetaProperty writebackProperty = object->metaObject()->property(property);
        if (!writebackProperty.isWritable())
            return;

        int flags = 0;
        int status = -1;
        if (writebackProperty.metaType() == QMetaType::fromType<QVariant>()) {
            QVariant variantReferenceValue = toVariant();
            void *a[] = { &variantReferenceValue, nullptr, &status, &flags };
            QMetaObject::metacall(object, QMetaObject::WriteProperty, property, a);
        } else {
            void *a[] = { gadgetPtr(), nullptr, &status, &flags };
            QMetaObject::metacall(object, QMetaObject::WriteProperty, property, a);
        }
    }

    QV4QPointer<QObject> object;
    int property;
};

}

struct Q_QML_EXPORT QQmlValueTypeWrapper : Object
{
    V4_OBJECT2(QQmlValueTypeWrapper, Object)
    V4_PROTOTYPE(valueTypeWrapperPrototype)
    V4_NEEDS_DESTROY

public:

    static ReturnedValue create(ExecutionEngine *engine, QObject *, int, const QMetaObject *metaObject, QMetaType type);
    static ReturnedValue create(ExecutionEngine *engine, const QVariant &, const QMetaObject *metaObject, QMetaType type);
    static ReturnedValue create(ExecutionEngine *engine, const void *, const QMetaObject *metaObject, QMetaType type);

    QVariant toVariant() const;
    bool toGadget(void *data) const;
    bool isEqual(const QVariant& value) const;
    int typeId() const;
    QMetaType type() const;
    bool write(QObject *target, int propertyIndex) const;

    QQmlPropertyData dataForPropertyKey(PropertyKey id) const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static bool virtualIsEqualTo(Managed *m, Managed *other);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static ReturnedValue method_toString(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);
    static bool virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value);
    static ReturnedValue lookupGetter(Lookup *lookup, ExecutionEngine *engine, const Value &object);
    static bool lookupSetter(QV4::Lookup *l, QV4::ExecutionEngine *engine,
                             QV4::Value &object, const QV4::Value &value);

    static void initProto(ExecutionEngine *v4);
};

struct QQmlValueTypeReference : public QQmlValueTypeWrapper
{
    V4_OBJECT2(QQmlValueTypeReference, QQmlValueTypeWrapper)
    V4_NEEDS_DESTROY

    bool readReferenceValue() const;
};

}

QT_END_NAMESPACE

#endif // QV8VALUETYPEWRAPPER_P_H


