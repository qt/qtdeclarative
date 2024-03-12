// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/debugutil_p.h"
#include "../shared/qqmldebugprocess_p.h"
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <private/qqmldebugconnection_p.h>
#include <private/qqmlinspectorclient_p.h>

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qlibraryinfo.h>
#include <QtNetwork/qhostaddress.h>

class tst_QQmlInspector : public QQmlDebugTest
{
    Q_OBJECT

public:
    tst_QQmlInspector();

private:
    ConnectResult startQmlProcess(const QString &qmlFile, bool restrictMode = true);
    void checkAnimationSpeed(int targetMillisPerDegree);
    QList<QQmlDebugClient *> createClients() override;
    QQmlDebugProcess *createProcess(const QString &executable) override;

    QPointer<QQmlInspectorClient> m_client;
    QPointer<QQmlInspectorResultRecipient> m_recipient;

private slots:
    void connect_data();
    void connect();
    void setAnimationSpeed();
    void showAppOnTop();
};

tst_QQmlInspector::tst_QQmlInspector()
    : QQmlDebugTest(QT_QMLTEST_DATADIR)
{
}

QQmlDebugTest::ConnectResult tst_QQmlInspector::startQmlProcess(const QString &qmlFile,
                                                                bool restrictServices)
{
    return QQmlDebugTest::connectTo(QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qml",
                                  restrictServices ? QStringLiteral("QmlInspector") : QString(),
                                  testFile(qmlFile), true);
}

void tst_QQmlInspector::checkAnimationSpeed(int targetMillisPerDegree)
{
    const QString markerString = QStringLiteral("ms/degrees");

    // Funny things can happen with time and VMs. Also the change might take a while to propagate.
    // Thus, we wait until we either have 3 passes or 3 failures in a row, or 10 loops have passed.

    int numFailures = 0;
    int numPasses = 0;

    for (int i = 0; i < 10; ++i) {
        QString output = m_process->output();
        int position = output.size();
        do {
            QVERIFY(QQmlDebugTest::waitForSignal(m_process, SIGNAL(readyReadStandardOutput())));
            output = m_process->output();
        } while (!output.mid(position).contains(markerString));


        QStringList words = output.split(QLatin1Char(' '));
        const int marker = words.lastIndexOf(markerString);
        QVERIFY(marker > 1);
        const double degrees = words[marker - 1].toDouble();
        const int milliseconds = words[marker - 2].toInt();
        const double millisecondsPerDegree = milliseconds / degrees;

        if (millisecondsPerDegree > targetMillisPerDegree - 3
                || millisecondsPerDegree < targetMillisPerDegree + 3) {
            if (++numPasses == 3)
                return; // pass
            numFailures = 0;
        } else {
            QVERIFY2(++numFailures < 3,
                     QString("3 consecutive failures when checking for %1 milliseconds per degree")
                     .arg(targetMillisPerDegree).toLocal8Bit().constData());
            numPasses = 0;
        }
    }

    QFAIL(QString("Animation speed won't settle to %1 milliseconds per degree")
          .arg(targetMillisPerDegree).toLocal8Bit().constData());
}

QList<QQmlDebugClient *> tst_QQmlInspector::createClients()
{
    m_client = new QQmlInspectorClient(m_connection);
    m_recipient = new QQmlInspectorResultRecipient(m_client);
    QObject::connect(m_client.data(), &QQmlInspectorClient::responseReceived,
                     m_recipient.data(), &QQmlInspectorResultRecipient::recordResponse);
    return QList<QQmlDebugClient *>({m_client});
}

QQmlDebugProcess *tst_QQmlInspector::createProcess(const QString &executable)
{
    QQmlDebugProcess *process = QQmlDebugTest::createProcess(executable);
    // Make sure the animation timing is exact
    process->addEnvironment(QLatin1String("QSG_RENDER_LOOP=basic"));
    return process;
}

void tst_QQmlInspector::connect_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("restrictMode");
    QTest::newRow("rectangle/unrestricted") << "qtquick2.qml" << false;
    QTest::newRow("rectangle/restricted")   << "qtquick2.qml" << true;
    QTest::newRow("window/unrestricted")    << "window.qml"   << false;
    QTest::newRow("window/restricted")      << "window.qml"   << true;
}

void tst_QQmlInspector::connect()
{
    QFETCH(QString, file);
    QFETCH(bool, restrictMode);
    QCOMPARE(startQmlProcess(file, restrictMode), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    int requestId = m_client->setInspectToolEnabled(true);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);

    requestId = m_client->setInspectToolEnabled(false);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
}

void tst_QQmlInspector::showAppOnTop()
{
    QCOMPARE(startQmlProcess("qtquick2.qml"), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);

    int requestId = m_client->setShowAppOnTop(true);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);

    requestId = m_client->setShowAppOnTop(false);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
}

void tst_QQmlInspector::setAnimationSpeed()
{
    QCOMPARE(startQmlProcess("qtquick2.qml"), ConnectSuccess);
    QVERIFY(m_client);
    QTRY_COMPARE(m_client->state(), QQmlDebugClient::Enabled);
    checkAnimationSpeed(10);

    int requestId = m_client->setAnimationSpeed(0.5);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
    checkAnimationSpeed(5);

    requestId = m_client->setAnimationSpeed(2.0);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
    checkAnimationSpeed(20);

    requestId = m_client->setAnimationSpeed(1.0);
    QTRY_COMPARE(m_recipient->lastResponseId, requestId);
    QVERIFY(m_recipient->lastResult);
    checkAnimationSpeed(10);
}

QTEST_MAIN(tst_QQmlInspector)

#include "tst_qqmlinspector.moc"
