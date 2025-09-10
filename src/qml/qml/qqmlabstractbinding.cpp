// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlabstractbinding_p.h"

#include <QtQml/qqmlinfo.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlvaluetypeproxybinding_p.h>
#include <private/qqmlvmemetaobject_p.h>

QT_BEGIN_NAMESPACE

QQmlAbstractBinding::QQmlAbstractBinding()
    : m_targetIndex(-1)
{
    Q_ASSERT(!isAddedToObject());
}

QQmlAbstractBinding::~QQmlAbstractBinding()
{
    Q_ASSERT(!ref);
    Q_ASSERT(!isAddedToObject());

    if (m_nextBinding.data() && !m_nextBinding->ref.deref())
        delete m_nextBinding.data();
}

/*!
Add this binding to \a object.

This transfers ownership of the binding to the object, marks the object's property as
being bound.

However, it does not enable the binding itself or call update() on it.
*/
void QQmlAbstractBinding::addToObject()
{
    Q_ASSERT(!nextBinding());
    Q_ASSERT(isAddedToObject() == false);

    QObject *obj = targetObject();
    Q_ASSERT(obj);

    QQmlData *data = QQmlData::get(obj, true);

    int coreIndex = targetPropertyIndex().coreIndex();
    if (targetPropertyIndex().hasValueTypeIndex()) {
        // Value type

        // Find the value type proxy (if there is one)
        QQmlValueTypeProxyBinding *proxy = nullptr;
        QQmlAbstractBinding *b = data->bindings;
        while (b && (b->targetPropertyIndex().coreIndex() != coreIndex ||
                     b->targetPropertyIndex().hasValueTypeIndex())) {
            b = b->nextBinding();
        }

        if (b && b->kind() == QQmlAbstractBinding::ValueTypeProxy) {
            proxy = static_cast<QQmlValueTypeProxyBinding *>(b);
        } else {
            proxy = new QQmlValueTypeProxyBinding(obj, QQmlPropertyIndex(coreIndex));

            Q_ASSERT(proxy->targetPropertyIndex().coreIndex() == coreIndex);
            Q_ASSERT(!proxy->targetPropertyIndex().hasValueTypeIndex());
            Q_ASSERT(proxy->targetObject() == obj);

            proxy->addToObject();
        }

        setNextBinding(proxy->m_bindings.data());
        proxy->m_bindings = this;

    } else {
        setNextBinding(data->bindings);
        if (data->bindings) {
            data->bindings->ref.deref();
            Q_ASSERT(data->bindings->ref.refCount > 0);
        }
        data->bindings = this;
        ref.ref();

        data->setBindingBit(obj, coreIndex);
    }

    setAddedToObject(true);
}

/*!
Remove the binding from the object.
*/
void QQmlAbstractBinding::removeFromObject()
{
    if (!isAddedToObject())
        return;

    setAddedToObject(false);

    QObject *obj = targetObject();
    QQmlData *data = QQmlData::get(obj, false);
    Q_ASSERT(data);

    QQmlAbstractBinding::Ptr next;
    next = nextBinding();
    setNextBinding(nullptr);

    int coreIndex = targetPropertyIndex().coreIndex();
    if (targetPropertyIndex().hasValueTypeIndex()) {

        // Find the value type binding
        QQmlAbstractBinding *vtbinding = data->bindings;
        Q_ASSERT(vtbinding);
        while (vtbinding->targetPropertyIndex().coreIndex() != coreIndex
               || vtbinding->targetPropertyIndex().hasValueTypeIndex()) {
            vtbinding = vtbinding->nextBinding();
            Q_ASSERT(vtbinding);
        }
        Q_ASSERT(vtbinding->kind() == QQmlAbstractBinding::ValueTypeProxy);

        QQmlValueTypeProxyBinding *vtproxybinding =
            static_cast<QQmlValueTypeProxyBinding *>(vtbinding);

        QQmlAbstractBinding *binding = vtproxybinding->m_bindings.data();
        if (binding == this) {
            vtproxybinding->m_bindings = next;
        } else {
           while (binding->nextBinding() != this) {
              binding = binding->nextBinding();
              Q_ASSERT(binding);
           }
           binding->setNextBinding(next.data());
        }

        // Value type - we don't remove the proxy from the object.  It will sit their happily
        // doing nothing until it is removed by a write, a binding change or it is reused
        // to hold more sub-bindings.
        return;
    }

    if (data->bindings == this) {
        if (next.data())
            next->ref.ref();
        data->bindings = next.data();
        if (!ref.deref())
            delete this;
    } else {
        QQmlAbstractBinding *binding = data->bindings;
        while (binding->nextBinding() != this) {
            binding = binding->nextBinding();
            Q_ASSERT(binding);
        }
        binding->setNextBinding(next.data());
    }

    data->clearBindingBit(coreIndex);
}

void QQmlAbstractBinding::printBindingLoopError(const QQmlProperty &prop)
{
    qmlWarning(prop.object()) << QString(QLatin1String("Binding loop detected for property \"%1\"")).arg(prop.name());
}

void QQmlAbstractBinding::getPropertyData(
        const QQmlPropertyData **propertyData, QQmlPropertyData *valueTypeData) const
{
    Q_ASSERT(propertyData);

    QQmlData *data = QQmlData::get(m_target.data(), false);
    Q_ASSERT(data);

    if (Q_UNLIKELY(!data->propertyCache))
        data->propertyCache = QQmlMetaType::propertyCache(m_target->metaObject());

    *propertyData = data->propertyCache->property(m_targetIndex.coreIndex());
    Q_ASSERT(*propertyData);

    if (Q_UNLIKELY(m_targetIndex.hasValueTypeIndex() && valueTypeData)) {
        const QMetaObject *valueTypeMetaObject
                = QQmlMetaType::metaObjectForValueType((*propertyData)->propType());
        Q_ASSERT(valueTypeMetaObject);
        QMetaProperty vtProp = valueTypeMetaObject->property(m_targetIndex.valueTypeIndex());
        valueTypeData->setFlags(QQmlPropertyData::flagsForProperty(vtProp));
        valueTypeData->setPropType(vtProp.metaType());
        valueTypeData->setCoreIndex(m_targetIndex.valueTypeIndex());
    }
}

void QQmlAbstractBinding::updateCanUseAccessor()
{
    setCanUseAccessor(true); // Always use accessors, except when:
    if (auto interceptorMetaObject = QQmlInterceptorMetaObject::get(targetObject())) {
        if (!m_targetIndex.isValid() || interceptorMetaObject->intercepts(m_targetIndex))
            setCanUseAccessor(false);
    }
}

void QQmlAbstractBinding::setTarget(const QQmlProperty &prop)
{
    auto pd = QQmlPropertyPrivate::get(prop);
    setTarget(prop.object(), pd->core, &pd->valueTypeData);
}

bool QQmlAbstractBinding::setTarget(
        QObject *object, const QQmlPropertyData &core, const QQmlPropertyData *valueType)
{
    return setTarget(object, core.coreIndex(), core.isAlias(),
                     valueType ? valueType->coreIndex() : -1);
}

static const QQmlPropertyData *getObjectPropertyData(QObject *object, int coreIndex)
{
    QQmlData *data = QQmlData::get(object, true);
    if (!data)
        return nullptr;

    if (!data->propertyCache) {
        data->propertyCache = QQmlMetaType::propertyCache(object);
        if (!data->propertyCache)
            return nullptr;
    }

    const QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
    Q_ASSERT(propertyData);
    return propertyData;
}

bool QQmlAbstractBinding::setTarget(
        QObject *object, int coreIndex, bool coreIsAlias, int valueTypeIndex)
{
    auto invalidate = [this]() {
        m_target = nullptr;
        m_targetIndex = QQmlPropertyIndex();
        return false;
    };

    if (!object)
        return invalidate();

    m_target = object;

    for (bool isAlias = coreIsAlias; isAlias;) {
        QQmlVMEMetaObject *vme = QQmlVMEMetaObject::getForProperty(object, coreIndex);

        int aValueTypeIndex;
        if (!vme->aliasTarget(coreIndex, &object, &coreIndex, &aValueTypeIndex)) {
            // can't resolve id (yet)
            return invalidate();
        }

        const QQmlPropertyData *propertyData = getObjectPropertyData(object, coreIndex);
        if (!propertyData)
            return invalidate();

        if (aValueTypeIndex != -1) {
            if (propertyData->propType().flags().testFlag(QMetaType::PointerToQObject)) {
                // deep alias
                propertyData->readProperty(object, &object);
                coreIndex = aValueTypeIndex;
                valueTypeIndex = -1;
                propertyData = getObjectPropertyData(object, coreIndex);
                if (!propertyData)
                    return invalidate();
            } else {
                valueTypeIndex = aValueTypeIndex;
            }
        }

        m_target = object;
        isAlias = propertyData->isAlias();
        coreIndex = propertyData->coreIndex();
    }
    m_targetIndex = QQmlPropertyIndex(coreIndex, valueTypeIndex);

    QQmlData *data = QQmlData::get(m_target.data(), true);
    if (!data->propertyCache)
        data->propertyCache = QQmlMetaType::propertyCache(m_target->metaObject());

    return true;
}


QString QQmlAbstractBinding::expression() const
{
    return QLatin1String("<Unknown>");
}

QT_END_NAMESPACE
