// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmldebugserverconnection_p.h>

#include <QtCore/qplugin.h>
#include <QtNetwork/qlocalsocket.h>
#include <private/qqmldebugserver_p.h>

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)

QT_BEGIN_NAMESPACE


class QLocalClientConnection : public QQmlDebugServerConnection
{
    Q_OBJECT
    Q_DISABLE_COPY(QLocalClientConnection)

public:
    QLocalClientConnection();
    ~QLocalClientConnection() override;

    void setServer(QQmlDebugServer *server) override;
    bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress) override;
    bool setFileName(const QString &filename, bool block) override;

    bool isConnected() const override;
    void disconnect() override;

    void waitForConnection() override;
    void flush() override;

private:
    void connectionEstablished();
    bool connectToServer();

    bool m_block = false;
    QString m_filename;
    QLocalSocket *m_socket = nullptr;
    QQmlDebugServer *m_debugServer = nullptr;
};

QLocalClientConnection::QLocalClientConnection() { }

QLocalClientConnection::~QLocalClientConnection()
{
    if (isConnected())
        disconnect();
}

void QLocalClientConnection::setServer(QQmlDebugServer *server)
{
    m_debugServer = server;
}

bool QLocalClientConnection::isConnected() const
{
    return m_socket && m_socket->state() == QLocalSocket::ConnectedState;
}

void QLocalClientConnection::disconnect()
{
    while (m_socket && m_socket->bytesToWrite() > 0)
        m_socket->waitForBytesWritten();

    m_socket->deleteLater();
    m_socket = nullptr;
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
    m_filename = filename;
    m_block = block;
    return connectToServer();
}

void QLocalClientConnection::waitForConnection()
{
    m_socket->waitForConnected(-1);
}

bool QLocalClientConnection::connectToServer()
{
    m_socket = new QLocalSocket;
    m_socket->setParent(this);
    connect(m_socket, &QLocalSocket::connected,
            this, &QLocalClientConnection::connectionEstablished);
    connect(m_socket, static_cast<void(QLocalSocket::*)(QLocalSocket::LocalSocketError)>(
                &QLocalSocket::errorOccurred), m_socket, [this](QLocalSocket::LocalSocketError) {
        m_socket->disconnectFromServer();
        m_socket->connectToServer(m_filename);
    }, Qt::QueuedConnection);

    m_socket->connectToServer(m_filename);
    qDebug("QML Debugger: Connecting to socket %s...", m_filename.toLatin1().constData());
    return true;
}

void QLocalClientConnection::flush()
{
    if (m_socket)
        m_socket->flush();
}

void QLocalClientConnection::connectionEstablished()
{
    m_debugServer->setDevice(m_socket);
}

class QLocalClientConnectionFactory : public QQmlDebugServerConnectionFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugServerConnectionFactory_iid FILE "qlocalclientconnection.json")
    Q_INTERFACES(QQmlDebugServerConnectionFactory)
public:
    QQmlDebugServerConnection *create(const QString &key) override;
};

QQmlDebugServerConnection *QLocalClientConnectionFactory::create(const QString &key)
{
    return (key == QLatin1String("QLocalClientConnection") ? new QLocalClientConnection : nullptr);
}

QT_END_NAMESPACE

#include "qlocalclientconnection.moc"
