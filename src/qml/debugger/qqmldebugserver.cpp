/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldebugserver_p.h"
#include "qqmldebugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/QAtomicInt>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>
#include <QtCore/QStringList>
#include <QtCore/qwaitcondition.h>

#include <private/qobject_p.h>
#include <private/qcoreapplication_p.h>

QT_BEGIN_NAMESPACE

/*
  QQmlDebug Protocol (Version 1):

  handshake:
    1. Client sends
         "QDeclarativeDebugServer" 0 version pluginNames [QDataStream version]
       version: an int representing the highest protocol version the client knows
       pluginNames: plugins available on client side
    2. Server sends
         "QDeclarativeDebugClient" 0 version pluginNames pluginVersions [QDataStream version]
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
int QQmlDebugServer::s_dataStreamVersion = QDataStream::Qt_4_7;

// print detailed information about loading of plugins
DEFINE_BOOL_CONFIG_OPTION(qmlDebugVerbose, QML_DEBUGGER_VERBOSE)

class QQmlDebugServerThread;

class QQmlDebugServerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlDebugServer)
public:
    QQmlDebugServerPrivate();

    void advertisePlugins();
    QQmlDebugServerConnection *loadConnectionPlugin(const QString &pluginName);

    QQmlDebugServerConnection *connection;
    QHash<QString, QQmlDebugService *> plugins;
    mutable QReadWriteLock pluginsLock;
    QStringList clientPlugins;
    bool gotHello;
    bool blockingMode;

    QMutex helloMutex;
    QWaitCondition helloCondition;
    QQmlDebugServerThread *thread;
    QPluginLoader loader;
    QAtomicInt changeServiceStateCalls;

private:
    // private slots
    void _q_changeServiceState(const QString &serviceName,
                               QQmlDebugService::State newState);
    void _q_sendMessages(const QList<QByteArray> &messages);
};

class QQmlDebugServerThread : public QThread
{
public:
    void setPluginName(const QString &pluginName) {
        m_pluginName = pluginName;
    }

    void setPortRange(int portFrom, int portTo, bool block, QString &hostAddress) {
        m_portFrom = portFrom;
        m_portTo = portTo;
        m_block = block;
        m_hostAddress = hostAddress;
    }

    void run();

private:
    QString m_pluginName;
    int m_portFrom;
    int m_portTo;
    bool m_block;
    QString m_hostAddress;
};

QQmlDebugServerPrivate::QQmlDebugServerPrivate() :
    connection(0),
    gotHello(false),
    blockingMode(false),
    thread(0)
{
    // used in _q_sendMessages
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
    // used in _q_changeServiceState
    qRegisterMetaType<QQmlDebugService::State>("QQmlDebugService::State");
}

void QQmlDebugServerPrivate::advertisePlugins()
{
    Q_Q(QQmlDebugServer);

    if (!gotHello)
        return;

    QByteArray message;
    {
        QQmlDebugStream out(&message, QIODevice::WriteOnly);
        QStringList pluginNames;
        QList<float> pluginVersions;
        foreach (QQmlDebugService *service, plugins.values()) {
            pluginNames << service->name();
            pluginVersions << service->version();
        }
        out << QString(QStringLiteral("QDeclarativeDebugClient")) << 1 << pluginNames << pluginVersions;
    }

    QMetaObject::invokeMethod(q, "_q_sendMessages", Qt::QueuedConnection, Q_ARG(QList<QByteArray>, QList<QByteArray>() << message));
}

QQmlDebugServerConnection *QQmlDebugServerPrivate::loadConnectionPlugin(
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
            qDebug() << "QML Debugger: Trying to load plugin " << pluginPath << "...";

        loader.setFileName(pluginPath);
        if (!loader.load()) {
            if (qmlDebugVerbose())
                qDebug() << "QML Debugger: Error while loading: " << loader.errorString();
            continue;
        }
        if (QObject *instance = loader.instance())
            connection = qobject_cast<QQmlDebugServerConnection*>(instance);

        if (connection) {
            if (qmlDebugVerbose())
                qDebug() << "QML Debugger: Plugin successfully loaded.";

            return connection;
        }

        if (qmlDebugVerbose())
            qDebug() << "QML Debugger: Plugin does not implement interface QQmlDebugServerConnection.";

        loader.unload();
    }
#endif
    return 0;
}

void QQmlDebugServerThread::run()
{
    QQmlDebugServer *server = QQmlDebugServer::instance();
    QQmlDebugServerConnection *connection
            = server->d_func()->loadConnectionPlugin(m_pluginName);
    if (connection) {
        connection->setServer(QQmlDebugServer::instance());
        connection->setPortRange(m_portFrom, m_portTo, m_block, m_hostAddress);
    } else {
        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
        qWarning() << QString(QLatin1String("QML Debugger: Ignoring \"-qmljsdebugger=%1\". "
                                            "Remote debugger plugin has not been found.")).arg(appD->qmljsDebugArgumentsString());
    }

    exec();

    // make sure events still waiting are processed
    QEventLoop eventLoop;
    eventLoop.processEvents(QEventLoop::AllEvents);
}

bool QQmlDebugServer::hasDebuggingClient() const
{
    Q_D(const QQmlDebugServer);
    return d->connection
            && d->connection->isConnected()
            && d->gotHello;
}

bool QQmlDebugServer::blockingMode() const
{
    Q_D(const QQmlDebugServer);
    return d->blockingMode;
}

static QQmlDebugServer *qQmlDebugServer = 0;


static void cleanup()
{
    delete qQmlDebugServer;
    qQmlDebugServer = 0;
}

QQmlDebugServer *QQmlDebugServer::instance()
{
    static bool commandLineTested = false;

    if (!commandLineTested) {
        commandLineTested = true;

        QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
#ifndef QT_QML_NO_DEBUGGER
        // ### remove port definition when protocol is changed
        int portFrom = 0;
        int portTo = 0;
        bool block = false;
        bool ok = false;
        QString hostAddress;

        // format: qmljsdebugger=port:<port_from>[,port_to],host:<ip address>][,block]
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            if (!QQmlEnginePrivate::qml_debugging_enabled) {
                qWarning() << QString(QLatin1String(
                                  "QML Debugger: Ignoring \"-qmljsdebugger=%1\". "
                                  "Debugging has not been enabled.")).arg(
                                  appD->qmljsDebugArgumentsString());
                return 0;
            }

            QString pluginName;
            QStringList lstjsDebugArguments = appD->qmljsDebugArgumentsString()
                                                                    .split(QLatin1Char(','));
            QStringList::const_iterator argsItEnd = lstjsDebugArguments.end();
            QStringList::const_iterator argsIt = lstjsDebugArguments.begin();
            for (; argsIt != argsItEnd; ++argsIt) {
                const QString strArgument = *argsIt;
                if (strArgument.startsWith(QLatin1String("port:"))) {
                    pluginName = QLatin1String("qmldbg_tcp");
                    portFrom = strArgument.mid(5).toInt(&ok);
                    portTo = portFrom;
                    QStringList::const_iterator argsNext = argsIt + 1;
                    if (argsNext == argsItEnd)
                        break;
                    const QString nextArgument = *argsNext;
                    if (ok && nextArgument.contains(QRegExp(QStringLiteral("^\\s*\\d+\\s*$")))) {
                        portTo = nextArgument.toInt(&ok);
                        ++argsIt;
                    }
                } else if (strArgument.startsWith(QLatin1String("host:"))) {
                    hostAddress = strArgument.mid(5);
                } else if (strArgument == QLatin1String("block")) {
                    block = true;
                } else {
                    qWarning() << QString::fromLatin1("QML Debugger: Invalid argument '%1' "
                                                      "detected. Ignoring the same.")
                                                       .arg(strArgument);
                }
            }

            if (ok) {
                qQmlDebugServer = new QQmlDebugServer();
                QQmlDebugServerThread *thread = new QQmlDebugServerThread;
                qQmlDebugServer->d_func()->thread = thread;
                qQmlDebugServer->moveToThread(thread);
                thread->setPluginName(pluginName);
                thread->setPortRange(portFrom, portTo, block, hostAddress);

                QQmlDebugServerPrivate *d = qQmlDebugServer->d_func();
                d->blockingMode = block;

                QMutexLocker locker(&d->helloMutex);
                thread->start();

                if (d->blockingMode)
                    d->helloCondition.wait(&d->helloMutex);

            } else {
                qWarning() << QString(QLatin1String(
                                  "QML Debugger: Ignoring \"-qmljsdebugger=%1\". "
                                  "Format is qmljsdebugger=port:<port_from>[,port_to],host:"
                                  "<ip address>][,block]")).arg(appD->qmljsDebugArgumentsString());
            }
        }
#else
        if (!appD->qmljsDebugArgumentsString().isEmpty()) {
            qWarning() << QString(QLatin1String(
                         "QML Debugger: Ignoring \"-qmljsdebugger=%1\". "
                         "QtQml is not configured for debugging.")).arg(
                         appD->qmljsDebugArgumentsString());
        }
#endif
    }

    return qQmlDebugServer;
}

QQmlDebugServer::QQmlDebugServer()
    : QObject(*(new QQmlDebugServerPrivate))
{
    qAddPostRoutine(cleanup);
}

// called from GUI thread!
QQmlDebugServer::~QQmlDebugServer()
{
    Q_D(QQmlDebugServer);

    {
        QReadLocker lock(&d->pluginsLock);
        foreach (QQmlDebugService *service, d->plugins.values()) {
            d->changeServiceStateCalls.ref();
            QMetaObject::invokeMethod(this, "_q_changeServiceState", Qt::QueuedConnection,
                                      Q_ARG(QString, service->name()),
                                      Q_ARG(QQmlDebugService::State, QQmlDebugService::NotConnected));
        }
    }

    // Wait for changeServiceState calls to finish
    // (while running an event loop because some services
    // might again use slots to execute stuff in the GUI thread)
    QEventLoop loop;
    while (!d->changeServiceStateCalls.testAndSetOrdered(0, 0))
        loop.processEvents();

    if (d->thread) {
        d->thread->exit();
        d->thread->wait();
        delete d->thread;
    }
    delete d->connection;
}

void QQmlDebugServer::receiveMessage(const QByteArray &message)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == thread());

    Q_D(QQmlDebugServer);

    QQmlDebugStream in(message);

    QString name;

    in >> name;
    if (name == QLatin1String("QDeclarativeDebugServer")) {
        int op = -1;
        in >> op;
        if (op == 0) {
            int version;
            in >> version >> d->clientPlugins;

            //Get the supported QDataStream version
            if (!in.atEnd()) {
                in >> s_dataStreamVersion;
                if (s_dataStreamVersion > QDataStream().version())
                    s_dataStreamVersion = QDataStream().version();
            }

            // Send the hello answer immediately, since it needs to arrive before
            // the plugins below start sending messages.

            QByteArray helloAnswer;
            {
                QReadLocker readPluginsLock(&d->pluginsLock);
                QQmlDebugStream out(&helloAnswer, QIODevice::WriteOnly);
                QStringList pluginNames;
                QList<float> pluginVersions;
                foreach (QQmlDebugService *service, d->plugins.values()) {
                    pluginNames << service->name();
                    pluginVersions << service->version();
                }

                out << QString(QStringLiteral("QDeclarativeDebugClient")) << 0 << protocolVersion
                    << pluginNames << pluginVersions << s_dataStreamVersion;
            }
            d->connection->send(QList<QByteArray>() << helloAnswer);

            d->gotHello = true;

            QReadLocker lock(&d->pluginsLock);
            QHash<QString, QQmlDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (d->clientPlugins.contains(iter.key()))
                    newState = QQmlDebugService::Enabled;
                d->changeServiceStateCalls.ref();
                d->_q_changeServiceState(iter.value()->name(), newState);
            }

            QMutexLocker helloLock(&d->helloMutex);
            d->helloCondition.wakeAll();

        } else if (op == 1) {

            // Service Discovery
            QStringList oldClientPlugins = d->clientPlugins;
            in >> d->clientPlugins;

            QReadLocker lock(&d->pluginsLock);
            QHash<QString, QQmlDebugService*>::ConstIterator iter = d->plugins.constBegin();
            for (; iter != d->plugins.constEnd(); ++iter) {
                const QString pluginName = iter.key();
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (d->clientPlugins.contains(pluginName))
                    newState = QQmlDebugService::Enabled;

                if (oldClientPlugins.contains(pluginName)
                        != d->clientPlugins.contains(pluginName)) {
                    d->changeServiceStateCalls.ref();
                    d->_q_changeServiceState(iter.value()->name(), newState);
                }
            }

        } else {
            qWarning("QML Debugger: Invalid control message %d.", op);
            d->connection->disconnect();
            return;
        }

    } else {
        if (d->gotHello) {
            QByteArray message;
            in >> message;

            QReadLocker lock(&d->pluginsLock);
            QHash<QString, QQmlDebugService *>::Iterator iter = d->plugins.find(name);
            if (iter == d->plugins.end()) {
                qWarning() << "QML Debugger: Message received for missing plugin" << name << '.';
            } else {
                (*iter)->messageReceived(message);
            }
        } else {
            qWarning("QML Debugger: Invalid hello message.");
        }

    }
}

void QQmlDebugServerPrivate::_q_changeServiceState(const QString &serviceName,
                                                   QQmlDebugService::State newState)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == q_func()->thread());

    QQmlDebugService *service = 0;
    {
        QReadLocker lock(&pluginsLock);
        service = plugins.value(serviceName);
    }

    if (service && (service->d_func()->state != newState)) {
        service->stateAboutToBeChanged(newState);
        service->d_func()->state = newState;
        if (newState == QQmlDebugService::NotConnected)
            service->d_func()->server = 0;
        service->stateChanged(newState);
    }

    changeServiceStateCalls.deref();
}

void QQmlDebugServerPrivate::_q_sendMessages(const QList<QByteArray> &messages)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == q_func()->thread());

    if (connection)
        connection->send(messages);
}

QList<QQmlDebugService*> QQmlDebugServer::services() const
{
    Q_D(const QQmlDebugServer);
    QReadLocker lock(&d->pluginsLock);
    return d->plugins.values();
}

QStringList QQmlDebugServer::serviceNames() const
{
    Q_D(const QQmlDebugServer);
    QReadLocker lock(&d->pluginsLock);
    return d->plugins.keys();
}

bool QQmlDebugServer::addService(QQmlDebugService *service)
{
    Q_D(QQmlDebugServer);

    // to be executed in GUI thread
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    {
        QWriteLocker lock(&d->pluginsLock);
        if (!service || d->plugins.contains(service->name()))
            return false;
        d->plugins.insert(service->name(), service);
    }
    {
        QReadLocker lock(&d->pluginsLock);
        d->advertisePlugins();
        QQmlDebugService::State newState = QQmlDebugService::Unavailable;
        if (d->clientPlugins.contains(service->name()))
            newState = QQmlDebugService::Enabled;
        service->d_func()->state = newState;
    }
    return true;
}

bool QQmlDebugServer::removeService(QQmlDebugService *service)
{
    Q_D(QQmlDebugServer);

    // to be executed in GUI thread
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    {
        QWriteLocker lock(&d->pluginsLock);
        QQmlDebugService::State newState = QQmlDebugService::NotConnected;

        d->changeServiceStateCalls.ref();
        QMetaObject::invokeMethod(this, "_q_changeServiceState", Qt::QueuedConnection,
                                  Q_ARG(QString, service->name()),
                                  Q_ARG(QQmlDebugService::State, newState));

        if (!service || !d->plugins.contains(service->name()))
            return false;
        d->plugins.remove(service->name());

        d->advertisePlugins();
    }

    return true;
}

void QQmlDebugServer::sendMessages(QQmlDebugService *service,
                                          const QList<QByteArray> &messages)
{
    QList<QByteArray> prefixedMessages;
    foreach (const QByteArray &message, messages) {
        QByteArray prefixed;
        QQmlDebugStream out(&prefixed, QIODevice::WriteOnly);
        out << service->name() << message;
        prefixedMessages << prefixed;
    }

    QMetaObject::invokeMethod(this, "_q_sendMessages", Qt::QueuedConnection,
                              Q_ARG(QList<QByteArray>, prefixedMessages));
}

QT_END_NAMESPACE

#include "moc_qqmldebugserver_p.cpp"
