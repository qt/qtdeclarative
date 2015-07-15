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

#include "qlocalclientconnection.h"
#include "qpacketprotocol.h"
#include "qqmldebugserver.h"

#include <QtCore/qplugin.h>
#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE

class QLocalClientConnectionPrivate {
public:
    QLocalClientConnectionPrivate();

    bool block;
    QString filename;
    QLocalSocket *socket;
    QQmlDebugServer *debugServer;
};

QLocalClientConnectionPrivate::QLocalClientConnectionPrivate() :
    block(false),
    socket(0),
    debugServer(0)
{
}

QLocalClientConnection::QLocalClientConnection() :
    d_ptr(new QLocalClientConnectionPrivate)
{
}

QLocalClientConnection::~QLocalClientConnection()
{
    if (isConnected())
        disconnect();
    delete d_ptr;
}

void QLocalClientConnection::setServer(QQmlDebugServer *server)
{
    Q_D(QLocalClientConnection);
    d->debugServer = server;
}

bool QLocalClientConnection::isConnected() const
{
    Q_D(const QLocalClientConnection);
    return d->socket && d->socket->state() == QLocalSocket::ConnectedState;
}

void QLocalClientConnection::disconnect()
{
    Q_D(QLocalClientConnection);

    while (d->socket && d->socket->bytesToWrite() > 0)
        d->socket->waitForBytesWritten();

    d->socket->deleteLater();
    d->socket = 0;
}

bool QLocalClientConnection::setPortRange(int portFrom, int portTo, bool block,
                                        const QString &hostaddress)
{
    Q_UNUSED(portFrom);
    Q_UNUSED(portTo);
    Q_UNUSED(block);
    Q_UNUSED(hostaddress);
    return false;
}

bool QLocalClientConnection::setFileName(const QString &filename, bool block)
{
    Q_D(QLocalClientConnection);
    d->filename = filename;
    d->block = block;
    return connectToServer();
}

void QLocalClientConnection::waitForConnection()
{
    Q_D(QLocalClientConnection);
    d->socket->waitForConnected(-1);
}

bool QLocalClientConnection::connectToServer()
{
    Q_D(QLocalClientConnection);

    d->socket = new QLocalSocket;
    d->socket->setParent(this);
    QObject::connect(d->socket, SIGNAL(connected()), this, SLOT(connectionEstablished()));
    d->socket->connectToServer(d->filename);
    qDebug("QML Debugger: Connecting to socket %s...",
           d->filename.toLatin1().constData());
    return true;
}

void QLocalClientConnection::flush()
{
    Q_D(QLocalClientConnection);
    if (d->socket)
        d->socket->flush();
}

void QLocalClientConnection::connectionEstablished()
{
    Q_D(QLocalClientConnection);
    d->debugServer->setDevice(d->socket);
}

QQmlDebugServerConnection *QLocalClientConnectionFactory::create(const QString &key)
{
    return (key == QLatin1String("QLocalClientConnection") ? new QLocalClientConnection : 0);
}

QT_END_NAMESPACE
