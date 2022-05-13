// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugconnection_p.h"
#include "qqmldebugclient_p.h"

#include <private/qpacketprotocol_p.h>
#include <private/qpacket_p.h>
#include <private/qobject_p.h>

#include <QtCore/qeventloop.h>
#include <QtCore/qtimer.h>
#include <QtCore/QHash>
#include <QtCore/qdatastream.h>
#include <QtNetwork/qlocalserver.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtNetwork/qtcpsocket.h>

QT_BEGIN_NAMESPACE

static const int protocolVersion = 1;
static const QString serverId = QLatin1String("QDeclarativeDebugServer");
static const QString clientId = QLatin1String("QDeclarativeDebugClient");

class QQmlDebugConnectionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlDebugConnection)

public:
    QQmlDebugConnectionPrivate();
    QPacketProtocol *protocol = nullptr;
    QIODevice *device = nullptr;
    QLocalServer *server = nullptr;
    QEventLoop handshakeEventLoop;
    QTimer handshakeTimer;

    bool gotHello = false;
    int currentDataStreamVersion = QDataStream::Qt_4_7;
    int maximumDataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;
    QHash <QString, float> serverPlugins;
    QHash<QString, QQmlDebugClient *> plugins;
    QStringList removedPlugins;

    void advertisePlugins();
    void createProtocol();
    void flush();
};

QQmlDebugConnectionPrivate::QQmlDebugConnectionPrivate()
{
    handshakeTimer.setSingleShot(true);
    handshakeTimer.setInterval(3000);
}

void QQmlDebugConnectionPrivate::advertisePlugins()
{
    Q_Q(QQmlDebugConnection);
    if (!q->isConnected())
        return;

    QPacket pack(currentDataStreamVersion);
    pack << serverId << 1 << plugins.keys();
    protocol->send(pack.data());
    flush();
}

void QQmlDebugConnection::socketConnected()
{
    Q_D(QQmlDebugConnection);
    QPacket pack(d->currentDataStreamVersion);
    pack << serverId << 0 << protocolVersion << d->plugins.keys() << d->maximumDataStreamVersion
         << true; // We accept multiple messages per packet
    d->protocol->send(pack.data());
    d->flush();
}

void QQmlDebugConnection::socketDisconnected()
{
    Q_D(QQmlDebugConnection);
    d->gotHello = false;
    emit disconnected();
}

void QQmlDebugConnection::protocolReadyRead()
{
    Q_D(QQmlDebugConnection);
    if (!d->gotHello) {
        QPacket pack(d->currentDataStreamVersion, d->protocol->read());
        QString name;

        pack >> name;

        bool validHello = false;
        if (name == clientId) {
            int op = -1;
            pack >> op;
            if (op == 0) {
                int version = -1;
                pack >> version;
                if (version == protocolVersion) {
                    QStringList pluginNames;
                    QList<float> pluginVersions;
                    pack >> pluginNames;
                    if (!pack.atEnd())
                        pack >> pluginVersions;

                    const int pluginNamesSize = pluginNames.size();
                    const int pluginVersionsSize = pluginVersions.size();
                    for (int i = 0; i < pluginNamesSize; ++i) {
                        float pluginVersion = 1.0;
                        if (i < pluginVersionsSize)
                            pluginVersion = pluginVersions.at(i);
                        d->serverPlugins.insert(pluginNames.at(i), pluginVersion);
                    }

                    pack >> d->currentDataStreamVersion;
                    validHello = true;
                }
            }
        }

        if (!validHello) {
            qWarning("QQmlDebugConnection: Invalid hello message");
            close();
            return;
        }
        d->gotHello = true;
        emit connected();

        QHash<QString, QQmlDebugClient *>::Iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter) {
            QQmlDebugClient::State newState = QQmlDebugClient::Unavailable;
            if (d->serverPlugins.contains(iter.key()))
                newState = QQmlDebugClient::Enabled;
            iter.value()->stateChanged(newState);
        }

        d->handshakeTimer.stop();
        d->handshakeEventLoop.quit();
    }

    while (d->protocol->packetsAvailable()) {
        QPacket pack(d->currentDataStreamVersion, d->protocol->read());
        QString name;
        pack >> name;

        if (name == clientId) {
            int op = -1;
            pack >> op;

            if (op == 1) {
                // Service Discovery
                QHash<QString, float> oldServerPlugins = d->serverPlugins;
                d->serverPlugins.clear();

                QStringList pluginNames;
                QList<float> pluginVersions;
                pack >> pluginNames;
                if (!pack.atEnd())
                    pack >> pluginVersions;

                const int pluginNamesSize = pluginNames.size();
                const int pluginVersionsSize = pluginVersions.size();
                for (int i = 0; i < pluginNamesSize; ++i) {
                    float pluginVersion = 1.0;
                    if (i < pluginVersionsSize)
                        pluginVersion = pluginVersions.at(i);
                    d->serverPlugins.insert(pluginNames.at(i), pluginVersion);
                }

                QHash<QString, QQmlDebugClient *>::Iterator iter = d->plugins.begin();
                for (; iter != d->plugins.end(); ++iter) {
                    const QString &pluginName = iter.key();
                    QQmlDebugClient::State newState = QQmlDebugClient::Unavailable;
                    if (d->serverPlugins.contains(pluginName))
                        newState = QQmlDebugClient::Enabled;

                    if (oldServerPlugins.contains(pluginName)
                            != d->serverPlugins.contains(pluginName)) {
                        iter.value()->stateChanged(newState);
                    }
                }
            } else {
                qWarning() << "QQmlDebugConnection: Unknown control message id" << op;
            }
        } else {
            QHash<QString, QQmlDebugClient *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end()) {
                // We can get more messages for plugins we have removed because it takes time to
                // send the advertisement message but the removal is instant locally.
                if (!d->removedPlugins.contains(name)) {
                    qWarning() << "QQmlDebugConnection: Message received for missing plugin"
                               << name;
                }
            } else {
                QQmlDebugClient *client = *iter;
                QByteArray message;
                while (!pack.atEnd()) {
                    pack >> message;
                    client->messageReceived(message);
                }
            }
        }
    }
}

void QQmlDebugConnection::handshakeTimeout()
{
    Q_D(QQmlDebugConnection);
    if (!d->gotHello) {
        qWarning() << "QQmlDebugConnection: Did not get handshake answer in time";
        d->handshakeEventLoop.quit();
    }
}

QQmlDebugConnection::QQmlDebugConnection(QObject *parent) :
    QObject(*(new QQmlDebugConnectionPrivate), parent)
{
    Q_D(QQmlDebugConnection);
    connect(&d->handshakeTimer, &QTimer::timeout, this, &QQmlDebugConnection::handshakeTimeout);
}

QQmlDebugConnection::~QQmlDebugConnection()
{
    Q_D(QQmlDebugConnection);
    QHash<QString, QQmlDebugClient*>::iterator iter = d->plugins.begin();
    for (; iter != d->plugins.end(); ++iter)
        emit iter.value()->stateChanged(QQmlDebugClient::NotConnected);
}

int QQmlDebugConnection::currentDataStreamVersion() const
{
    Q_D(const QQmlDebugConnection);
    return d->currentDataStreamVersion;
}

void QQmlDebugConnection::setMaximumDataStreamVersion(int maximumVersion)
{
    Q_D(QQmlDebugConnection);
    d->maximumDataStreamVersion = maximumVersion;
}

bool QQmlDebugConnection::isConnected() const
{
    Q_D(const QQmlDebugConnection);
    return d->gotHello;
}

bool QQmlDebugConnection::isConnecting() const
{
    Q_D(const QQmlDebugConnection);
    return !d->gotHello && d->device;
}

void QQmlDebugConnection::close()
{
    Q_D(QQmlDebugConnection);
    if (d->gotHello) {
        d->gotHello = false;
        d->device->close();

        QHash<QString, QQmlDebugClient*>::iterator iter = d->plugins.begin();
        for (; iter != d->plugins.end(); ++iter)
            emit iter.value()->stateChanged(QQmlDebugClient::NotConnected);
    }

    if (d->device) {
        d->device->deleteLater();
        d->device = nullptr;
    }
}

bool QQmlDebugConnection::waitForConnected(int msecs)
{
    Q_D(QQmlDebugConnection);
    auto socket = qobject_cast<QAbstractSocket*>(d->device);
    if (!socket) {
        if (!d->server || (!d->server->hasPendingConnections() &&
                           !d->server->waitForNewConnection(msecs)))
            return false;
    } else if (!socket->waitForConnected(msecs)) {
        return false;
    }
    // wait for handshake
    d->handshakeTimer.start();
    d->handshakeEventLoop.exec();
    return d->gotHello;
}

QQmlDebugClient *QQmlDebugConnection::client(const QString &name) const
{
    Q_D(const QQmlDebugConnection);
    return d->plugins.value(name, nullptr);
}

bool QQmlDebugConnection::addClient(const QString &name, QQmlDebugClient *client)
{
    Q_D(QQmlDebugConnection);
    if (d->plugins.contains(name))
        return false;
    d->removedPlugins.removeAll(name);
    d->plugins.insert(name, client);
    d->advertisePlugins();
    return true;
}

bool QQmlDebugConnection::removeClient(const QString &name)
{
    Q_D(QQmlDebugConnection);
    if (!d->plugins.contains(name))
        return false;
    d->plugins.remove(name);
    d->removedPlugins.append(name);
    d->advertisePlugins();
    return true;
}

float QQmlDebugConnection::serviceVersion(const QString &serviceName) const
{
    Q_D(const QQmlDebugConnection);
    return d->serverPlugins.value(serviceName, -1);
}

bool QQmlDebugConnection::sendMessage(const QString &name, const QByteArray &message)
{
    Q_D(QQmlDebugConnection);
    if (!isConnected() || !d->serverPlugins.contains(name))
        return false;

    QPacket pack(d->currentDataStreamVersion);
    pack << name << message;
    d->protocol->send(pack.data());
    d->flush();

    return true;
}

void QQmlDebugConnectionPrivate::flush()
{
    if (auto socket = qobject_cast<QAbstractSocket *>(device))
        socket->flush();
    else if (auto socket = qobject_cast<QLocalSocket *>(device))
        socket->flush();
}

void QQmlDebugConnection::connectToHost(const QString &hostName, quint16 port)
{
    Q_D(QQmlDebugConnection);
    if (d->gotHello)
        close();
    auto socket = new QTcpSocket(this);
    d->device = socket;
    d->createProtocol();
    connect(socket, &QAbstractSocket::disconnected, this, &QQmlDebugConnection::socketDisconnected);
    connect(socket, &QAbstractSocket::connected, this, &QQmlDebugConnection::socketConnected);
    connect(socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                &QAbstractSocket::errorOccurred), this, &QQmlDebugConnection::socketError);
    connect(socket, &QAbstractSocket::stateChanged, this, &QQmlDebugConnection::socketStateChanged);
    socket->connectToHost(hostName, port);
}

void QQmlDebugConnection::startLocalServer(const QString &fileName)
{
    Q_D(QQmlDebugConnection);
    if (d->gotHello)
        close();
    if (d->server)
        d->server->deleteLater();
    d->server = new QLocalServer(this);
    // QueuedConnection so that waitForNewConnection() returns true.
    connect(d->server, &QLocalServer::newConnection,
            this, &QQmlDebugConnection::newConnection, Qt::QueuedConnection);
    d->server->listen(fileName);
}

class LocalSocketSignalTranslator : public QObject
{
    Q_OBJECT
public:
    LocalSocketSignalTranslator(QLocalSocket *parent) : QObject(parent)
    {
        connect(parent, &QLocalSocket::stateChanged,
                this, &LocalSocketSignalTranslator::onStateChanged);
        connect(parent, &QLocalSocket::errorOccurred, this, &LocalSocketSignalTranslator::onError);
    }

    void onError(QLocalSocket::LocalSocketError error)
    {
        emit socketError(static_cast<QAbstractSocket::SocketError>(error));
    }

    void onStateChanged(QLocalSocket::LocalSocketState state)
    {
        emit socketStateChanged(static_cast<QAbstractSocket::SocketState>(state));
    }

signals:
    void socketError(QAbstractSocket::SocketError);
    void socketStateChanged(QAbstractSocket::SocketState);
};

void QQmlDebugConnection::newConnection()
{
    Q_D(QQmlDebugConnection);
    delete d->device;
    QLocalSocket *socket = d->server->nextPendingConnection();
    d->server->close();
    d->device = socket;
    d->createProtocol();
    connect(socket, &QLocalSocket::disconnected, this, &QQmlDebugConnection::socketDisconnected);
    auto translator = new LocalSocketSignalTranslator(socket);

    connect(translator, &LocalSocketSignalTranslator::socketError,
            this, &QQmlDebugConnection::socketError);
    connect(translator, &LocalSocketSignalTranslator::socketStateChanged,
            this, &QQmlDebugConnection::socketStateChanged);
    socketConnected();
}

void QQmlDebugConnectionPrivate::createProtocol()
{
    Q_Q(QQmlDebugConnection);
    delete protocol;
    protocol = new QPacketProtocol(device, q);
    QObject::connect(protocol, &QPacketProtocol::readyRead,
                     q, &QQmlDebugConnection::protocolReadyRead);
}

QT_END_NAMESPACE

#include <qqmldebugconnection.moc>

#include "moc_qqmldebugconnection_p.cpp"
