/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtDeclarative/private/qdeclarativedebugclient_p.h>

//QDeclarativeDebugTest
#include "../shared/debugutil_p.h"
#include "../../../shared/util.h"

#include <QtCore/QString>
#include <QtTest/QtTest>

const char *NORMALMODE = "-qmljsdebugger=port:3777,block";
const char *QMLFILE = "test.qml";

class QDeclarativeDebugMsgClient;
class tst_QDebugMessageService : public QDeclarativeDataTest
{
    Q_OBJECT

public:
    tst_QDebugMessageService();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void retrieveDebugOutput();

private:
    QDeclarativeDebugProcess *m_process;
    QDeclarativeDebugMsgClient *m_client;
    QDeclarativeDebugConnection *m_connection;
};

struct LogEntry {
    LogEntry(QtMsgType _type, QString _message)
        : type(_type), message(_message) {}

    QtMsgType type;
    QString message;

    QString toString() const { return QString::number(type) + ": " + message; }
};

class QDeclarativeDebugMsgClient : public QDeclarativeDebugClient
{
    Q_OBJECT
public:
    QDeclarativeDebugMsgClient(QDeclarativeDebugConnection *connection)
        : QDeclarativeDebugClient(QLatin1String("DebugMessages"), connection)
    {
    }

    QList<LogEntry> logBuffer;

protected:
    //inherited from QDeclarativeDebugClient
    void statusChanged(Status status);
    void messageReceived(const QByteArray &data);

signals:
    void enabled();
    void debugOutput();
};

void QDeclarativeDebugMsgClient::statusChanged(Status status)
{
    if (status == Enabled) {
        emit enabled();
    }
}

void QDeclarativeDebugMsgClient::messageReceived(const QByteArray &data)
{
    QDataStream ds(data);
    QByteArray command;
    ds >> command;

    if (command == "MESSAGE") {
        int type;
        QByteArray message;
        ds >> type >> message;
        QVERIFY(ds.atEnd());

        QVERIFY(type >= QtDebugMsg);
        QVERIFY(type <= QtFatalMsg);

        logBuffer << LogEntry((QtMsgType)type, QString::fromUtf8(message));
        emit debugOutput();
    } else {
        QFAIL("Unknown message");
    }
}

tst_QDebugMessageService::tst_QDebugMessageService()
{
}

void tst_QDebugMessageService::initTestCase()
{
    QDeclarativeDataTest::initTestCase();
    m_process = 0;
    m_client = 0;
    m_connection = 0;
}

void tst_QDebugMessageService::cleanupTestCase()
{
    if (m_process)
        delete m_process;

    if (m_client)
        delete m_client;

    if (m_connection)
        delete m_connection;
}

void tst_QDebugMessageService::init()
{
    m_connection = new QDeclarativeDebugConnection();
    m_process = new QDeclarativeDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene");
    m_client = new QDeclarativeDebugMsgClient(m_connection);

    m_process->setEnvironment(QProcess::systemEnvironment() << "QML_CONSOLE_EXTENDED=1");
    m_process->start(QStringList() << QLatin1String(NORMALMODE) << QDeclarativeDataTest::instance()->testFile(QMLFILE));
    if (!m_process->waitForSessionStart()) {
        QFAIL(QString("Could not launch app. Application output: \n%1").arg(m_process->output()).toAscii());
    }

    m_connection->connectToHost("127.0.0.1", 3777);
    QVERIFY(m_connection->waitForConnected());

    QVERIFY(QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(enabled())));
}

void tst_QDebugMessageService::cleanup()
{
    if (QTest::currentTestFailed())
        qDebug() << m_process->output();
    if (m_process)
        delete m_process;

    if (m_client)
        delete m_client;

    if (m_connection)
        delete m_connection;

    m_process = 0;
    m_client = 0;
    m_connection = 0;
}

void tst_QDebugMessageService::retrieveDebugOutput()
{
    if (m_client->logBuffer.isEmpty())
        QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(debugOutput()));
    QVERIFY(!m_client->logBuffer.isEmpty());


    QString msg = QString::fromLatin1("console.log (%1:%2)").arg(
                QUrl::fromLocalFile(QDeclarativeDataTest::instance()->testFile(QMLFILE)).toString()).arg(48);
    QCOMPARE(m_client->logBuffer.last().toString(),
             LogEntry(QtDebugMsg, msg).toString());
}

QTEST_MAIN(tst_QDebugMessageService)

#include "tst_qdebugmessageservice.moc"
