/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qtest.h>
#include <QSignalSpy>
#include <QTimer>
#include <QHostAddress>
#include <QDebug>
#include <QThread>
#include <QLibraryInfo>

#include <QtQml/qqmlengine.h>

#include "../../../shared/util.h"
#include "debugutil_p.h"
#include "qqmldebugclient.h"
#include "qqmldebugtestservice.h"
#include <private/qqmldebugconnector_p.h>

#define PORT 3769
#define STR_PORT "3769"

class tst_QQmlDebugService : public QQmlDataTest
{
    Q_OBJECT
private:
    QQmlDebugConnection *m_conn;
    QQmlDebugTestService *m_service;

private slots:

    void initTestCase();
    void checkPortRange();
    void name();
    void version();
    void state();
    void sendMessage();
    void idForObject();
    void objectForId();
    void checkSupportForDataStreamVersion();
    void checkSupportForOldDataStreamVersion();
};

void tst_QQmlDebugService::initTestCase()
{
    QQmlDataTest::initTestCase();
    QQmlDebugConnector::setPluginKey(QLatin1String("QQmlDebugServer"));
    QTest::ignoreMessage(QtWarningMsg,
                         "QML debugger: Cannot set plugin key after loading the plugin.");
    m_service = new QQmlDebugTestService("tst_QQmlDebugService", 2);

    const QString waitingMsg = QString("QML Debugger: Waiting for connection on port %1...").arg(PORT);
    QTest::ignoreMessage(QtDebugMsg, waitingMsg.toLatin1().constData());
    QQmlDebuggingEnabler::startTcpDebugServer(PORT);

    new QQmlEngine(this);

    m_conn = new QQmlDebugConnection(this);

    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }
    QVERIFY(m_conn->isConnected());
}

void tst_QQmlDebugService::checkPortRange()
{
    QQmlDebugConnection *connection1 = new QQmlDebugConnection();
    QQmlDebugProcess *process1 = new QQmlDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene", this);

    process1->start(QStringList() << QLatin1String("-qmljsdebugger=port:3782,3792") << testFile("test.qml"));

    if (!process1->waitForSessionStart())
        QFAIL("could not launch application, or did not get 'Waiting for connection'.");

    const int port1 = process1->debugPort();
    connection1->connectToHost("127.0.0.1", port1);
    if (!connection1->waitForConnected())
        QFAIL("could not connect to host!");

    // Second instance
    QQmlDebugConnection *connection2 = new QQmlDebugConnection();
    QQmlDebugProcess *process2 = new QQmlDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene", this);

    process2->start(QStringList() << QLatin1String("-qmljsdebugger=port:3782,3792") << testFile("test.qml"));

    if (!process2->waitForSessionStart())
        QFAIL("could not launch application, or did not get 'Waiting for connection'.");

    const int port2 = process2->debugPort();
    connection2->connectToHost("127.0.0.1", port2);
    if (!connection2->waitForConnected())
        QFAIL("could not connect to host!");

    delete connection1;
    delete process1;
    delete connection2;
    delete process2;
}

void tst_QQmlDebugService::name()
{
    QCOMPARE(m_service->name(), QLatin1String("tst_QQmlDebugService"));
}

void tst_QQmlDebugService::version()
{
    QCOMPARE(m_service->version(), 2.0f);
}

void tst_QQmlDebugService::state()
{
    QCOMPARE(m_service->state(), QQmlDebugService::Unavailable);

    {
        QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);
        QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
        QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);
    }

    QTRY_COMPARE(m_service->state(), QQmlDebugService::Unavailable);

    // We can do this because it will never addService()
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService\"");
    QQmlDebugTestService duplicate("tst_QQmlDebugService");
    QCOMPARE(duplicate.state(), QQmlDebugService::NotConnected);
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Plugin \"tst_QQmlDebugService\" is not registered.");
}

void tst_QQmlDebugService::sendMessage()
{
    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);

    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService\"");
    QQmlDebugTestService duplicate("tst_QQmlDebugService");
    emit duplicate.messageToClient(duplicate.name(), "msg");
    QTest::ignoreMessage(QtWarningMsg,
                         "QQmlDebugService: Plugin \"tst_QQmlDebugService\" is not registered.");
}

void tst_QQmlDebugService::checkSupportForDataStreamVersion()
{
    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);
    QCOMPARE(m_conn->dataStreamVersion(), int(QDataStream::Qt_5_0));
}

void tst_QQmlDebugService::idForObject()
{
    QCOMPARE(QQmlDebugService::idForObject(0), -1);

    QObject *objA = new QObject;

    int idA = QQmlDebugService::idForObject(objA);
    QVERIFY(idA >= 0);
    QCOMPARE(QQmlDebugService::objectForId(idA), objA);

    int idAA = QQmlDebugService::idForObject(objA);
    QCOMPARE(idAA, idA);

    QObject *objB = new QObject;
    int idB = QQmlDebugService::idForObject(objB);
    QVERIFY(idB != idA);
    QCOMPARE(QQmlDebugService::objectForId(idB), objB);

    delete objA;
    delete objB;
}

void tst_QQmlDebugService::objectForId()
{
    QCOMPARE(QQmlDebugService::objectForId(-1), static_cast<QObject*>(0));
    QCOMPARE(QQmlDebugService::objectForId(1), static_cast<QObject*>(0));

    QObject *obj = new QObject;
    int id = QQmlDebugService::idForObject(obj);
    QCOMPARE(QQmlDebugService::objectForId(id), obj);

    delete obj;
    QCOMPARE(QQmlDebugService::objectForId(id), static_cast<QObject*>(0));
}

void tst_QQmlDebugService::checkSupportForOldDataStreamVersion()
{
    //create a new connection;
    delete m_conn;
    m_conn = new QQmlDebugConnection(this);
    m_conn->setDataStreamVersion(QDataStream::Qt_4_7);
    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }
    QVERIFY(m_conn->isConnected());

    QQmlDebugTestClient client("tst_QQmlDebugService", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(m_service->state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);
    QCOMPARE(m_conn->dataStreamVersion(), int(QDataStream::Qt_4_7));
}

QTEST_MAIN(tst_QQmlDebugService)

#include "tst_qqmldebugservice.moc"
