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

#include "qlocalclientconnectionfactory.h"
#include "qpacketprotocol.h"
#include "qqmldebugserver.h"

#include <QtCore/qplugin.h>
#include <QtNetwork/qlocalsocket.h>

QT_BEGIN_NAMESPACE


class QLocalClientConnection : public QQmlDebugServerConnection
{
    Q_OBJECT
    Q_DISABLE_COPY(QLocalClientConnection)

public:
    QLocalClientConnection();
    ~QLocalClientConnection();

    void setServer(QQmlDebugServer *server);
    bool setPortRange(int portFrom, int portTo, bool block, const QString &hostaddress);
    bool setFileName(const QString &filename, bool block);

    bool isConnected() const;
    void disconnect();

    void waitForConnection();
    void flush();

private slots:
    void connectionEstablished();

private:
    bool connectToServer();

    bool m_block;
    QString m_filename;
    QLocalSocket *m_socket;
    QQmlDebugServer *m_debugServer;
};

QLocalClientConnection::QLocalClientConnection() :
    m_block(false),
    m_socket(0),
    m_debugServer(0)
{
}

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
    m_socket = 0;
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
    QObject::connect(m_socket, SIGNAL(connected()), this, SLOT(connectionEstablished()));
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

QQmlDebugServerConnection *QLocalClientConnectionFactory::create(const QString &key)
{
    return (key == QLatin1String("QLocalClientConnection") ? new QLocalClientConnection : 0);
}

QT_END_NAMESPACE

#include "qlocalclientconnection.moc"
