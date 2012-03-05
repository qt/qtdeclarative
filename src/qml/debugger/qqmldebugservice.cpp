/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldebugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include "qqmldebugserver_p.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

QQmlDebugServicePrivate::QQmlDebugServicePrivate()
    : server(0)
{
}

QQmlDebugService::QQmlDebugService(const QString &name, float version, QObject *parent)
    : QObject(*(new QQmlDebugServicePrivate), parent)
{
    Q_D(QQmlDebugService);
    d->name = name;
    d->version = version;
    d->server = QQmlDebugServer::instance();
    d->state = QQmlDebugService::NotConnected;


}

QQmlDebugService::QQmlDebugService(QQmlDebugServicePrivate &dd,
                                                   const QString &name, float version, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QQmlDebugService);
    d->name = name;
    d->version = version;
    d->server = QQmlDebugServer::instance();
    d->state = QQmlDebugService::NotConnected;
}

/**
  Registers the service. This should be called in the constructor of the inherited class. From
  then on the service might get asynchronous calls to messageReceived().
  */
QQmlDebugService::State QQmlDebugService::registerService()
{
    Q_D(QQmlDebugService);
    if (!d->server)
        return NotConnected;

    if (d->server->serviceNames().contains(d->name)) {
        qWarning() << "QQmlDebugService: Conflicting plugin name" << d->name;
        d->server = 0;
    } else {
        d->server->addService(this);
    }
    return state();
}

QQmlDebugService::~QQmlDebugService()
{
    Q_D(const QQmlDebugService);
    if (d->server) {
        d->server->removeService(this);
    }
}

QString QQmlDebugService::name() const
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

namespace {

struct ObjectReference
{
    QPointer<QObject> object;
    int id;
};

struct ObjectReferenceHash
{
    ObjectReferenceHash() : nextId(0) {}

    QHash<QObject *, ObjectReference> objects;
    QHash<int, QObject *> ids;

    int nextId;
};

}
Q_GLOBAL_STATIC(ObjectReferenceHash, objectReferenceHash);


/*!
    Returns a unique id for \a object.  Calling this method multiple times
    for the same object will return the same id.
*/
int QQmlDebugService::idForObject(QObject *object)
{
    if (!object)
        return -1;

    ObjectReferenceHash *hash = objectReferenceHash();
    QHash<QObject *, ObjectReference>::Iterator iter =
            hash->objects.find(object);

    if (iter == hash->objects.end()) {
        int id = hash->nextId++;

        hash->ids.insert(id, object);
        iter = hash->objects.insert(object, ObjectReference());
        iter->object = object;
        iter->id = id;
    } else if (iter->object != object) {
        int id = hash->nextId++;

        hash->ids.remove(iter->id);

        hash->ids.insert(id, object);
        iter->object = object;
        iter->id = id;
    }
    return iter->id;
}

/*!
    Returns the object for unique \a id.  If the object has not previously been
    assigned an id, through idForObject(), then 0 is returned.  If the object
    has been destroyed, 0 is returned.
*/
QObject *QQmlDebugService::objectForId(int id)
{
    ObjectReferenceHash *hash = objectReferenceHash();

    QHash<int, QObject *>::Iterator iter = hash->ids.find(id);
    if (iter == hash->ids.end())
        return 0;


    QHash<QObject *, ObjectReference>::Iterator objIter =
            hash->objects.find(*iter);
    Q_ASSERT(objIter != hash->objects.end());

    if (objIter->object == 0) {
        hash->ids.erase(iter);
        hash->objects.erase(objIter);
        return 0;
    } else {
        return *iter;
    }
}

bool QQmlDebugService::isDebuggingEnabled()
{
    return QQmlDebugServer::instance() != 0;
}

bool QQmlDebugService::hasDebuggingClient()
{
    return QQmlDebugServer::instance() != 0
            && QQmlDebugServer::instance()->hasDebuggingClient();
}

QString QQmlDebugService::objectToString(QObject *obj)
{
    if(!obj)
        return QLatin1String("NULL");

    QString objectName = obj->objectName();
    if(objectName.isEmpty())
        objectName = QLatin1String("<unnamed>");

    QString rv = QString::fromUtf8(obj->metaObject()->className()) +
            QLatin1String(": ") + objectName;

    return rv;
}

void QQmlDebugService::sendMessage(const QByteArray &message)
{
    sendMessages(QList<QByteArray>() << message);
}

void QQmlDebugService::sendMessages(const QList<QByteArray> &messages)
{
    Q_D(QQmlDebugService);

    if (state() != Enabled)
        return;

    d->server->sendMessages(this, messages);
}

bool QQmlDebugService::waitForMessage()
{
    Q_D(QQmlDebugService);

    if (state() != Enabled)
        return false;

    return d->server->waitForMessage(this);
}

void QQmlDebugService::stateAboutToBeChanged(State)
{
}

void QQmlDebugService::stateChanged(State)
{
}

void QQmlDebugService::messageReceived(const QByteArray &)
{
}

QT_END_NAMESPACE
