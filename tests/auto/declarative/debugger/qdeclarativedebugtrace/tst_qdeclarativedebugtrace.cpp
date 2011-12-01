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

#include "QtDeclarative/private/qdeclarativedebugtrace_p.h"
#include "../shared/debugutil_p.h"
#include "../../shared/util.h"

#define PORT 13773
#define STR_PORT "13773"

class QDeclarativeDebugTraceClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    QDeclarativeDebugTraceClient(QDeclarativeDebugConnection *connection)
        : QDeclarativeDebugClient(QLatin1String("CanvasFrameRate"), connection)
    {
    }

    QList<QDeclarativeDebugData> traceMessages;

    void setTraceStatus(bool enabled) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << enabled;
        sendMessage(message);
    }

signals:
    void complete();

protected:
    void messageReceived(const QByteArray &message);
};

class tst_QDeclarativeDebugTrace : public QObject
{
    Q_OBJECT

public:
    tst_QDeclarativeDebugTrace()
        : m_process(0)
        , m_connection(0)
        , m_client(0)
    {
    }

private:
    QDeclarativeDebugProcess *m_process;
    QDeclarativeDebugConnection *m_connection;
    QDeclarativeDebugTraceClient *m_client;

    void connect(bool block);

private slots:
    void cleanup();

    void blockingConnectWithTraceEnabled();
    void blockingConnectWithTraceDisabled();
    void nonBlockingConnect();
};

void QDeclarativeDebugTraceClient::messageReceived(const QByteArray &message)
{
    QByteArray msg = message;
    QDataStream stream(&msg, QIODevice::ReadOnly);


    QDeclarativeDebugData data;
    data.time = -2;
    data.messageType = -1;
    data.detailType = -1;
    data.line = -1;
    data.framerate = -1;
    data.animationcount = -1;

    stream >> data.time >> data.messageType;

    QVERIFY(data.time >= -1);

    switch (data.messageType) {
    case (QDeclarativeDebugTrace::Event): {
        stream >> data.detailType;

        switch (data.detailType) {
        case QDeclarativeDebugTrace::AnimationFrame: {
            stream >> data.framerate >> data.animationcount;
            QVERIFY(data.framerate != -1);
            QVERIFY(data.animationcount != -1);
            break;
        }
        case QDeclarativeDebugTrace::FramePaint:
        case QDeclarativeDebugTrace::Mouse:
        case QDeclarativeDebugTrace::Key:
        case QDeclarativeDebugTrace::StartTrace:
        case QDeclarativeDebugTrace::EndTrace:
            break;
        default: {
            QString failMsg = QString("Unknown event type:") + data.detailType;
            QFAIL(qPrintable(failMsg));
            break;
        }
        }
        break;
    }
    case QDeclarativeDebugTrace::Complete: {
        emit complete();
        QVERIFY(stream.atEnd());
        return;
    }
    case QDeclarativeDebugTrace::RangeStart: {
        stream >> data.detailType;
        QVERIFY(data.detailType >= 0 && data.detailType < QDeclarativeDebugTrace::MaximumRangeType);
        break;
    }
    case QDeclarativeDebugTrace::RangeEnd: {
        stream >> data.detailType;
        QVERIFY(data.detailType >= 0 && data.detailType < QDeclarativeDebugTrace::MaximumRangeType);
        break;
    }
    case QDeclarativeDebugTrace::RangeData: {
        stream >> data.detailType >> data.detailData;
        QVERIFY(data.detailType >= 0 && data.detailType < QDeclarativeDebugTrace::MaximumRangeType);
        break;
    }
    case QDeclarativeDebugTrace::RangeLocation: {
        stream >> data.detailType >> data.detailData >> data.line;
        QVERIFY(data.detailType >= 0 && data.detailType < QDeclarativeDebugTrace::MaximumRangeType);
        QVERIFY(data.line >= -2);
        break;
    }
    default:
        QString failMsg = QString("Unknown message type:") + data.messageType;
        QFAIL(qPrintable(failMsg));
        break;
    }
    QVERIFY(stream.atEnd());
    traceMessages.append(data);
}

void tst_QDeclarativeDebugTrace::connect(bool block)
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
    m_client = new QDeclarativeDebugTraceClient(m_connection);

    m_connection->connectToHost(QLatin1String("127.0.0.1"), PORT);
}

void tst_QDeclarativeDebugTrace::cleanup()
{
    delete m_process;
    delete m_connection;
    delete m_client;
}

void tst_QDeclarativeDebugTrace::blockingConnectWithTraceEnabled()
{
    connect(true);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->setTraceStatus(true);
    m_client->setTraceStatus(false);
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: \n%1\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QDeclarativeDebugTrace::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QDeclarativeDebugTrace::EndTrace);
}

void tst_QDeclarativeDebugTrace::blockingConnectWithTraceDisabled()
{
    connect(true);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->setTraceStatus(false);
    m_client->setTraceStatus(true);
    m_client->setTraceStatus(false);
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: \n%1\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QDeclarativeDebugTrace::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QDeclarativeDebugTrace::EndTrace);
}

void tst_QDeclarativeDebugTrace::nonBlockingConnect()
{
    connect(false);
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->setTraceStatus(true);
    m_client->setTraceStatus(false);
    if (!QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(complete()))) {
        QString failMsg
                = QString("No trace received in time. App output: \n%1\n").arg(m_process->output());
        QFAIL(qPrintable(failMsg));
    }

    // must start with "StartTrace"
    QCOMPARE(m_client->traceMessages.first().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.first().detailType, (int)QDeclarativeDebugTrace::StartTrace);

    // must end with "EndTrace"
    QCOMPARE(m_client->traceMessages.last().messageType, (int)QDeclarativeDebugTrace::Event);
    QCOMPARE(m_client->traceMessages.last().detailType, (int)QDeclarativeDebugTrace::EndTrace);
}

QTEST_MAIN(tst_QDeclarativeDebugTrace)

#include "tst_qdeclarativedebugtrace.moc"
