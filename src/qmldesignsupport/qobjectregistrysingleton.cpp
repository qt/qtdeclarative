// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qobjectregistrysingleton_p.h"

#include <private/qabstractobjectregistryref_p.h>
#include <private/qqmldata_p.h>

#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

QObjectRegistrySingleton::QObjectRegistrySingleton(QObject *parent)
    : QObject(parent)
{
}

QObjectRegistrySingleton::~QObjectRegistrySingleton()
{
    for (const auto &guards : std::as_const(m_objects))
        qDeleteAll(guards);
}

void QObjectRegistrySingleton::add(const QString &key, QObject *obj)
{
    if (key.isEmpty() || !obj)
        return;

    if (QQmlData::wasDeleted(obj))
        return;

    auto &guards = m_objects[key];
    if (guards.contains(obj))
        return;

    guards.insert(obj, new ObjectGuard(this, key, obj));

    notifyRefs(key, obj, Notification::ObjectAdded);
}

void QObjectRegistrySingleton::remove(const QString &key, QObject *obj)
{
    if (key.isEmpty() || !obj)
        return;

    const auto keyIt = m_objects.find(key);
    if (keyIt == m_objects.end())
        return;

    const std::unique_ptr<ObjectGuard> guard(keyIt->take(obj));
    if (!guard)
        return;

    if (keyIt->isEmpty())
        m_objects.erase(keyIt);

    notifyRefs(key, obj, Notification::ObjectRemoved);
}

void QObjectRegistrySingleton::notifyRefs(const QString &key, QObject *obj,
                                          Notification notification)
{
    // Guard against add/remove handlers changing/removing existing references by iterating over
    // a copy of references and checking each reference is still registered before each notification
    const auto refs = m_refs.value(key);
    for (const auto &ref : refs) {
        const auto keyIt = m_refs.constFind(key);
        if (keyIt == m_refs.cend() || !keyIt->contains(ref))
            continue;

        if (notification == Notification::ObjectAdded)
            ref->handleObjectAdded(obj);
        else
            ref->handleObjectRemoved(obj);
    }
}

QSet<QObject*> QObjectRegistrySingleton::objects(const QString &key) const
{
    const auto keyIt = m_objects.constFind(key);
    if (keyIt == m_objects.cend())
        return {};

    return QSet<QObject *>(keyIt->keyBegin(), keyIt->keyEnd());
}

void QObjectRegistrySingleton::registerRef(QAbstractObjectRegistryRefPrivate *ref)
{
    if (!ref)
        return;

    m_refs[ref->key()].insert(ref);
}

void QObjectRegistrySingleton::deregisterRef(QAbstractObjectRegistryRefPrivate *ref)
{
    if (!ref)
        return;

    if (m_refs.contains(ref->key())) {
        m_refs[ref->key()].remove(ref);
        if (m_refs[ref->key()].isEmpty())
            m_refs.remove(ref->key());
    }
}

QObjectRegistrySingleton *QObjectRegistrySingleton::registryForObject(QObject *obj)
{
    return registryForEngine(qmlEngine(obj));
}

QObjectRegistrySingleton *QObjectRegistrySingleton::registryForEngine(QQmlEngine *engine)
{
    if (engine) {
        static int typeId = qmlTypeId("QtQml.DesignSupport",
                                      QT_VERSION_MAJOR,
                                      QT_VERSION_MINOR,
                                      "InternalObjectRegistry");

        return engine->singletonInstance<QObjectRegistrySingleton *>(typeId);
    }
    return nullptr;
}

QT_END_NAMESPACE
