/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

#include <QtQml/qqmlengine.h>

#include <private/qqmldebugservice_p.h>

#include "../../../shared/util.h"
#include "debugutil_p.h"
#include "qqmldebugclient.h"

#define PORT 13769
#define STR_PORT "13769"

class tst_QQmlDebugService : public QObject
{
    Q_OBJECT
private:
    QQmlDebugConnection *m_conn;

private slots:
    void initTestCase();

    void name();
    void version();
    void state();
    void sendMessage();
    void idForObject();
    void objectForId();
    void objectToString();
};

void tst_QQmlDebugService::initTestCase()
{
    const QString waitingMsg = QString("QML Debugger: Waiting for connection on port %1...").arg(PORT);
    QTest::ignoreMessage(QtDebugMsg, waitingMsg.toAscii().constData());
    new QQmlEngine(this);

    m_conn = new QQmlDebugConnection(this);


    QTest::ignoreMessage(QtDebugMsg, "QML Debugger: Connection established.");
    for (int i = 0; i < 50; ++i) {
        // try for 5 seconds ...
        m_conn->connectToHost("127.0.0.1", PORT);
        if (m_conn->waitForConnected())
            break;
        QTest::qSleep(100);
    }
    QVERIFY(m_conn->isConnected());

    QTRY_VERIFY(QQmlDebugService::hasDebuggingClient());
}

void tst_QQmlDebugService::name()
{
    QString name = "tst_QQmlDebugService::name()";

    QQmlDebugService service(name, 1);
    QCOMPARE(service.name(), name);
}

void tst_QQmlDebugService::version()
{
    QString name = "tst_QQmlDebugService::name()";

    QQmlDebugService service(name, 2);
    QCOMPARE(service.version(), 2.0f);
}

void tst_QQmlDebugService::state()
{
    QQmlDebugTestService service("tst_QQmlDebugService::state()");
    QCOMPARE(service.state(), QQmlDebugService::Unavailable);

    {
        QQmlDebugTestClient client("tst_QQmlDebugService::state()", m_conn);
        QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
        QTRY_COMPARE(service.state(), QQmlDebugService::Enabled);
    }


    QTRY_COMPARE(service.state(), QQmlDebugService::Unavailable);

    QTest::ignoreMessage(QtWarningMsg, "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService::state()\" ");
    QQmlDebugTestService duplicate("tst_QQmlDebugService::state()");
    QCOMPARE(duplicate.state(), QQmlDebugService::NotConnected);
}

void tst_QQmlDebugService::sendMessage()
{
    QQmlDebugTestService service("tst_QQmlDebugService::sendMessage()");
    QQmlDebugTestClient client("tst_QQmlDebugService::sendMessage()", m_conn);

    QByteArray msg = "hello!";

    QTRY_COMPARE(client.state(), QQmlDebugClient::Enabled);
    QTRY_COMPARE(service.state(), QQmlDebugService::Enabled);

    client.sendMessage(msg);
    QByteArray resp = client.waitForResponse();
    QCOMPARE(resp, msg);

    QTest::ignoreMessage(QtWarningMsg, "QQmlDebugService: Conflicting plugin name \"tst_QQmlDebugService::sendMessage()\" ");
    QQmlDebugTestService duplicate("tst_QQmlDebugService::sendMessage()");
    duplicate.sendMessage("msg");
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

void tst_QQmlDebugService::objectToString()
{
    QCOMPARE(QQmlDebugService::objectToString(0), QString("NULL"));

    QObject *obj = new QObject;
    QCOMPARE(QQmlDebugService::objectToString(obj), QString("QObject: <unnamed>"));

    obj->setObjectName("Hello");
    QCOMPARE(QQmlDebugService::objectToString(obj), QString("QObject: Hello"));
    delete obj;
}


int main(int argc, char *argv[])
{
    int _argc = argc + 1;
    char **_argv = new char*[_argc];
    for (int i = 0; i < argc; ++i)
        _argv[i] = argv[i];
    char arg[] = "-qmljsdebugger=port:" STR_PORT;
    _argv[_argc - 1] = arg;

    QGuiApplication app(_argc, _argv);
    tst_QQmlDebugService tc;
    return QTest::qExec(&tc, _argc, _argv);
    delete _argv;
}

#include "tst_qqmldebugservice.moc"
