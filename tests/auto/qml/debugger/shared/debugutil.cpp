// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debugutil_p.h"
#include "qqmldebugprocess_p.h"

#include <private/qqmldebugconnection_p.h>

#include <QtCore/qtimer.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

QQmlDebugTest::QQmlDebugTest(const char *qmlTestDataDir, FailOnWarningsPolicy failOnWarningsPolicy)
    : QQmlDataTest(qmlTestDataDir, failOnWarningsPolicy)
{
}

bool QQmlDebugTest::waitForSignal(QObject *sender, const char *member, int timeout)
{
    QSignalSpy spy(sender, member);

    // Do not use spy.wait(). We want to avoid nested event loops.
    if (QTest::qWaitFor([&]() { return spy.size() > 0; }, timeout))
        return true;

    qWarning("waitForSignal %s timed out after %d ms", member, timeout);
    return false;
}

QList<QQmlDebugClient *> QQmlDebugTest::createOtherClients(QQmlDebugConnection *connection)
{
    QList<QQmlDebugClient *> ret;

    static const auto debuggerServices
            = QStringList({"V8Debugger", "QmlDebugger", "DebugMessages"});
    static const auto inspectorServices
            = QStringList({"QmlInspector"});
    static const auto profilerServices
            = QStringList({"CanvasFrameRate", "EngineControl", "DebugMessages"});

    for (const QString &service : debuggerServices) {
        if (!connection->client(service))
            ret << new QQmlDebugClient(service, connection);
    }
    for (const QString &service : inspectorServices) {
        if (!connection->client(service))
            ret << new QQmlDebugClient(service, connection);
    }
    for (const QString &service : profilerServices) {
        if (!connection->client(service))
            ret << new QQmlDebugClient(service, connection);
    }
    return ret;
}

QString QQmlDebugTest::clientStateString(const QQmlDebugClient *client)
{
    if (!client)
        return QLatin1String("null");

    switch (client->state()) {
    case QQmlDebugClient::NotConnected: return QLatin1String("Not connected");
    case QQmlDebugClient::Unavailable: return QLatin1String("Unavailable");
    case QQmlDebugClient::Enabled: return QLatin1String("Enabled");
    default: return QLatin1String("Invalid");
    }

}

QString QQmlDebugTest::connectionStateString(const QQmlDebugConnection *connection)
{
    if (!connection)
        return QLatin1String("null");

    return connection->isConnected() ? QLatin1String("connected") : QLatin1String("not connected");
}

QQmlDebugTestClient::QQmlDebugTestClient(const QString &s, QQmlDebugConnection *c)
    : QQmlDebugClient(s, c)
{
    connect(this, &QQmlDebugClient::stateChanged, this, [this](QQmlDebugClient::State newState) {
        QCOMPARE(newState, state());
    });
}

QByteArray QQmlDebugTestClient::waitForResponse()
{
    lastMsg.clear();
    QQmlDebugTest::waitForSignal(this, SIGNAL(serverMessage(QByteArray)));
    if (lastMsg.isEmpty()) {
        qWarning() << "no response from server!";
        return QByteArray();
    }
    return lastMsg;
}

void QQmlDebugTestClient::messageReceived(const QByteArray &ba)
{
    lastMsg = ba;
    emit serverMessage(ba);
}

QQmlDebugTest::ConnectResult QQmlDebugTest::connectTo(
        const QString &executable, const QString &services, const QString &extraArgs,
        bool block)
{
    QStringList arguments;
    arguments << QString::fromLatin1("-qmljsdebugger=port:13773,13783%3%4")
                 .arg(block ? QStringLiteral(",block") : QString())
                 .arg(services.isEmpty() ? services : (QStringLiteral(",services:") + services))
              << extraArgs;

    m_process = createProcess(executable);
    if (!m_process)
        return ProcessFailed;

    m_process->start(QStringList() << arguments);
    if (!m_process->waitForSessionStart())
        return SessionFailed;

    m_connection = createConnection();
    if (!m_connection)
        return ConnectionFailed;

    m_clients = createClients();
    if (m_clients.contains(nullptr))
        return ClientsFailed;

    ClientStateHandler stateHandler(m_clients, createOtherClients(m_connection), services.isEmpty()
                                    ? QQmlDebugClient::Enabled : QQmlDebugClient::Unavailable);

    QSignalSpy okSpy(&stateHandler, &ClientStateHandler::allOk);
    QSignalSpy disconnectSpy(m_connection, &QQmlDebugConnection::disconnected);
    m_connection->connectToHost(QLatin1String("127.0.0.1"), m_process->debugPort());
    if (!QTest::qWaitFor([&](){ return okSpy.size() > 0 || disconnectSpy.size() > 0; }, 5000))
        return ConnectionTimeout;

    if (!stateHandler.allEnabled())
        return EnableFailed;

    if (!stateHandler.othersAsExpected())
        return RestrictFailed;

    return ConnectSuccess;
}

QList<QQmlDebugClient *> QQmlDebugTest::createClients()
{
    return QList<QQmlDebugClient *>();
}

QQmlDebugProcess *QQmlDebugTest::createProcess(const QString &executable)
{
    return new QQmlDebugProcess(executable, this);
}

QQmlDebugConnection *QQmlDebugTest::createConnection()
{
    return new QQmlDebugConnection(this);
}

void QQmlDebugTest::cleanup()
{
    if (QTest::currentTestFailed()) {
        const QString null = QStringLiteral("null");

        qDebug() << "Process State:" << (m_process ? m_process->stateString() : null);
        qDebug() << "Application Output:" << (m_process ? m_process->output() : null);
        qDebug() << "Connection State:" << QQmlDebugTest::connectionStateString(m_connection);
        for (QQmlDebugClient *client : m_clients) {
            if (client)
                qDebug() << client->name() << "State:" << QQmlDebugTest::clientStateString(client);
            else
                qDebug() << "Failed Client:" << null;
        }
    }

    qDeleteAll(m_clients);
    m_clients.clear();

    delete m_connection;
    m_connection = nullptr;

    if (m_process) {
        m_process->stop();
        delete m_process;
        m_process = nullptr;
    }
}

ClientStateHandler::ClientStateHandler(const QList<QQmlDebugClient *> &clients,
                                       const QList<QQmlDebugClient *> &others,
                                       QQmlDebugClient::State expectedOthers) :
    m_clients(clients), m_others(others), m_expectedOthers(expectedOthers)
{
    for (QQmlDebugClient *client : m_clients) {
        QObject::connect(client, &QQmlDebugClient::stateChanged,
                         this, &ClientStateHandler::checkStates);
    }
    for (QQmlDebugClient *client : m_others) {
        QObject::connect(client, &QQmlDebugClient::stateChanged,
                         this, &ClientStateHandler::checkStates);
    }
}

ClientStateHandler::~ClientStateHandler()
{
    qDeleteAll(m_others);
}

void ClientStateHandler::checkStates()
{
    for (QQmlDebugClient *client : m_clients) {
        if (client->state() != QQmlDebugClient::Enabled)
            return;
    }

    m_allEnabled = true;

    for (QQmlDebugClient *other : m_others) {
        if (other->state() != m_expectedOthers)
            return;
    }

    m_othersAsExpected = true;
    emit allOk();
}

QString debugJsServerPath(const QString &selfPath)
{
    static const char *debugserver = "qqmldebugjsserver";
    QString appPath = QCoreApplication::applicationDirPath();
    const int position = appPath.lastIndexOf(selfPath);
    return (position == -1 ? appPath : appPath.replace(position, selfPath.size(), debugserver))
            + "/" + debugserver;
}

#include <moc_debugutil_p.cpp>
