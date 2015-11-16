/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        connection = 0;
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

void QQmlDebugClient::stateChanged(QQmlDebugClient::State state)
{
    Q_UNUSED(state);
}

void QQmlDebugClient::messageReceived(const QByteArray &message)
{
    Q_UNUSED(message);
}

QT_END_NAMESPACE
