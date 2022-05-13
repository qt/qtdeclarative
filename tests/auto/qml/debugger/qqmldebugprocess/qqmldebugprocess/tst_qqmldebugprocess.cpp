// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qqmldebugprocess_p.h>
#include <QtTest>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qscopedpointer.h>

class tst_QQmlDebugProcess : public QObject
{
    Q_OBJECT

private slots:
    void sessionStart_data();
    void sessionStart();
};

void tst_QQmlDebugProcess::sessionStart_data()
{
    QTest::addColumn<int>("delay");
    QTest::addColumn<QString>("arg");
    QTest::addColumn<bool>("sessionExpected");
    QTest::addColumn<bool>("outputExpected");

    QTest::addRow("synchronous / waiting") << -1
                                           << "QML Debugger: Waiting for connection on port 2423..."
                                           << true << false;
    QTest::addRow("synchronous / failed")  << -1 << "QML Debugger: Unable to listen to port 242."
                                           << false << false;
    QTest::addRow("synchronous / unknown") << -1 << "QML Debugger: You don't know this string."
                                           << false << true;
    QTest::addRow("synchronous / appout")  << -1 << "Output from app itself."
                                           << false << true;

    QTest::addRow("no delay / waiting") << 0
                                        << "QML Debugger: Waiting for connection on port 2423..."
                                        << true << false;
    QTest::addRow("no delay / failed")  << 0 << "QML Debugger: Unable to listen to port 242."
                                        << false << false;
    QTest::addRow("no delay / unknown") << 0 << "QML Debugger: You don't know this string."
                                        << false << true;
    QTest::addRow("no delay / appout")  << 0 << "Output from app itself."
                                        << false << true;

    QTest::addRow("delay / waiting") << 1000
                                     << "QML Debugger: Waiting for connection on port 2423..."
                                     << true << false;
    QTest::addRow("delay / failed")  << 1000 << "QML Debugger: Unable to listen to port 242."
                                     << false << false;
    QTest::addRow("delay / unknown") << 1000 << "QML Debugger: You don't know this string."
                                     << false << true;
    QTest::addRow("delay / appout")  << 1000 << "Output from app itself."
                                     << false << true;
}

void tst_QQmlDebugProcess::sessionStart()
{
    QFETCH(int, delay);
    QFETCH(QString, arg);
    QFETCH(bool, sessionExpected);
    QFETCH(bool, outputExpected);

    QScopedPointer<QQmlDebugProcess> process(
                new QQmlDebugProcess(QCoreApplication::applicationDirPath()
                                     + QLatin1String("/qqmldebugprocessprocess"), this));
    QVERIFY(process);

    bool outputReceived = false;
    connect(process.data(), &QQmlDebugProcess::readyReadStandardOutput, this, [&]() {
        QVERIFY(outputExpected);
        QVERIFY(!outputReceived);
        QCOMPARE(process->output().trimmed(), arg);
        outputReceived = true;
        QTimer::singleShot(qMax(delay, 0), process.data(), &QQmlDebugProcess::stop);
    });

    if (!outputExpected && !sessionExpected)
        QTest::ignoreMessage(QtWarningMsg, "App was unable to bind to port!");
    process->start(QStringList({arg}));

    bool done = false;
    auto wait = [&](){
        QCOMPARE(process->waitForSessionStart(), sessionExpected);
        QCOMPARE(outputReceived, outputExpected);
        process->stop();
        done = true;
    };

    if (delay < 0)
        wait();
    else
        QTimer::singleShot(delay, process.data(), wait);

    QTRY_VERIFY(done);
    QCOMPARE(process->state(), QProcess::NotRunning);
}

QTEST_MAIN(tst_QQmlDebugProcess)

#include "tst_qqmldebugprocess.moc"
