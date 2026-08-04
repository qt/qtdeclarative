// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qobjectregistry_p.h"

#include <private/qabstractobjectregistryref_p.h>
#include <private/qobjectregistrysingleton_p.h>
#include <private/qqmldata_p.h>

#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ObjectRegistry
    \inqmlmodule QtQml.DesignSupport
    \ingroup qmldesignsupport_qml
    \brief This type is used to register objects for later access based on a key string.

    The object registry provides a QML engine-wide reference for objects. You register each object
    with a string key, which you can use later to retrieve the object through one of the following
    reference types: ObjectRegistryRef and MultiObjectRegistryRef. C++ classes are also provided
    for accessing registered objects: QObjectRegistryRef and QMultiObjectRegistryRef.

    The same key can be used to register multiple objects. A typical use case for this is
    registering a list or repeater delegate.

    The registered object is automatically removed from the registry on either object
    destruction or ObjectRegistry instance destruction. Changing the target object or the key
    removes the old combination from the registry.

    Do not register the same object and key combination more than once. Only one registration per
    combination is stored.

    The registration can be done either with attached property on the registered object or
    using separate ObjectRegistry object with target and key:

    \snippet doc_src_objectregistry.cpp 6

    \sa ObjectRegistryRef, MultiObjectRegistryRef, QObjectRegistryRef, QMultiObjectRegistryRef
*/

/*!
    \qmlproperty string ObjectRegistry::key
    This property specifies the key of the registered object.
*/

/*!
    \qmlproperty QtObject ObjectRegistry::target
    This property specifies the target object to register.

    If the target object is destroyed, this property is reset to null.
*/

QObjectRegistryAttachedType::QObjectRegistryAttachedType(QObject *parent)
    : QObject(parent)
{
    m_registry = QObjectRegistrySingleton::registryForObject(parent);
}

QObjectRegistryAttachedType::~QObjectRegistryAttachedType()
{
    if (m_registry)
        m_registry->remove(m_key, parent());
}

QString QObjectRegistryAttachedType::key() const
{
    return m_key;
}

void QObjectRegistryAttachedType::setKey(const QString &key)
{
    if (m_key == key)
        return;

    if (m_qmlSetupInProgress) {
        m_key = key;
        emit keyChanged();
        return;
    }

    if (m_registry)
        m_registry->remove(m_key, parent());

    m_key = key;

    if (m_registry)
        m_registry->add(m_key, parent());

    emit keyChanged();
}

void QObjectRegistryAttachedType::classBegin()
{
    m_qmlSetupInProgress = true;
}

void QObjectRegistryAttachedType::componentComplete()
{
    m_qmlSetupInProgress = false;

    if (m_registry)
        m_registry->add(m_key, parent());
}

QObjectRegistry::QObjectRegistry(QObject *parent)
    : QObject(parent)
{
}

QObjectRegistry::~QObjectRegistry()
{
    if (m_registry)
        m_registry->remove(m_key, m_target);
}

QObjectRegistryAttachedType *QObjectRegistry::qmlAttachedProperties(QObject *object)
{
    return new QObjectRegistryAttachedType(object);
}

QString QObjectRegistry::key() const
{
    return m_key;
}

void QObjectRegistry::setKey(const QString &key)
{
    if (m_key == key)
        return;

    if (!m_target) {
        m_key = key;
        emit keyChanged();
        return; // Postpone actual handling until we have both key and target
    }

    if (!m_registry)
        m_registry = QObjectRegistrySingleton::registryForObject(this);

    if (m_registry)
        m_registry->remove(m_key, m_target);

    m_key = key;

    if (m_registry)
        m_registry->add(m_key, m_target);

    emit keyChanged();
}

QObject *QObjectRegistry::target() const
{
    return m_target.object();
}

void QObjectRegistry::setTarget(QObject *target)
{
    if (QQmlData::wasDeleted(target))
        target = nullptr;

    if (m_target == target)
        return;

    if (m_key.isEmpty()) {
        m_target.setObject(target);
        emit targetChanged();
        return; // Postpone actual handling until we have both key and target
    }

    if (!m_registry)
        m_registry = QObjectRegistrySingleton::registryForObject(this);

    if (m_registry)
        m_registry->remove(m_key, m_target);

    m_target.setObject(target);

    if (m_registry)
        m_registry->add(m_key, m_target);

    emit targetChanged();
}

QT_END_NAMESPACE

