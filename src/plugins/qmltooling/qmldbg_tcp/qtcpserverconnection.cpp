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

#include "qtcpserverconnectionfactory.h"
#include "qqmldebugserver.h"

#include <QtCore/qplugin.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_NAMESPACE

class QTcpServerConnection : public QQmlDebugServerConnection
{
    Q_OBJECT
    Q_DISABLE_COPY(QTcpServerConnection)

public:
    QTcpServerConnection();
    ~QTcpServerConnection();

    void setServer(QQmlDebugServer *server);
    bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress);
    bool setFileName(const QString &fileName, bool block);

    bool isConnected() const;
    void disconnect();

    void waitForConnection();
    void flush();

private slots:
    void newConnection();

private:
    bool listen();

    int m_portFrom;
    int m_portTo;
    bool m_block;
    QString m_hostaddress;
    QTcpSocket *m_socket;
    QTcpServer *m_tcpServer;
    QQmlDebugServer *m_debugServer;
};

QTcpServerConnection::QTcpServerConnection() :
    m_portFrom(0),
    m_portTo(0),
    m_block(false),
    m_socket(0),
    m_tcpServer(0),
    m_debugServer(0)
{
}

QTcpServerConnection::~QTcpServerConnection()
{
    if (isConnected())
        disconnect();
}

void QTcpServerConnection::setServer(QQmlDebugServer *server)
{
    m_debugServer = server;
}

bool QTcpServerConnection::isConnected() const
{
    return m_socket && m_socket->state() == QTcpSocket::ConnectedState;
}

void QTcpServerConnection::disconnect()
{
    while (m_socket && m_socket->bytesToWrite() > 0) {
        if (!m_socket->waitForBytesWritten()) {
            qWarning("QML Debugger: Failed to send remaining %lld bytes on disconnect.",
                     m_socket->bytesToWrite());
            break;
        }
    }

    m_socket->deleteLater();
    m_socket = 0;
}

bool QTcpServerConnection::setPortRange(int portFrom, int portTo, bool block,
                                        const QString &hostaddress)
{
    m_portFrom = portFrom;
    m_portTo = portTo;
    m_block = block;
    m_hostaddress = hostaddress;

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
    m_tcpServer->waitForNewConnection(-1);
}

void QTcpServerConnection::flush()
{
    if (m_socket)
        m_socket->flush();
}

bool QTcpServerConnection::listen()
{
    m_tcpServer = new QTcpServer(this);
    QObject::connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    QHostAddress hostaddress;
    if (!m_hostaddress.isEmpty()) {
        if (!hostaddress.setAddress(m_hostaddress)) {
            hostaddress = QHostAddress::Any;
            qDebug("QML Debugger: Incorrect host address provided. So accepting connections "
                     "from any host.");
        }
    } else {
        hostaddress = QHostAddress::Any;
    }
    int port = m_portFrom;
    do {
        if (m_tcpServer->listen(hostaddress, port)) {
            qDebug("QML Debugger: Waiting for connection on port %d...", port);
            break;
        }
        ++port;
    } while (port <= m_portTo);
    if (port > m_portTo) {
        if (m_portFrom == m_portTo)
            qWarning("QML Debugger: Unable to listen to port %d.", m_portFrom);
        else
            qWarning("QML Debugger: Unable to listen to ports %d - %d.", m_portFrom, m_portTo);
        return false;
    } else {
        return true;
    }
}

void QTcpServerConnection::newConnection()
{
    if (m_socket && m_socket->peerPort()) {
        qWarning("QML Debugger: Another client is already connected.");
        QTcpSocket *faultyConnection = m_tcpServer->nextPendingConnection();
        delete faultyConnection;
        return;
    }

    delete m_socket;
    m_socket = m_tcpServer->nextPendingConnection();
    m_socket->setParent(this);
    m_debugServer->setDevice(m_socket);
}

QQmlDebugServerConnection *QTcpServerConnectionFactory::create(const QString &key)
{
    return (key == QLatin1String("QTcpServerConnection") ? new QTcpServerConnection : 0);
}

QT_END_NAMESPACE

#include "qtcpserverconnection.moc"
