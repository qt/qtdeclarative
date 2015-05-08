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

    void custom();
    void customBlock();

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
        process = new QQmlDebugProcess(QCoreApplication::applicationDirPath() + QLatin1String("/qqmldebuggingenablerserver"), this);
        process->setMaximumBindErrors(portTo - portFrom);
    }

    if (qmlscene) {
        process->start(QStringList() << QLatin1String("-qmljsdebugger=port:") +
                       QString::number(portFrom) + QLatin1String(",") + QString::number(portTo) +
                       QLatin1String(blockMode ? ",block": "") <<
                       testFile(QLatin1String("test.qml")));
    } else {
        QStringList args;
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

void tst_QQmlDebuggingEnabler::custom()
{
    QVERIFY(init(false, false, 5555, 5565));
}

void tst_QQmlDebuggingEnabler::customBlock()
{
    QVERIFY(init(true, false, 5555, 5565));
}

QTEST_MAIN(tst_QQmlDebuggingEnabler)

#include "tst_qqmldebuggingenabler.moc"

