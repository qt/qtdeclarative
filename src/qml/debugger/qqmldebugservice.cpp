// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugservice_p.h"
#include "qqmldebugconnector_p.h"
#include <private/qqmldata_p.h>
#include <private/qqmlcontext_p.h>

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

class QQmlDebugServer;

class QQmlDebugServicePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlDebugService)
public:
    QQmlDebugServicePrivate(const QString &name, float version);

    const QString name;
    const float version;
    QQmlDebugService::State state;
};

QQmlDebugServicePrivate::QQmlDebugServicePrivate(const QString &name, float version) :
    name(name), version(version), state(QQmlDebugService::NotConnected)
{
}

QQmlDebugService::QQmlDebugService(const QString &name, float version, QObject *parent)
    : QObject(*(new QQmlDebugServicePrivate(name, version)), parent)
{
    Q_D(QQmlDebugService);
    QQmlDebugConnector *server = QQmlDebugConnector::instance();

    if (!server)
        return;

    if (server->service(d->name)) {
        qWarning() << "QQmlDebugService: Conflicting plugin name" << d->name;
    } else {
        server->addService(d->name, this);
    }
}

QQmlDebugService::~QQmlDebugService()
{
    Q_D(QQmlDebugService);
    QQmlDebugConnector *server = QQmlDebugConnector::instance();

    if (!server)
        return;

    if (server->service(d->name) != this)
        qWarning() << "QQmlDebugService: Plugin" << d->name << "is not registered.";
    else
        server->removeService(d->name);
}

const QString &QQmlDebugService::name() const
{
    Q_D(const QQmlDebugService);
    return d->name;
}

float QQmlDebugService::version() const
{
    Q_D(const QQmlDebugService);
    return d->version;
}

QQmlDebugService::State QQmlDebugService::state() const
{
    Q_D(const QQmlDebugService);
    return d->state;
}

void QQmlDebugService::setState(QQmlDebugService::State newState)
{
    Q_D(QQmlDebugService);
    d->state = newState;
}

namespace {
class ObjectReferenceHash : public QObject
{
    Q_OBJECT
public:
    ObjectReferenceHash() : nextId(0) {}

    QHash<QObject *, int> objects;
    QHash<int, QObject *> ids;

    int nextId;

    void remove(QObject *obj);
};
}
Q_GLOBAL_STATIC(ObjectReferenceHash, objectReferenceHash)

void ObjectReferenceHash::remove(QObject *obj)
{
    const auto iter = objects.constFind(obj);
    if (iter != objects.cend()) {
        ids.remove(iter.value());
        objects.erase(iter);
    }
}

/*!
    Returns a unique id for \a object.  Calling this method multiple times
    for the same object will return the same id.
*/
int QQmlDebugService::idForObject(QObject *object)
{
    if (!object)
        return -1;

    ObjectReferenceHash *hash = objectReferenceHash();
    auto iter = hash->objects.constFind(object);

    if (iter == hash->objects.cend()) {
        int id = hash->nextId++;
        hash->ids.insert(id, object);
        iter = hash->objects.insert(object, id);
        connect(object, &QObject::destroyed, hash, &ObjectReferenceHash::remove);
    }
    return iter.value();
}

/*!
    Returns the mapping of objects to unique \a ids, created through calls to idForObject().
*/
const QHash<int, QObject *> &QQmlDebugService::objectsForIds()
{
    return objectReferenceHash()->ids;
}

QT_END_NAMESPACE

#include "qqmldebugservice.moc"
#include "moc_qqmldebugservice_p.cpp"
