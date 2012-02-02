/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "qdeclarativedebugserver_p.h"
#include "qdeclarativedebugservice_p.h"
#include "qdeclarativedebugservice_p_p.h"
#include <private/qdeclarativeengine_p.h>

#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QStringList>
#include <QtCore/qwaitcondition.h>

#include <private/qobject_p.h>
#include <private/qcoreapplication_p.h>

QT_BEGIN_NAMESPACE

/*
  QDeclarativeDebug Protocol (Version 1):

  handshake:
    1. Client sends
         "QDeclarativeDebugServer" 0 version pluginNames
       version: an int representing the highest protocol version the client knows
       pluginNames: plugins available on client side
    2. Server sends
         "QDeclarativeDebugClient" 0 version pluginNames pluginVersions
       version: an int representing the highest protocol version the client & server know
       pluginNames: plugins available on server side. plugins both in the client and server message are enabled.
  client plugin advertisement
    1. Client sends
         "QDeclarativeDebugServer" 1 pluginNames
  server plugin advertisement
    1. Server sends
         "QDeclarativeDebugClient" 1 pluginNames pluginVersions
  plugin communication:
       Everything send with a header different to "QDeclarativeDebugServer" is sent to the appropriate plugin.
  */

const int protocolVersion = 1;

// print detailed information about loading of plugins
DEFINE_BOOL_CONFIG_OPTION(qmlDebugVerbose, QML_DEBUGGER_VERBOSE)

class QDeclarativeDebugServerThread;

class QDeclarativeDebugServerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeDebugServer)
public:
    QDeclarativeDebugServerPrivate();

    void advertisePlugins();
    QDeclarativeDebugServerConnection *loadConnectionPlugin(const QString &pluginName);

    QDeclarativeDebugServerConnection *connection;
    QHash<QString, QDeclarativeDebugService *> plugins;
    mutable QReadWriteLock pluginsLock;
    QStringList clientPlugins;
    bool gotHello;

    QMutex messageArrivedMutex;
    QWaitCondition messageArrivedCondition;
    QStringList waitingForMessageNames;
    QDeclarativeDebugServerThread *thread;
    QPluginLoader loader;

private:
    // private slot
    void _q_sendMessages(const QList<QByteArray> &messages);
};

class QDeclarativeDebugServerThread : public QThread
{
public:
    void setPluginName(const QString &pluginName) {
        m_pluginName = pluginName;
    }

    void setPort(int port, bool block) {
        m_port = port;
        m_block = block;
    }

    void run();

private:
    QString m_pluginName;
    int m_port;
    bool m_block;
};

QDeclarativeDebugServerPrivate::QDeclarativeDebugServerPrivate() :
    connection(0),
    gotHello(false),
    thread(0)
{
    // used in _q_sendMessages
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
}

void QDeclarativeDebugServerPrivate::advertisePlugins()
{
    Q_Q(QDeclarativeDebugServer);

    if (!gotHello)
        return;

    QByteArray message;
    {
        QDataStream out(&message, QIODevice::WriteOnly);
        QStringList pluginNames;
        QList<float> pluginVersions;
        foreach (QDeclarativeDebugService *service, plugins.values()) {
            pluginNames << service->name();
            pluginVersions << service->version();
        }
        out << QString(QLatin1String("QDeclarativeDebugClient")) << 1 << pluginNames << pluginVersions;
    }

    QMetaObject::invokeMethod(q, "_q_sendMessages", Qt::QueuedConnection, Q_ARG(QList<QByteArray>, QList<QByteArray>() << message));
}

QDeclarativeDebugServerConnection *QDeclarativeDebugServerPrivate::loadConnectionPlugin(
        const QString &pluginName)
{
#ifndef QT_NO_LIBRARY
    QStringList pluginCandidates;
    const QStringList paths = QCoreApplication::libraryPaths();
    foreach (const QString &libPath, paths) {
        const QDir dir(libPath + QLatin1String("/qmltooling"));
        if (dir.exists()) {
            QStringList plugins(dir.entryList(QDir::Files));
            foreach (const QString &pluginPath, plugins) {
                if (QFileInfo(pluginPath).fileName().contains(pluginName))
                    pluginCandidates << dir.absoluteFilePath(pluginPath);
            }
        }
    }

    foreach (const QString &pluginPath, pluginCandidates) {
        if (qmlDebugVerbose())
            qDebug() << "QDeclarativeDebugServer: Trying to load plugin " << pluginPath << "...";

        loader.setFileName(pluginPath);
        if (!loader.load()) {
            if (qmlDebugVerbose())
                qDebug() << "QDeclarativeDebugServer: Error while loading: " << loader.errorString();
            continue;
        }
        if (QObject *instance = loader.instance())
            connection = qobject_cast<QDeclarativeDebugServerConnection*>(instance);

        if (connection) {
            if (qmlDebugVerbose())
                qDebug() << "QDeclarativeDebugServer: Plugin successfully loaded.";

            return connection;
        }

        if (qmlDebugVerbose())
            qDebug() << "QDeclarativeDebugServer: Plugin does not implement interface QDeclarativeDebugServerConnection.";

        loader.unload();
    }
#endif
    return 0;
}

void QDeclarativeDebugServerThread::run()
{
    QDeclarativeDebugServer *server = QDeclarativeDebugServer::instance();
    QDeclarativeDebugServerConnection *connection
            = server->d_func()->loadConnectionPlugin(m_pluginName);
    if (connection) {
        connection->setServer(QDeclarativeDebugServer::instance());
        connection->setPort(m_port, m_block);
    } else {
        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
        qWarning() << QString::fromAscii("QDeclarativeDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                         "Remote debugger plugin has not been found.").arg(appD->qmljsDebugArgumentsString());
    }

    exec();

    // make sure events still waiting are processed
    QEventLoop eventLoop;
    eventLoop.processEvents(QEventLoop::AllEvents);
}

bool QDeclarativeDebugServer::hasDebuggingClient() const
{
    Q_D(const QDeclarativeDebugServer);
    return d->connection
            && d->connection->isConnected()
            && d->gotHello;
}

static QDeclarativeDebugServer *qDeclarativeDebugServer = 0;


static void cleanup()
{
    delete qDeclarativeDebugServer;
    qDeclarativeDebugServer = 0;
}

QDeclarativeDebugServer *QDeclarativeDebugServer::instance()
{
    static bool commandLineTested = false;

    if (!commandLineTested) {
        commandLineTested = true;

        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
#ifndef QDECLARATIVE_NO_DEBUG_PROTOCOL
        // ### remove port definition when protocol is changed
        int port = 0;
        bool block = false;
        bool ok = false;

        // format: qmljsdebugger=port:3768[,block] OR qmljsdebugger=ost[,block]
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            if (!QDeclarativeEnginePrivate::qml_debugging_enabled) {
                qWarning() << QString::fromLatin1(
                                  "QDeclarativeDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                  "Debugging has not been enabled.").arg(
                                  appD->qmljsDebugArgumentsString());
                return 0;
            }

            QString pluginName;
            if (appD->qmljsDebugArgumentsString().indexOf(QLatin1String("port:")) == 0) {
                int separatorIndex = appD->qmljsDebugArgumentsString().indexOf(QLatin1Char(','));
                port = appD->qmljsDebugArgumentsString().mid(5, separatorIndex - 5).toInt(&ok);
                pluginName = QLatin1String("qmldbg_tcp");
            } else if (appD->qmljsDebugArgumentsString().contains(QLatin1String("ost"))) {
                pluginName = QLatin1String("qmldbg_ost");
                ok = true;
            }

            block = appD->qmljsDebugArgumentsString().contains(QLatin1String("block"));

            if (ok) {
                qDeclarativeDebugServer = new QDeclarativeDebugServer();
                QDeclarativeDebugServerThread *thread = new QDeclarativeDebugServerThread;
                qDeclarativeDebugServer->d_func()->thread = thread;
                qDeclarativeDebugServer->moveToThread(thread);
                thread->setPluginName(pluginName);
                thread->setPort(port, block);
                thread->start();

                if (block) {
                    QDeclarativeDebugServerPrivate *d = qDeclarativeDebugServer->d_func();
                    d->messageArrivedMutex.lock();
                    d->messageArrivedCondition.wait(&d->messageArrivedMutex);
                    d->messageArrivedMutex.unlock();
                }

            } else {
                qWarning() << QString::fromLatin1(
                                  "QDeclarativeDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                                  "Format is -qmljsdebugger=port:<port>[,block]").arg(
                                  appD->qmljsDebugArgumentsString());
            }
        }
#else
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            qWarning() << QString::fromLatin1(
                         "QDeclarativeDebugServer: Ignoring \"-qmljsdebugger=%1\". "
                         "QtDeclarative is not configured for debugging.").arg(
                         appD->qmljsDebugArgumentsString());
        }
#endif
    }

    return qDeclarativeDebugServer;
}

QDeclarativeDebugServer::QDeclarativeDebugServer()
    : QObject(*(new QDeclarativeDebugServerPrivate))
{
    qAddPostRoutine(cleanup);
}

QDeclarativeDebugServer::~QDeclarativeDebugServer()
{
    Q_D(QDeclarativeDebugServer);

    QReadLocker(&d->pluginsLock);
    {
        foreach (QDeclarativeDebugService *service, d->plugins.values()) {
            service->stateAboutToBeChanged(QDeclarativeDebugService::NotConnected);
            service->d_func()->server = 0;
            service->d_func()->state = QDeclarativeDebugService::NotConnected;
            service->stateChanged(QDeclarativeDebugService::NotConnected);
        }
    }

    if (d->thread) {
        d->thread->exit();
        d->thread->wait();
        delete d->thread;
    }
    delete d->connection;
}

void QDeclarativeDebugServer::receiveMessage(const QByteArray &message)
{
    Q_D(QDeclarativeDebugServer);

    QDataStream in(message);

    QString name;

    in >> name;
    if (name == QLatin1String("QDeclarativeDebugServer")) {
        int op = -1;
        in >> op;
        if (op == 0) {
            int version;
            in >> version >> d->clientPlugins;

            // Send the hello answer immediately, since it needs to arrive before
            // the plugins below start sending messages.
            QByteArray helloAnswer;
            {
                QDataStream out(&helloAnswer, QIODevice::WriteOnly);
                QStringList pluginNames;
                QList<float> pluginVersions;
                foreach (QDeclarativeDebugService *service, d->plugins.values()) {
                    pluginNames << service->name();
                    pluginVersions << service->version();
                }

                out << QString(QLatin1String("QDeclarativeDebugClient")) << 0 << protocolVersion << pluginNames << pluginVersions;
            }
            d->connection->send(QList<QByteArray>() << helloAnswer);

            d->gotHello = true;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QDeclarativeDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                QDeclarativeDebugService::State newState = QDeclarativeDebugService::Unavailable;
                if (d->clientPlugins.contains(iter.key()))
                    newState = QDeclarativeDebugService::Enabled;
                iter.value()->d_func()->state = newState;
                iter.value()->stateChanged(newState);
            }

            qWarning("QDeclarativeDebugServer: Connection established");
            d->messageArrivedCondition.wakeAll();

        } else if (op == 1) {

            // Service Discovery
            QStringList oldClientPlugins = d->clientPlugins;
            in >> d->clientPlugins;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QDeclarativeDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                const QString pluginName = iter.key();
                QDeclarativeDebugService::State newState = QDeclarativeDebugService::Unavailable;
                if (d->clientPlugins.contains(pluginName))
                    newState = QDeclarativeDebugService::Enabled;

                if (oldClientPlugins.contains(pluginName)
                        != d->clientPlugins.contains(pluginName)) {
                    iter.value()->d_func()->state = newState;
                    iter.value()->stateChanged(newState);
                }
            }

        } else {
            qWarning("QDeclarativeDebugServer: Invalid control message %d", op);
            d->connection->disconnect();
            return;
        }

    } else {
        if (d->gotHello) {
            QByteArray message;
            in >> message;

            QReadLocker(&d->pluginsLock);
            QHash<QString, QDeclarativeDebugService *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end()) {
                qWarning() << "QDeclarativeDebugServer: Message received for missing plugin" << name;
            } else {
                (*iter)->messageReceived(message);

                if (d->waitingForMessageNames.removeOne(name))
                    d->messageArrivedCondition.wakeAll();
            }
        } else {
            qWarning("QDeclarativeDebugServer: Invalid hello message");
        }

    }
}

void QDeclarativeDebugServerPrivate::_q_sendMessages(const QList<QByteArray> &messages)
{
    if (connection)
        connection->send(messages);
}

QList<QDeclarativeDebugService*> QDeclarativeDebugServer::services() const
{
    const Q_D(QDeclarativeDebugServer);
    QReadLocker(&d->pluginsLock);
    return d->plugins.values();
}

QStringList QDeclarativeDebugServer::serviceNames() const
{
    const Q_D(QDeclarativeDebugServer);
    QReadLocker(&d->pluginsLock);
    return d->plugins.keys();
}

bool QDeclarativeDebugServer::addService(QDeclarativeDebugService *service)
{
    Q_D(QDeclarativeDebugServer);
    {
        QWriteLocker(&d->pluginsLock);
        if (!service || d->plugins.contains(service->name()))
            return false;
        d->plugins.insert(service->name(), service);
    }
    {
        QReadLocker(&d->pluginsLock);
        d->advertisePlugins();
        QDeclarativeDebugService::State newState = QDeclarativeDebugService::Unavailable;
        if (d->clientPlugins.contains(service->name()))
            newState = QDeclarativeDebugService::Enabled;
        service->d_func()->state = newState;
    }
    return true;
}

bool QDeclarativeDebugServer::removeService(QDeclarativeDebugService *service)
{
    Q_D(QDeclarativeDebugServer);
    {
        QWriteLocker(&d->pluginsLock);
        if (!service || !d->plugins.contains(service->name()))
            return false;
        d->plugins.remove(service->name());
    }
    {
        QReadLocker(&d->pluginsLock);
        QDeclarativeDebugService::State newState = QDeclarativeDebugService::NotConnected;
        service->stateAboutToBeChanged(newState);
        d->advertisePlugins();
        service->d_func()->server = 0;
        service->d_func()->state = newState;
        service->stateChanged(newState);
    }

    return true;
}

void QDeclarativeDebugServer::sendMessages(QDeclarativeDebugService *service,
                                          const QList<QByteArray> &messages)
{
    QList<QByteArray> prefixedMessages;
    foreach (const QByteArray &message, messages) {
        QByteArray prefixed;
        QDataStream out(&prefixed, QIODevice::WriteOnly);
        out << service->name() << message;
        prefixedMessages << prefixed;
    }

    QMetaObject::invokeMethod(this, "_q_sendMessages", Qt::QueuedConnection, Q_ARG(QList<QByteArray>, prefixedMessages));
}

bool QDeclarativeDebugServer::waitForMessage(QDeclarativeDebugService *service)
{
    Q_D(QDeclarativeDebugServer);
    QReadLocker(&d->pluginsLock);

    if (!service
            || !d->plugins.contains(service->name()))
        return false;

    d->messageArrivedMutex.lock();
    d->waitingForMessageNames << service->name();
    do {
        d->messageArrivedCondition.wait(&d->messageArrivedMutex);
    } while (d->waitingForMessageNames.contains(service->name()));
    d->messageArrivedMutex.unlock();
    return true;
}

QT_END_NAMESPACE

#include "moc_qdeclarativedebugserver_p.cpp"
