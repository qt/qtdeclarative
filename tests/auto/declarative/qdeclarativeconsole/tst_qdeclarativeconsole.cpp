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
#include <QDebug>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include "../../shared/util.h"

class tst_qdeclarativeconsole : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativeconsole() {}

private slots:
    void logging();
    void tracing();
    void profiling();
    void assert();
    void exception();

private:
    QDeclarativeEngine engine;
};

void tst_qdeclarativeconsole::logging()
{
    QUrl testUrl = testFileUrl("logging.qml");

    QTest::ignoreMessage(QtDebugMsg, "console.debug");
    QTest::ignoreMessage(QtDebugMsg, "console.log");
    QTest::ignoreMessage(QtDebugMsg, "console.info");
    QTest::ignoreMessage(QtWarningMsg, "console.warn");
    QTest::ignoreMessage(QtCriticalMsg, "console.error");

    QTest::ignoreMessage(QtDebugMsg, "console.count: 1");
    QTest::ignoreMessage(QtDebugMsg, ": 1");
    QTest::ignoreMessage(QtDebugMsg, "console.count: 2");
    QTest::ignoreMessage(QtDebugMsg, ": 2");

    QTest::ignoreMessage(QtDebugMsg, "[1,2]");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "undefined");
    QTest::ignoreMessage(QtDebugMsg, "12");
    QTest::ignoreMessage(QtDebugMsg, "function () { return 5;}");
    QTest::ignoreMessage(QtDebugMsg, "true");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "1 pong! Object");
    QTest::ignoreMessage(QtDebugMsg, "1 [ping,pong] Object 2");

    QDeclarativeComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeconsole::tracing()
{
    QUrl testUrl = testFileUrl("tracing.qml");

    QString trace1 = QString::fromLatin1("tracing (%1:%2:%3)\n").arg(testUrl.toString()).arg(50).arg(17);
    QString trace2 = QString::fromLatin1("onCompleted (%1:%2:%3)\n").arg(testUrl.toString()).arg(54).arg(9);
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace1));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace2));

    QDeclarativeComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeconsole::profiling()
{
    QUrl testUrl = testFileUrl("profiling.qml");

    // profiling()
    QTest::ignoreMessage(QtDebugMsg, "Profiling started.");
    QTest::ignoreMessage(QtDebugMsg, "Profiling ended.");

    QDeclarativeComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeconsole::assert()
{
    QUrl testUrl = testFileUrl("assert.qml");

    // assert()
    QTest::ignoreMessage(QtCriticalMsg, "This will fail");
    QTest::ignoreMessage(QtCriticalMsg, "This will fail too");
    QString trace1 = QString::fromLatin1("onCompleted (%1:%2:%3)\n").arg(testUrl.toString()).arg(54).arg(17);
    QString trace2 = QString::fromLatin1("onCompleted (%1:%2:%3)\n").arg(testUrl.toString()).arg(59).arg(9);
    QString trace3 = QString::fromLatin1("assertFail (%1:%2:%3)\n").arg(testUrl.toString()).arg(47).arg(17);
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace1));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace2));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace3));

    QDeclarativeComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeconsole::exception()
{
    QUrl testUrl = testFileUrl("exception.qml");

    // exception()
    QTest::ignoreMessage(QtCriticalMsg, "Exception 1");
    QTest::ignoreMessage(QtCriticalMsg, "Exception 2");
    QString trace1 = QString::fromLatin1("onCompleted (%1:%2:%3)\n").arg(testUrl.toString()).arg(51).arg(21);
    QString trace2 = QString::fromLatin1("onCompleted (%1:%2:%3)\n").arg(testUrl.toString()).arg(56).arg(9);
    QString trace3 = QString::fromLatin1("exceptionFail (%1:%2:%3)\n").arg(testUrl.toString()).arg(46).arg(17);
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace1));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace2));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(trace3));

    QDeclarativeComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

QTEST_MAIN(tst_qdeclarativeconsole)

#include "tst_qdeclarativeconsole.moc"
