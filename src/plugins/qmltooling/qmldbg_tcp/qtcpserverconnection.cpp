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

#include "qtcpserverconnection.h"
#include "qqmldebugserver.h"

#include <QtCore/qplugin.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_NAMESPACE

class QTcpServerConnectionPrivate {
public:
    QTcpServerConnectionPrivate();

    int portFrom;
    int portTo;
    bool block;
    QString hostaddress;
    QTcpSocket *socket;
    QTcpServer *tcpServer;

    QQmlDebugServer *debugServer;
};

QTcpServerConnectionPrivate::QTcpServerConnectionPrivate() :
    portFrom(0),
    portTo(0),
    block(false),
    socket(0),
    tcpServer(0),
    debugServer(0)
{
}

QTcpServerConnection::QTcpServerConnection() :
    d_ptr(new QTcpServerConnectionPrivate)
{

}

QTcpServerConnection::~QTcpServerConnection()
{
    if (isConnected())
        disconnect();
    delete d_ptr;
}

void QTcpServerConnection::setServer(QQmlDebugServer *server)
{
    Q_D(QTcpServerConnection);
    d->debugServer = server;
}

bool QTcpServerConnection::isConnected() const
{
    Q_D(const QTcpServerConnection);
    return d->socket && d->socket->state() == QTcpSocket::ConnectedState;
}

void QTcpServerConnection::disconnect()
{
    Q_D(QTcpServerConnection);

    while (d->socket && d->socket->bytesToWrite() > 0) {
        if (!d->socket->waitForBytesWritten()) {
            qWarning("QML Debugger: Failed to send remaining %lld bytes on disconnect.",
                     d->socket->bytesToWrite());
            break;
        }
    }

    d->socket->deleteLater();
    d->socket = 0;
}

bool QTcpServerConnection::setPortRange(int portFrom, int portTo, bool block,
                                        const QString &hostaddress)
{
    Q_D(QTcpServerConnection);
    d->portFrom = portFrom;
    d->portTo = portTo;
    d->block = block;
    d->hostaddress = hostaddress;

    return listen();
}

bool QTcpServerConnection::setFileName(const QString &fileName, bool block)
{
    Q_UNUSED(fileName);
    Q_UNUSED(block);
    return false;
}

void QTcpServerConnection::waitForConnection()
{
    Q_D(QTcpServerConnection);
    d->tcpServer->waitForNewConnection(-1);
}

void QTcpServerConnection::flush()
{
    Q_D(QTcpServerConnection);
    if (d->socket)
        d->socket->flush();
}

bool QTcpServerConnection::listen()
{
    Q_D(QTcpServerConnection);

    d->tcpServer = new QTcpServer(this);
    QObject::connect(d->tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    QHostAddress hostaddress;
    if (!d->hostaddress.isEmpty()) {
        if (!hostaddress.setAddress(d->hostaddress)) {
            hostaddress = QHostAddress::Any;
            qDebug("QML Debugger: Incorrect host address provided. So accepting connections "
                     "from any host.");
        }
    } else {
        hostaddress = QHostAddress::Any;
    }
    int port = d->portFrom;
    do {
        if (d->tcpServer->listen(hostaddress, port)) {
            qDebug("QML Debugger: Waiting for connection on port %d...", port);
            break;
        }
        ++port;
    } while (port <= d->portTo);
    if (port > d->portTo) {
        if (d->portFrom == d->portTo)
            qWarning("QML Debugger: Unable to listen to port %d.", d->portFrom);
        else
            qWarning("QML Debugger: Unable to listen to ports %d - %d.", d->portFrom, d->portTo);
        return false;
    } else {
        return true;
    }
}

void QTcpServerConnection::newConnection()
{
    Q_D(QTcpServerConnection);

    if (d->socket && d->socket->peerPort()) {
        qWarning("QML Debugger: Another client is already connected.");
        QTcpSocket *faultyConnection = d->tcpServer->nextPendingConnection();
        delete faultyConnection;
        return;
    }

    delete d->socket;
    d->socket = d->tcpServer->nextPendingConnection();
    d->socket->setParent(this);
    d->debugServer->setDevice(d->socket);
}

QT_END_NAMESPACE
