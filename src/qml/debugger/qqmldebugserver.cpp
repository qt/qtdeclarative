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

#include "qqmldebugserver_p.h"
#include "qqmldebugserverconnection_p.h"
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

#if defined(QT_STATIC) && !defined(QT_NO_QML_DEBUGGER) && !defined(QT_NO_LIBRARY)
#include "../../plugins/qmltooling/qmldbg_tcp/qtcpserverconnection.h"
#endif

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
  server plugin advertisement (not implemented: all services are required to register before open())
    1. Server sends
         "QDeclarativeDebugClient" 1 pluginNames pluginVersions
  plugin communication:
       Everything send with a header different to "QDeclarativeDebugServer" is sent to the appropriate plugin.
  */

const int protocolVersion = 1;

// print detailed information about loading of plugins
#ifndef QT_NO_LIBRARY
DEFINE_BOOL_CONFIG_OPTION(qmlDebugVerbose, QML_DEBUGGER_VERBOSE)
#endif

class QQmlDebugServerThread;
class QQmlDebugServerImpl : public QQmlDebugServer
{
    Q_OBJECT
public:

    bool blockingMode() const;

    QQmlDebugService *service(const QString &name) const;

    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);

    bool addService(QQmlDebugService *service);
    bool removeService(QQmlDebugService *service);

    bool open(const QVariantHash &configuration);

    void receiveMessage(const QByteArray &message);

    template<class Action>
    bool enable(Action action);
    bool enableFromArguments();

private slots:
    void wakeEngine(QQmlEngine *engine);
    void sendMessage(const QString &name, const QByteArray &message);
    void sendMessages(const QString &name, const QList<QByteArray> &messages);
    void changeServiceState(const QString &serviceName, QQmlDebugService::State state);
    void removeThread();

private:
    friend struct StartTcpServerAction;
    friend struct ConnectToLocalAction;
    friend class QQmlDebugServerThread;
    friend struct QQmlDebugServerInstanceWrapper;
    friend QQmlDebugConnector *loadQQmlDebugConnector(const QString &key);

    class EngineCondition {
    public:
        EngineCondition() : numServices(0), condition(new QWaitCondition) {}

        bool waitForServices(QMutex *locked, int numEngines);

        void wake();
    private:
        int numServices;

        // shared pointer to allow for QHash-inflicted copying.
        QSharedPointer<QWaitCondition> condition;
    };

    QQmlDebugServerImpl();

    bool init(const QString &pluginName, bool block);

    void cleanup();
    QQmlDebugServerConnection *loadConnectionPlugin(const QString &pluginName);

    QQmlDebugServerConnection *m_connection;
    QHash<QString, QQmlDebugService *> m_plugins;
    QStringList m_clientPlugins;
    bool m_gotHello;
    bool m_blockingMode;

    QHash<QQmlEngine *, EngineCondition> m_engineConditions;

    QMutex m_helloMutex;
    QWaitCondition m_helloCondition;
    QQmlDebugServerThread *m_thread;
#ifndef QT_NO_LIBRARY
    QPluginLoader m_loader;
#endif
    QAtomicInt m_changeServiceStateCalls;
};

// We can't friend the Q_GLOBAL_STATIC to have the constructor available so we need a little
// workaround here. Using this wrapper we can also make QQmlEnginePrivate's cleanup() available to
// qAddPostRoutine(). We can't do the cleanup in the destructor because we need a  QApplication to
// be available when stopping the plugins.
struct QQmlDebugServerInstanceWrapper {
    QQmlDebugServerImpl m_instance;
    void cleanup();
};

Q_GLOBAL_STATIC(QQmlDebugServerInstanceWrapper, debugServerInstance)

void QQmlDebugServerInstanceWrapper::cleanup()
{
    m_instance.cleanup();
}

class QQmlDebugServerThread : public QThread
{
public:
    void setPluginName(const QString &pluginName) {
        m_pluginName = pluginName;
    }

    void setPortRange(int portFrom, int portTo, bool block, const QString &hostAddress) {
        m_portFrom = portFrom;
        m_portTo = portTo;
        m_block = block;
        m_hostAddress = hostAddress;
    }

    void setFileName(const QString &fileName, bool block)
    {
        m_fileName = fileName;
        m_block = block;
    }

    void run();

private:
    QString m_pluginName;
    int m_portFrom;
    int m_portTo;
    bool m_block;
    QString m_hostAddress;
    QString m_fileName;
};

struct StartTcpServerAction {
    int portFrom;
    int portTo;
    bool block;
    QString hostAddress;

    StartTcpServerAction(int portFrom, int portTo, bool block, const QString &hostAddress) :
        portFrom(portFrom), portTo(portTo), block(block), hostAddress(hostAddress) {}

    bool operator()(QQmlDebugServerImpl *d)
    {
        if (!d->init(QLatin1String("qmldbg_tcp"), block))
            return false;
        d->m_thread->setPortRange(portFrom, portTo == -1 ? portFrom : portTo, block, hostAddress);
        return true;
    }
};

struct ConnectToLocalAction {
    QString fileName;
    bool block;

    ConnectToLocalAction(const QString &fileName, bool block) :
        fileName(fileName), block(block) {}

    bool operator()(QQmlDebugServerImpl *d)
    {
        if (!d->init(QLatin1String("qmldbg_local"), block))
            return false;
        d->m_thread->setFileName(fileName, block);
        return true;
    }
};

void QQmlDebugServerImpl::cleanup()
{
    foreach (QQmlDebugService *service, m_plugins.values()) {
        m_changeServiceStateCalls.ref();
        QMetaObject::invokeMethod(this, "changeServiceState", Qt::QueuedConnection,
                                  Q_ARG(QString, service->name()),
                                  Q_ARG(QQmlDebugService::State, QQmlDebugService::NotConnected));
    }

    // Wait for changeServiceState calls to finish
    // (while running an event loop because some services
    // might again use slots to execute stuff in the GUI thread)
    QEventLoop loop;
    while (!m_changeServiceStateCalls.testAndSetOrdered(0, 0))
        loop.processEvents();

    // Stop the thread while the application is still there. Copy here as the thread will set itself
    // to 0 when it stops. It will also do deleteLater, but as long as we don't allow the GUI
    // thread's event loop to run we're safe from that.
    QThread *threadCopy = m_thread;
    if (threadCopy) {
        threadCopy->exit();
        threadCopy->wait();
    }
}

QQmlDebugServerConnection *QQmlDebugServerImpl::loadConnectionPlugin(const QString &pluginName)
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

    QQmlDebugServerConnection *loadedConnection = 0;
    foreach (const QString &pluginPath, pluginCandidates) {
        if (qmlDebugVerbose())
            qDebug() << "QML Debugger: Trying to load plugin " << pluginPath << "...";

        m_loader.setFileName(pluginPath);
        if (!m_loader.load()) {
            if (qmlDebugVerbose())
                qDebug() << "QML Debugger: Error while loading: " << m_loader.errorString();
            continue;
        }
        if (QObject *instance = m_loader.instance())
            loadedConnection = qobject_cast<QQmlDebugServerConnection*>(instance);

        if (loadedConnection) {
            if (qmlDebugVerbose())
                qDebug() << "QML Debugger: Plugin successfully loaded.";

            return loadedConnection;
        }

        if (qmlDebugVerbose())
            qDebug() << "QML Debugger: Plugin does not implement interface QQmlDebugServerConnection.";

        m_loader.unload();
    }
#else
    Q_UNUSED(pluginName);
#endif
    return 0;
}

void QQmlDebugServerThread::run()
{
    QQmlDebugServerInstanceWrapper *wrapper = debugServerInstance();
    Q_ASSERT_X(wrapper != 0, Q_FUNC_INFO, "There should always be a debug server available here.");
    QQmlDebugServerImpl *server = &wrapper->m_instance;
#if defined(QT_STATIC) && !defined(QT_NO_QML_DEBUGGER) && !defined(QT_NO_LIBRARY)
    QQmlDebugServerConnection *connection
            = new QTcpServerConnection;
#else
    QQmlDebugServerConnection *connection = server->loadConnectionPlugin(m_pluginName);
#endif
    if (connection) {
        connection->setServer(server);

        if (m_fileName.isEmpty()) {
            if (!connection->setPortRange(m_portFrom, m_portTo, m_block, m_hostAddress)) {
                delete connection;
                return;
            }
        } else {
            if (!connection->setFileName(m_fileName, m_block)) {
                delete connection;
                return;
            }
        }

        {
            QMutexLocker connectionLocker(&server->m_helloMutex);
            server->m_connection = connection;
            server->m_helloCondition.wakeAll();
        }

        if (m_block)
            connection->waitForConnection();
    } else {
        qWarning() << "QML Debugger: Couldn't load plugin" << m_pluginName;
        return;
    }

    exec();

    // make sure events still waiting are processed
    QEventLoop eventLoop;
    eventLoop.processEvents(QEventLoop::AllEvents);
}

bool QQmlDebugServerImpl::blockingMode() const
{
    return m_blockingMode;
}

QQmlDebugConnector *loadQQmlDebugConnector(const QString &key)
{
    if (key == QLatin1String("QQmlDebugServer")) {
        QQmlDebugServerInstanceWrapper *wrapper = debugServerInstance();
        return wrapper ? &(wrapper->m_instance) : 0;
    } else {
        return 0;
    }
}

static void cleanupOnShutdown()
{
    QQmlDebugServerInstanceWrapper *wrapper = debugServerInstance();
    if (wrapper)
        wrapper->cleanup();
}

bool QQmlDebugServerImpl::init(const QString &pluginName, bool block)
{
    if (m_thread)
        return false;
    static bool postRoutineAdded = false;
    if (!postRoutineAdded) {
        qAddPostRoutine(cleanupOnShutdown);
        postRoutineAdded = true;
    }
    m_thread = new QQmlDebugServerThread;
    moveToThread(m_thread);

    // Remove the thread immmediately when it finishes, so that we don't have to wait for the event
    // loop to signal that.
    QObject::connect(m_thread, SIGNAL(finished()), this, SLOT(removeThread()), Qt::DirectConnection);

    m_thread->setObjectName(QStringLiteral("QQmlDebugServerThread"));
    m_thread->setPluginName(pluginName);
    m_blockingMode = block;
    return true;
}


QQmlDebugServerImpl::QQmlDebugServerImpl() :
    m_connection(0),
    m_gotHello(false),
    m_blockingMode(false),
    m_thread(0)
{
    // used in sendMessages
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
    // used in changeServiceState
    qRegisterMetaType<QQmlDebugService::State>("QQmlDebugService::State");
}

bool QQmlDebugServerImpl::open(const QVariantHash &configuration = QVariantHash())
{
    if (configuration.isEmpty()) {
        return enableFromArguments();
    } if (configuration.contains(QLatin1String("portFrom"))) {
        return enable(StartTcpServerAction(
                             configuration[QLatin1String("portFrom")].toInt(),
                             configuration[QLatin1String("portTo")].toInt(),
                             configuration[QLatin1String("block")].toBool(),
                             configuration[QLatin1String("hostAddress")].toString()));
    } else if (configuration.contains(QLatin1String("fileName"))) {
        return enable(ConnectToLocalAction(configuration[QLatin1String("fileName")].toString(),
                      configuration[QLatin1String("block")].toBool()));
    }

    return false;
}

bool QQmlDebugServerImpl::enableFromArguments()
{
    if (qApp == 0)
        return false;
    QCoreApplicationPrivate *appD = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
#ifndef QT_NO_QML_DEBUGGER
    // ### remove port definition when protocol is changed
    int portFrom = 0;
    int portTo = 0;
    bool block = false;
    bool ok = false;
    QString hostAddress;
    QString fileName;

    // format: qmljsdebugger=port:<port_from>[,port_to],host:<ip address>][,block]
    if (!appD->qmljsDebugArgumentsString().isEmpty()) {
        QStringList lstjsDebugArguments = appD->qmljsDebugArgumentsString()
                                                                .split(QLatin1Char(','));
        QStringList::const_iterator argsItEnd = lstjsDebugArguments.cend();
        QStringList::const_iterator argsIt = lstjsDebugArguments.cbegin();
        for (; argsIt != argsItEnd; ++argsIt) {
            const QString strArgument = *argsIt;
            if (strArgument.startsWith(QLatin1String("port:"))) {
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
            } else if (strArgument.startsWith(QLatin1String("file:"))) {
                fileName = strArgument.mid(5);
                ok = !fileName.isEmpty();
            } else {
                qWarning() << QString::fromLatin1("QML Debugger: Invalid argument '%1' "
                                                  "detected. Ignoring the same.")
                                                   .arg(strArgument);
            }
        }

        if (ok) {
            if (!fileName.isEmpty())
                return enable(ConnectToLocalAction(fileName, block));
            else
                return enable(StartTcpServerAction(portFrom, portTo, block, hostAddress));
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
    return false;
}

void QQmlDebugServerImpl::receiveMessage(const QByteArray &message)
{
    typedef QHash<QString, QQmlDebugService*>::const_iterator DebugServiceConstIt;

    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == thread());

    QQmlDebugStream in(message);

    QString name;

    in >> name;
    if (name == QLatin1String("QDeclarativeDebugServer")) {
        int op = -1;
        in >> op;
        if (op == 0) {
            int version;
            in >> version >> m_clientPlugins;

            //Get the supported QDataStream version
            if (!in.atEnd()) {
                in >> QQmlDebugStream::s_dataStreamVersion;
                if (QQmlDebugStream::s_dataStreamVersion > QDataStream().version())
                    QQmlDebugStream::s_dataStreamVersion = QDataStream().version();
            }

            // Send the hello answer immediately, since it needs to arrive before
            // the plugins below start sending messages.

            QByteArray helloAnswer;
            QQmlDebugStream out(&helloAnswer, QIODevice::WriteOnly);
            QStringList pluginNames;
            QList<float> pluginVersions;
            const QList<QQmlDebugService*> debugServices = m_plugins.values();
            const int count = debugServices.count();
            pluginNames.reserve(count);
            pluginVersions.reserve(count);
            foreach (QQmlDebugService *service, debugServices) {
                pluginNames << service->name();
                pluginVersions << service->version();
            }

            out << QString(QStringLiteral("QDeclarativeDebugClient")) << 0 << protocolVersion
                << pluginNames << pluginVersions << QQmlDebugStream::s_dataStreamVersion;

            m_connection->send(QList<QByteArray>() << helloAnswer);

            QMutexLocker helloLock(&m_helloMutex);
            m_gotHello = true;

            for (DebugServiceConstIt iter = m_plugins.constBegin(), cend = m_plugins.constEnd(); iter != cend; ++iter) {
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (m_clientPlugins.contains(iter.key()))
                    newState = QQmlDebugService::Enabled;
                m_changeServiceStateCalls.ref();
                changeServiceState(iter.value()->name(), newState);
            }

            m_helloCondition.wakeAll();

        } else if (op == 1) {
            // Service Discovery
            QStringList oldClientPlugins = m_clientPlugins;
            in >> m_clientPlugins;

            for (DebugServiceConstIt iter = m_plugins.constBegin(), cend = m_plugins.constEnd(); iter != cend; ++iter) {
                const QString pluginName = iter.key();
                QQmlDebugService::State newState = QQmlDebugService::Unavailable;
                if (m_clientPlugins.contains(pluginName))
                    newState = QQmlDebugService::Enabled;

                if (oldClientPlugins.contains(pluginName)
                        != m_clientPlugins.contains(pluginName)) {
                    m_changeServiceStateCalls.ref();
                    changeServiceState(iter.value()->name(), newState);
                }
            }

        } else {
            qWarning("QML Debugger: Invalid control message %d.", op);
            m_connection->disconnect();
            return;
        }

    } else {
        if (m_gotHello) {
            QByteArray message;
            in >> message;

            QHash<QString, QQmlDebugService *>::Iterator iter = m_plugins.find(name);
            if (iter == m_plugins.end()) {
                qWarning() << "QML Debugger: Message received for missing plugin" << name << '.';
            } else {
                (*iter)->messageReceived(message);
            }
        } else {
            qWarning("QML Debugger: Invalid hello message.");
        }

    }
}

void QQmlDebugServerImpl::changeServiceState(const QString &serviceName,
                                             QQmlDebugService::State newState)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == thread());

    QQmlDebugService *service = m_plugins.value(serviceName);
    if (service && service->state() != newState) {
        service->stateAboutToBeChanged(newState);
        service->setState(newState);
        service->stateChanged(newState);
    }

    m_changeServiceStateCalls.deref();
}

void QQmlDebugServerImpl::removeThread()
{
    Q_ASSERT(m_thread->isFinished());
    Q_ASSERT(QThread::currentThread() == thread());

    QThread *parentThread = m_thread->thread();

    // We cannot delete it right away as it will access its data after the finished() signal.
    m_thread->deleteLater();
    m_thread = 0;

    delete m_connection;
    m_connection = 0;

    // Move it back to the parent thread so that we can potentially restart it on a new thread.
    moveToThread(parentThread);
}

QQmlDebugService *QQmlDebugServerImpl::service(const QString &name) const
{
    return m_plugins.value(name);
}

void QQmlDebugServerImpl::addEngine(QQmlEngine *engine)
{
    // to be executed outside of debugger thread
    Q_ASSERT(QThread::currentThread() != m_thread);

    QMutexLocker locker(&m_helloMutex);
    foreach (QQmlDebugService *service, m_plugins)
        service->engineAboutToBeAdded(engine);

    m_engineConditions[engine].waitForServices(&m_helloMutex, m_plugins.count());

    foreach (QQmlDebugService *service, m_plugins)
        service->engineAdded(engine);
}

void QQmlDebugServerImpl::removeEngine(QQmlEngine *engine)
{
    // to be executed outside of debugger thread
    Q_ASSERT(QThread::currentThread() != m_thread);

    QMutexLocker locker(&m_helloMutex);
    foreach (QQmlDebugService *service, m_plugins)
        service->engineAboutToBeRemoved(engine);

    m_engineConditions[engine].waitForServices(&m_helloMutex, m_plugins.count());

    foreach (QQmlDebugService *service, m_plugins)
        service->engineRemoved(engine);
}

bool QQmlDebugServerImpl::addService(QQmlDebugService *service)
{
    // to be executed before thread starts
    Q_ASSERT(!m_thread);

    connect(service, SIGNAL(messageToClient(QString,QByteArray)),
            this, SLOT(sendMessage(QString,QByteArray)));
    connect(service, SIGNAL(messagesToClient(QString,QList<QByteArray>)),
            this, SLOT(sendMessages(QString,QList<QByteArray>)));

    connect(service, SIGNAL(attachedToEngine(QQmlEngine*)),
            this, SLOT(wakeEngine(QQmlEngine*)), Qt::QueuedConnection);
    connect(service, SIGNAL(detachedFromEngine(QQmlEngine*)),
            this, SLOT(wakeEngine(QQmlEngine*)), Qt::QueuedConnection);


    if (!service || m_plugins.contains(service->name()))
        return false;
    m_plugins.insert(service->name(), service);
    QQmlDebugService::State newState = QQmlDebugService::Unavailable;
    if (m_clientPlugins.contains(service->name()))
        newState = QQmlDebugService::Enabled;
    service->setState(newState);
    return true;
}

bool QQmlDebugServerImpl::removeService(QQmlDebugService *service)
{
    // to be executed after thread ends
    Q_ASSERT(!m_thread);

    QQmlDebugService::State newState = QQmlDebugService::NotConnected;

    m_changeServiceStateCalls.ref();
    QMetaObject::invokeMethod(this, "changeServiceState", Qt::QueuedConnection,
                              Q_ARG(QString, service->name()),
                              Q_ARG(QQmlDebugService::State, newState));

    if (!service || !m_plugins.contains(service->name()))
        return false;

    disconnect(service, SIGNAL(messagesToClient(QString,QList<QByteArray>)),
               this, SLOT(sendMessages(QString,QList<QByteArray>)));
    disconnect(service, SIGNAL(messageToClient(QString,QByteArray)),
               this, SLOT(sendMessage(QString,QByteArray)));

    m_plugins.remove(service->name());

    return true;
}

void QQmlDebugServerImpl::sendMessage(const QString &name, const QByteArray &message)
{
    sendMessages(name, QList<QByteArray>() << message);
}

void QQmlDebugServerImpl::sendMessages(const QString &name, const QList<QByteArray> &messages)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == thread());

    if (!m_connection)
        return;

    if (!name.isEmpty()) {
        if (!m_clientPlugins.contains(name))
            return;
        QList<QByteArray> prefixedMessages;
        prefixedMessages.reserve(messages.count());
        foreach (const QByteArray &message, messages) {
            QByteArray prefixed;
            QQmlDebugStream out(&prefixed, QIODevice::WriteOnly);
            out << name << message;
            prefixedMessages << prefixed;
        }

        m_connection->send(prefixedMessages);
    } else {
        m_connection->send(messages);
    }
}

template<class Action>
bool QQmlDebugServerImpl::enable(Action action)
{
#ifndef QT_NO_QML_DEBUGGER
    if (m_thread)
        return false;
    if (!action(this))
        return false;
    QMutexLocker locker(&m_helloMutex);
    m_thread->start();
    m_helloCondition.wait(&m_helloMutex); // wait for connection
    if (m_blockingMode && !m_gotHello)
        m_helloCondition.wait(&m_helloMutex); // wait for hello
    return true;
#else
    Q_UNUSED(action);
    return false;
#endif
}

void QQmlDebugServerImpl::wakeEngine(QQmlEngine *engine)
{
    // to be executed in debugger thread
    Q_ASSERT(QThread::currentThread() == thread());

    QMutexLocker locker(&m_helloMutex);
    m_engineConditions[engine].wake();
}

bool QQmlDebugServerImpl::EngineCondition::waitForServices(QMutex *locked, int num)
{
    Q_ASSERT_X(numServices == 0, Q_FUNC_INFO, "Request to wait again before previous wait finished");
    numServices = num;
    return numServices > 0 ? condition->wait(locked) : true;
}

void QQmlDebugServerImpl::EngineCondition::wake()
{
    if (--numServices == 0)
        condition->wakeAll();
    Q_ASSERT_X(numServices >=0, Q_FUNC_INFO, "Woken more often than #services.");
}

QT_END_NAMESPACE

#include "qqmldebugserver.moc"
