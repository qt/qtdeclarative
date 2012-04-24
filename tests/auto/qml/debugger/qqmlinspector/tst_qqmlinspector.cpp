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

#include "../shared/debugutil_p.h"

#define PORT 3772
#define STR_PORT "3772"

class QQmlInspectorClient : public QQmlDebugClient
{
    Q_OBJECT

public:
    QQmlInspectorClient(QQmlDebugConnection *connection)
        : QQmlDebugClient(QLatin1String("QmlInspector"), connection)
        , m_showAppOnTop(false)
        , m_requestId(0)
        , m_requestResult(false)
    {
    }

    void setShowAppOnTop(bool showOnTop);

signals:
    void responseReceived();

protected:
    void messageReceived(const QByteArray &message);

private:
    bool m_showAppOnTop;
    int m_requestId;

public:
    bool m_requestResult;
};

class tst_QQmlInspector : public QObject
{
    Q_OBJECT

public:
    tst_QQmlInspector()
        : m_process(0)
        , m_connection(0)
        , m_client(0)
    {
    }


private:
    QQmlDebugProcess *m_process;
    QQmlDebugConnection *m_connection;
    QQmlInspectorClient *m_client;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void connect();
    void showAppOnTop();
};


void QQmlInspectorClient::setShowAppOnTop(bool showOnTop)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << QByteArray("request") << m_requestId++
       << QByteArray("showAppOnTop") << showOnTop;

    sendMessage(message);
}

void QQmlInspectorClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);
    QByteArray type;
    ds >> type;

    if (type != QByteArray("response")) {
        qDebug() << "Unhandled message of type" << type;
        return;
    }

    m_requestResult = false;
    int requestId;
    ds >> requestId >> m_requestResult;
    emit responseReceived();
}

void tst_QQmlInspector::initTestCase()
{
}

void tst_QQmlInspector::cleanupTestCase()
{
}

void tst_QQmlInspector::init()
{
#if defined(Q_OS_WIN)
    const QString executable = "app\\app.exe";
#else
    const QString executable = "app/app";
#endif
    const QString argument = "-qmljsdebugger=port:"STR_PORT",block";

    m_process = new QQmlDebugProcess(executable);
    m_process->start(QStringList() << argument);
    QVERIFY2(m_process->waitForSessionStart(),
             "Could not launch application, or did not get 'Waiting for connection'.");

    QQmlDebugConnection *m_connection = new QQmlDebugConnection();
    m_client = new QQmlInspectorClient(m_connection);

    m_connection->connectToHost(QLatin1String("127.0.0.1"), PORT);
}

void tst_QQmlInspector::cleanup()
{
    if (QTest::currentTestFailed()) {
        qDebug() << "Process State:" << m_process->state();
        qDebug() << "Application Output:" << m_process->output();
    }
    delete m_process;
    delete m_connection;
    delete m_client;
}

void tst_QQmlInspector::connect()
{
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
}

void tst_QQmlInspector::showAppOnTop()
{
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setShowAppOnTop(true);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));
    QCOMPARE(m_client->m_requestResult, true);

    m_client->setShowAppOnTop(false);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));
    QCOMPARE(m_client->m_requestResult, true);
}

QTEST_MAIN(tst_QQmlInspector)

#include "tst_qqmlinspector.moc"
