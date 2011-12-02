/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QLibraryInfo>

#include "QtDeclarative/private/qv8profilerservice_p.h"
#include "../shared/debugutil_p.h"
#include "../../shared/util.h"

#define PORT 13774
#define STR_PORT "13774"

class QV8ProfilerClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    QV8ProfilerClient(QDeclarativeDebugConnection *connection)
        : QDeclarativeDebugClient(QLatin1String("V8Profiler"), connection)
    {
    }

    void startProfiling(const QString &name) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << QByteArray("V8PROFILER") << QByteArray("start") << name;
        sendMessage(message);
    }

    void stopProfiling(const QString &name) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << QByteArray("V8PROFILER") << QByteArray("stop") << name;
        sendMessage(message);
    }

    void takeSnapshot() {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << QByteArray("V8SNAPSHOT") << QByteArray("full");
        sendMessage(message);
    }

    void deleteSnapshots() {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << QByteArray("V8SNAPSHOT") << QByteArray("delete");
        sendMessage(message);
    }

    QList<QV8ProfilerData> traceMessages;

signals:
    void complete();

protected:
    void messageReceived(const QByteArray &message);
};

class tst_QV8ProfilerService : public QObject
{
    Q_OBJECT

public:
    tst_QV8ProfilerService()
        : m_process(0)
        , m_connection(0)
        , m_client(0)
    {
    }

private:
    QDeclarativeDebugProcess *m_process;
    QDeclarativeDebugConnection *m_connection;
    QV8ProfilerClient *m_client;

    void connect(bool block);

private slots:
    void cleanup();

    void blockingConnectWithTraceEnabled();
    void blockingConnectWithTraceDisabled();
    void nonBlockingConnect();
};

void QV8ProfilerClient::messageReceived(const QByteArray &message)
{
    QByteArray msg = message;
    QDataStream stream(&msg, QIODevice::ReadOnly);

    int messageType;
    stream >> messageType;

    QVERIFY(messageType >= 0);
    QVERIFY(messageType < QV8ProfilerService::V8MaximumMessage);

    switch (messageType) {
    case QV8ProfilerService::V8Entry: {
        QV8ProfilerData entry;
        stream >> entry.filename >> entry.functionname >> entry.lineNumber >> entry.totalTime >> entry.selfTime >> entry.treeLevel;
        traceMessages.append(entry);
        break;
    }
    case QV8ProfilerService::V8Complete:
        emit complete();
        break;
    case QV8ProfilerService::V8Snapshot:
        break;
    default:
        QString failMessage = QString("Unknown message type: %1").arg(messageType);
        QFAIL(qPrintable(failMessage));
    }

    QVERIFY(stream.atEnd());
}

void tst_QV8ProfilerService::connect(bool block)
{
    const QString executable = QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene";
    QStringList arguments;

    if (block)
        arguments << QString("-qmljsdebugger=port:"STR_PORT",block");
    else
        arguments << QString("-qmljsdebugger=port:"STR_PORT);

    arguments << QString(TESTDATA(QLatin1String("test.qml")));

    m_process = new QDeclarativeDebugProcess(executable);
    m_process->start(QStringList() << arguments);
    if (!m_process->waitForSessionStart()) {
        QString failMsg = QString("Could not launch app '%1'.\nApplication output:\n%2").arg(
                    executable, m_process->output());
        QFAIL(qPrintable(failMsg));
    }

    QDeclarativeDebugConnection *m_connection = new QDeclarativeDebugConnection();
    m_client = new QV8ProfilerClient(m_connection);

    m_connection->connectToHost(QLatin1String("127.0.0.1"), PORT);
}

void tst_QV8ProfilerService::cleanup()
{
    delete m_process;
    delete m_connection;
    delete m_client;
}

void tst_QV8ProfilerService::blockingConnectWithTraceEnabled()
{
    connect(true);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->startProfiling("");
    m_client->stopProfiling("");
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: %1\n\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }
}

void tst_QV8ProfilerService::blockingConnectWithTraceDisabled()
{
    connect(true);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->startProfiling("");
    m_client->stopProfiling("");
    m_client->startProfiling("");
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: %1\n\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }
}

void tst_QV8ProfilerService::nonBlockingConnect()
{
    connect(false);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->stopProfiling("");
    m_client->startProfiling("");
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: %1\n\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }
}

QTEST_MAIN(tst_QV8ProfilerService)

#include "tst_qv8profilerservice.moc"
