// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmldebugserverconnection_p.h>
#include <private/qqmldebugserver_p.h>

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
    ~QTcpServerConnection() override;

    void setServer(QQmlDebugServer *server) override;
    bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress) override;
    bool setFileName(const QString &fileName, bool block) override;

    bool isConnected() const override;
    void disconnect() override;

    void waitForConnection() override;
    void flush() override;

private:
    void newConnection();
    bool listen();

    int m_portFrom = 0;
    int m_portTo = 0;
    bool m_block = false;
    QString m_hostaddress;
    QTcpSocket *m_socket = nullptr;
    QTcpServer *m_tcpServer = nullptr;
    QQmlDebugServer *m_debugServer = nullptr;
};

QTcpServerConnection::QTcpServerConnection() {}

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
    m_socket = nullptr;
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
    QObject::connect(m_tcpServer, &QTcpServer::newConnection,
                     this, &QTcpServerConnection::newConnection);
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


class QTcpServerConnectionFactory : public QQmlDebugServerConnectionFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServerConnectionFactory_iid FILE "qtcpserverconnection.json")
    Q_INTERFACES(QQmlDebugServerConnectionFactory)
public:
    QQmlDebugServerConnection *create(const QString &key) override;
};

QQmlDebugServerConnection *QTcpServerConnectionFactory::create(const QString &key)
{
    return (key == QLatin1String("QTcpServerConnection") ? new QTcpServerConnection : nullptr);
}

QT_END_NAMESPACE

#include "qtcpserverconnection.moc"
