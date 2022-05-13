// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugclient_p_p.h"
#include "qqmldebugconnection_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

QQmlDebugClient::QQmlDebugClient(const QString &name, QQmlDebugConnection *parent) :
    QObject(*(new QQmlDebugClientPrivate(name, parent)), parent)
{
    Q_D(QQmlDebugClient);
    d->addToConnection();
}

QQmlDebugClient::QQmlDebugClient(QQmlDebugClientPrivate &dd) :
    QObject(dd, dd.connection.data())
{
    Q_D(QQmlDebugClient);
    d->addToConnection();
}

QQmlDebugClient::~QQmlDebugClient()
{
    Q_D(QQmlDebugClient);
    if (d->connection && !d->connection->removeClient(d->name))
        qWarning() << "QQmlDebugClient: Plugin not registered" << d->name;
}

QQmlDebugClientPrivate::QQmlDebugClientPrivate(const QString &name,
                                               QQmlDebugConnection *connection) :
    name(name), connection(connection)
{
}

void QQmlDebugClientPrivate::addToConnection()
{
    Q_Q(QQmlDebugClient);
    if (connection && !connection->addClient(name, q)) {
        qWarning() << "QQmlDebugClient: Conflicting plugin name" << name;
        connection = nullptr;
    }
}

QString QQmlDebugClient::name() const
{
    Q_D(const QQmlDebugClient);
    return d->name;
}

float QQmlDebugClient::serviceVersion() const
{
    Q_D(const QQmlDebugClient);
    return d->connection->serviceVersion(d->name);
}

QQmlDebugClient::State QQmlDebugClient::state() const
{
    Q_D(const QQmlDebugClient);
    if (!d->connection || !d->connection->isConnected())
        return NotConnected;

    if (d->connection->serviceVersion(d->name) != -1)
        return Enabled;

    return Unavailable;
}

void QQmlDebugClient::sendMessage(const QByteArray &message)
{
    Q_D(QQmlDebugClient);
    d->connection->sendMessage(d->name, message);
}

QQmlDebugConnection *QQmlDebugClient::connection() const
{
    Q_D(const QQmlDebugClient);
    return d->connection;
}

void QQmlDebugClient::messageReceived(const QByteArray &message)
{
    Q_UNUSED(message);
}

QT_END_NAMESPACE

#include "moc_qqmldebugclient_p.cpp"
