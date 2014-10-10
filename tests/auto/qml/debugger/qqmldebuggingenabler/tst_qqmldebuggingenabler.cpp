/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QLibraryInfo>

#include "debugutil_p.h"
#include "qqmldebugclient.h"
#include "../../../shared/util.h"

class tst_QQmlDebuggingEnabler : public QQmlDataTest
{
    Q_OBJECT

    bool init(bool blockMode, bool qmlscene, int portFrom, int portTo);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();
    void qmlscene();
    void qmlsceneBlock();
    void qmlsceneInvalidPorts();
    void qmlsceneBlockInvalidPorts();
    void qmlsceneMixedPorts();
    void qmlsceneBlockMixedPorts();

    void custom();
    void customBlock();
    void customInvalidPorts();
    void customBlockInvalidPorts();
    void customMixedPorts();
    void customBlockMixedPorts();

private:
    QQmlDebugProcess *process;
    QQmlDebugConnection *connection;
    QTime t;
};

void tst_QQmlDebuggingEnabler::initTestCase()
{
    QQmlDataTest::initTestCase();
    t.start();
    process = 0;
    connection = 0;
}

void tst_QQmlDebuggingEnabler::cleanupTestCase()
{
    if (process) {
        process->stop();
        delete process;
    }

    if (connection)
        delete connection;
}

bool tst_QQmlDebuggingEnabler::init(bool blockMode, bool qmlscene, int portFrom, int portTo)
{
    connection = new QQmlDebugConnection();

    if (qmlscene) {
        process = new QQmlDebugProcess(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmlscene", this);
        process->setMaximumBindErrors(1);
    } else {
        process = new QQmlDebugProcess(QCoreApplication::applicationFilePath(), this);
        process->setMaximumBindErrors(portTo - portFrom);
    }

    if (qmlscene) {
        process->start(QStringList() << QLatin1String("-qmljsdebugger=port:") +
                       QString::number(portFrom) + QLatin1String(",") + QString::number(portTo) +
                       QLatin1String(blockMode ? ",block": "") <<
                       testFile(QLatin1String("test.qml")));
    } else {
        QStringList args;
        args << QLatin1String("-server");
        if (blockMode)
            args << QLatin1String("-block");
        args << QString::number(portFrom) << QString::number(portTo);
        process->start(args);
    }

    if (!process->waitForSessionStart()) {
        return false;
    }

    const int port = process->debugPort();
    connection->connectToHost("127.0.0.1", port);
    if (!connection->waitForConnected()) {
        qDebug() << "could not connect to host!";
        return false;
    }
    return true;
}

void tst_QQmlDebuggingEnabler::cleanup()
{
    if (QTest::currentTestFailed()) {
        qDebug() << "Process State:" << process->state();
        qDebug() << "Application Output:" << process->output();
    }

    if (process) {
        process->stop();
        delete process;
    }


    if (connection)
        delete connection;

    process = 0;
    connection = 0;
}

void tst_QQmlDebuggingEnabler::qmlscene()
{
    QVERIFY(init(false, true, 5555, 5565));
}

void tst_QQmlDebuggingEnabler::qmlsceneBlock()
{
    QVERIFY(init(true, true, 5555, 5565));
}

void tst_QQmlDebuggingEnabler::qmlsceneInvalidPorts()
{
    QVERIFY(!init(false, true, 10, 20));
    QVERIFY(process->output().contains(
                QLatin1String("QML Debugger: Unable to listen to ports 10 - 20.")));
}

void tst_QQmlDebuggingEnabler::qmlsceneBlockInvalidPorts()
{
    QVERIFY(!init(true, true, 10, 20));
    QVERIFY(process->output().contains(
                QLatin1String("QML Debugger: Unable to listen to ports 10 - 20.")));
}

void tst_QQmlDebuggingEnabler::qmlsceneMixedPorts()
{
    QVERIFY(init(false, true, 1020, 1030));
}

void tst_QQmlDebuggingEnabler::qmlsceneBlockMixedPorts()
{
    QVERIFY(init(true, true, 1020, 1030));
}

void tst_QQmlDebuggingEnabler::custom()
{
    QVERIFY(init(false, false, 5555, 5565));
}

void tst_QQmlDebuggingEnabler::customBlock()
{
    QVERIFY(init(true, false, 5555, 5565));
}

void tst_QQmlDebuggingEnabler::customInvalidPorts()
{
    QVERIFY(!init(false, false, 10, 20));
    for (int i = 10; i < 20; ++i) {
        QVERIFY(process->output().contains(
                QString(QLatin1String("QML Debugger: Unable to listen to port %1.")).arg(i)));
    }
}

void tst_QQmlDebuggingEnabler::customBlockInvalidPorts()
{
    QVERIFY(!init(true, false, 10, 20));
    for (int i = 10; i < 20; ++i) {
        QVERIFY(process->output().contains(
                QString(QLatin1String("QML Debugger: Unable to listen to port %1.")).arg(i)));
    }
}

void tst_QQmlDebuggingEnabler::customMixedPorts()
{
    QVERIFY(init(false, false, 1020, 1030));
    for (int i = 1020; i < 1024; ++i) {
        QVERIFY(process->output().contains(
                QString(QLatin1String("QML Debugger: Unable to listen to port %1.")).arg(i)));
    }
}

void tst_QQmlDebuggingEnabler::customBlockMixedPorts()
{
    QVERIFY(init(true, false, 1020, 1030));
    for (int i = 1020; i < 1024; ++i) {
        QVERIFY(process->output().contains(
                QString(QLatin1String("QML Debugger: Unable to listen to port %1.")).arg(i)));
    }
}

namespace QQmlDebuggingEnablerTest {
    QTEST_MAIN(tst_QQmlDebuggingEnabler)
}

int main(int argc, char *argv[])
{
    if (argc > 1 && QLatin1String(argv[1]) == QLatin1String("-server")) {
        int one = 1;
        QCoreApplication app(one, argv);
        bool block = argc > 2 && QLatin1String(argv[2]) == QLatin1String("-block");
        int portFrom = QString(QLatin1String(argv[argc - 2])).toInt();
        int portTo = QString(QLatin1String(argv[argc - 1])).toInt();
        while (portFrom <= portTo)
            QQmlDebuggingEnabler::startTcpDebugServer(portFrom++, block);
        QQmlEngine engine;
        Q_UNUSED(engine);
        app.exec();
    } else {
        QQmlDebuggingEnablerTest::main(argc, argv);
    }
}

#include "tst_qqmldebuggingenabler.moc"

