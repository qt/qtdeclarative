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
#include <QSignalSpy>
#include <QTimer>
#include <QHostAddress>
#include <QDebug>
#include <QThread>

#include "../../../../src/plugins/qmltooling/qmldbg_inspector/qdeclarativeinspectorprotocol.h"
#include "../shared/debugutil_p.h"

using namespace QmlJSDebugger;

#define PORT 13772
#define STR_PORT "13772"

class QDeclarativeInspectorClient : public QDeclarativeDebugClient
{
    Q_OBJECT

public:
    QDeclarativeInspectorClient(QDeclarativeDebugConnection *connection)
        : QDeclarativeDebugClient(QLatin1String("QDeclarativeObserverMode"), connection)
        , m_showAppOnTop(false)
    {
    }

    bool showAppOnTop() const { return m_showAppOnTop; }
    void setShowAppOnTop(bool showOnTop);

signals:
    void showAppOnTopChanged();

protected:
    void messageReceived(const QByteArray &message);

private:
    bool m_showAppOnTop;
};

class tst_QDeclarativeInspector : public QObject
{
    Q_OBJECT

public:
    tst_QDeclarativeInspector()
        : m_process(0)
        , m_connection(0)
        , m_client(0)
    {
    }


private:
    QDeclarativeDebugProcess *m_process;
    QDeclarativeDebugConnection *m_connection;
    QDeclarativeInspectorClient *m_client;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void connect();
    void showAppOnTop();
};


void QDeclarativeInspectorClient::setShowAppOnTop(bool showOnTop)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);
    ds << InspectorProtocol::ShowAppOnTop << showOnTop;

    sendMessage(message);
}

void QDeclarativeInspectorClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);
    InspectorProtocol::Message type;
    ds >> type;

    switch (type) {
    case InspectorProtocol::ShowAppOnTop:
        ds >> m_showAppOnTop;
        emit showAppOnTopChanged();
        break;
    default:
        qDebug() << "Unhandled message " << (int)type;
    }
}

void tst_QDeclarativeInspector::initTestCase()
{
}

void tst_QDeclarativeInspector::cleanupTestCase()
{
}

void tst_QDeclarativeInspector::init()
{
    const QString executable = SRCDIR"/app/app";
    const QString argument = "-qmljsdebugger=port:"STR_PORT",block";

    m_process = new QDeclarativeDebugProcess(executable);
    m_process->start(QStringList() << argument);
    if (!m_process->waitForSessionStart()) {
        QFAIL(QString("Could not launch app '%1'.\nApplication output:\n%2").arg(executable, m_process->output()).toAscii());
    }

    QDeclarativeDebugConnection *m_connection = new QDeclarativeDebugConnection();
    m_client = new QDeclarativeInspectorClient(m_connection);

    m_connection->connectToHost(QLatin1String("127.0.0.1"), PORT);
}

void tst_QDeclarativeInspector::cleanup()
{
    delete m_process;
    delete m_connection;
    delete m_client;
}

void tst_QDeclarativeInspector::connect()
{
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);
}

void tst_QDeclarativeInspector::showAppOnTop()
{
    QTRY_COMPARE(m_client->status(), QDeclarativeDebugClient::Enabled);

    m_client->setShowAppOnTop(true);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(showAppOnTopChanged())));
    QCOMPARE(m_client->showAppOnTop(), true);

    m_client->setShowAppOnTop(false);
    QVERIFY(QDeclarativeDebugTest::waitForSignal(m_client, SIGNAL(showAppOnTopChanged())));
    QCOMPARE(m_client->showAppOnTop(), false);
}

QTEST_MAIN(tst_QDeclarativeInspector)

#include "tst_qdeclarativeinspector.moc"
