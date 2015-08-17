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
#include <QtCore/QLibraryInfo>

#include "../shared/debugutil_p.h"
#include "../../../shared/util.h"
#include "qqmlinspectorclient.h"

#define STR_PORT_FROM "3772"
#define STR_PORT_TO "3782"



class tst_QQmlInspector : public QQmlDataTest
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
    void startQmlsceneProcess(const char *qmlFile, bool restrictMode = true);

private:
    QQmlDebugProcess *m_process;
    QQmlDebugConnection *m_connection;
    QQmlInspectorClient *m_client;

private slots:
    void cleanup();

    void connect_data();
    void connect();
    void showAppOnTop();
    void reloadQml();
    void reloadQmlWindow();
};

void tst_QQmlInspector::startQmlsceneProcess(const char * /* qmlFile */, bool restrictServices)
{
    const QString argument = QString::fromLatin1("-qmljsdebugger=port:%1,%2,block%3")
            .arg(STR_PORT_FROM).arg(STR_PORT_TO)
            .arg(restrictServices ? QStringLiteral(",services:QmlInspector") : QString());

    // ### This should be using qml instead of qmlscene, but can't because of QTBUG-33376 (same as the XFAIL testcase)
    m_process = new QQmlDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene", this);
    m_process->start(QStringList() << argument << testFile("qtquick2.qml"));
    QVERIFY2(m_process->waitForSessionStart(),
             "Could not launch application, or did not get 'Waiting for connection'.");

    m_connection = new QQmlDebugConnection();
    m_client = new QQmlInspectorClient(m_connection);

    m_connection->connectToHost(QLatin1String("127.0.0.1"), m_process->debugPort());
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

}

void tst_QQmlInspector::cleanup()
{
    if (QTest::currentTestFailed()) {
        qDebug() << "Process State:" << m_process->state();
        qDebug() << "Application Output:" << m_process->output();
    }
    delete m_client;
    m_client = 0;
    delete m_connection;
    m_connection = 0;
    delete m_process;
    m_process = 0;
}

void tst_QQmlInspector::connect_data()
{
    QTest::addColumn<bool>("restrictMode");
    QTest::newRow("unrestricted") << false;
    QTest::newRow("restricted") << true;
}

void tst_QQmlInspector::connect()
{
    QFETCH(bool, restrictMode);
    startQmlsceneProcess("qtquick2.qml", restrictMode);
}

void tst_QQmlInspector::showAppOnTop()
{
    startQmlsceneProcess("qtquick2.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    m_client->setShowAppOnTop(true);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));
    QCOMPARE(m_client->m_requestResult, true);

    m_client->setShowAppOnTop(false);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));
    QCOMPARE(m_client->m_requestResult, true);
}

void tst_QQmlInspector::reloadQml()
{
    startQmlsceneProcess("qtquick2.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    QByteArray fileContents;

    QFile file(testFile("changes.txt"));
    if (file.open(QFile::ReadOnly))
        fileContents = file.readAll();
    file.close();

    QHash<QString, QByteArray> changesHash;
    changesHash.insert("qtquick2.qml", fileContents);

    m_client->reloadQml(changesHash);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));

    QTRY_COMPARE(m_process->output().contains(
                 QString("version 2.0")), true);

    QCOMPARE(m_client->m_requestResult, true);
    QCOMPARE(m_client->m_reloadRequestId, m_client->m_responseId);
}

void tst_QQmlInspector::reloadQmlWindow()
{
    startQmlsceneProcess("window.qml");
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    QByteArray fileContents;

    QFile file(testFile("changes.txt"));
    if (file.open(QFile::ReadOnly))
        fileContents = file.readAll();
    file.close();

    QHash<QString, QByteArray> changesHash;
    changesHash.insert("window.qml", fileContents);

    m_client->reloadQml(changesHash);
    QVERIFY(QQmlDebugTest::waitForSignal(m_client, SIGNAL(responseReceived())));

    QEXPECT_FAIL("", "cannot debug with a QML file containing a top-level Window", Abort); // QTBUG-33376
    // TODO: remove the timeout once we don't expect it to fail anymore.
    QTRY_VERIFY_WITH_TIMEOUT(m_process->output().contains(QString("version 2.0")), 1);

    QCOMPARE(m_client->m_requestResult, true);
    QCOMPARE(m_client->m_reloadRequestId, m_client->m_responseId);
}

QTEST_MAIN(tst_QQmlInspector)

#include "tst_qqmlinspector.moc"
