// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QOBJECTREGISTRYSINGLETON_P_H
#define QOBJECTREGISTRYSINGLETON_P_H

#include <QtQmlDesignSupport/qtqmldesignsupportexports.h>

#include <QtCore/qhash.h>
#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtQml/qqml.h>
#include <QtQml/private/qqmlguard_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QAbstractObjectRegistryRefPrivate;
class QQmlEngine;

class Q_QMLDESIGNSUPPORT_EXPORT QObjectRegistrySingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(InternalObjectRegistry)

public:
    explicit QObjectRegistrySingleton(QObject *parent = nullptr);
    ~QObjectRegistrySingleton();

    void registerRef(QAbstractObjectRegistryRefPrivate *ref);
    void deregisterRef(QAbstractObjectRegistryRefPrivate *ref);

    void add(const QString &key, QObject *obj);
    void remove(const QString &key, QObject *obj);
    QSet<QObject*> objects(const QString &key) const;

    static QObjectRegistrySingleton *registryForObject(QObject *obj);
    static QObjectRegistrySingleton *registryForEngine(QQmlEngine *engine);

private:
    enum class Notification { ObjectAdded, ObjectRemoved };

    void notifyRefs(const QString &key, QObject *obj, Notification notification);

    class ObjectGuard : public QQmlGuard<QObject>
    {
    public:
        ObjectGuard(QObjectRegistrySingleton *owner, const QString &key, QObject *obj)
            : QQmlGuard<QObject>(&ObjectGuard::objectDestroyedImpl, obj)
            , m_owner(owner)
            , m_key(key)
            , m_object(obj)
        {
        }

    private:
        static void objectDestroyedImpl(QQmlGuardImpl *guard)
        {
            ObjectGuard *self = static_cast<ObjectGuard *>(guard);
            QObjectRegistrySingleton *owner = self->m_owner;
            const QString key = self->m_key;
            QObject *obj = self->m_object;

            owner->remove(key, obj);
        }

        QObjectRegistrySingleton *m_owner = nullptr;
        QString m_key;
        QObject *m_object = nullptr;
    };

    QHash<QString, QHash<QObject *, ObjectGuard *>> m_objects;
    QHash<QString, QSet<QAbstractObjectRegistryRefPrivate *>> m_refs;
};

QT_END_NAMESPACE

#endif // QOBJECTREGISTRYSINGLETON_P_H
