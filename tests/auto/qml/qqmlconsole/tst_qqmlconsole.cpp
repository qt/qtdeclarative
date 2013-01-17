/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include "../../shared/util.h"

class tst_qqmlconsole : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlconsole() {}

private slots:
    void logging();
    void tracing();
    void profiling();
    void assert();
    void exception();

private:
    QQmlEngine engine;
};

void tst_qqmlconsole::logging()
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
    QTest::ignoreMessage(QtDebugMsg, "{\"a\":\"hello\",\"d\":1}");
    QTest::ignoreMessage(QtDebugMsg, "undefined");
    QTest::ignoreMessage(QtDebugMsg, "12");
    QTest::ignoreMessage(QtDebugMsg, "function () { return 5;}");
    QTest::ignoreMessage(QtDebugMsg, "true");
    // Printing QML object prints out the class/type of QML object with the memory address
//    QTest::ignoreMessage(QtDebugMsg, "QtObject_QML_0(0xABCD..)");
    QTest::ignoreMessage(QtDebugMsg, "[object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 pong! [object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 [ping,pong] [object Object] 2");

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qqmlconsole::tracing()
{
    QUrl testUrl = testFileUrl("tracing.qml");

    QString traceText =
            QString::fromLatin1("tracing (%1:%2:%3)\n").arg(testUrl.toString()).arg(50).arg(17) +
            QString::fromLatin1("onCompleted (%1:%2:%3)").arg(testUrl.toString()).arg(54).arg(9);

    QTest::ignoreMessage(QtDebugMsg, qPrintable(traceText));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qqmlconsole::profiling()
{
    QUrl testUrl = testFileUrl("profiling.qml");

    // profiling()
    QTest::ignoreMessage(QtDebugMsg, "Profiling started.");
    QTest::ignoreMessage(QtDebugMsg, "Profiling ended.");

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qqmlconsole::assert()
{
    QUrl testUrl = testFileUrl("assert.qml");

    // assert()
    QString assert1 = "This will fail\n" +
            QString::fromLatin1("onCompleted (%1:%2:%3)").arg(testUrl.toString()).arg(54).arg(17);

    QString assert2 = "This will fail too\n" +
            QString::fromLatin1("assertFail (%1:%2:%3)\n").arg(testUrl.toString()).arg(47).arg(17) +
            QString::fromLatin1("onCompleted (%1:%2:%3)").arg(testUrl.toString()).arg(59).arg(9);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qqmlconsole::exception()
{
    QUrl testUrl = testFileUrl("exception.qml");

    // exception()
    QString exception1 = "Exception 1\n" +
            QString::fromLatin1("onCompleted (%1:%2:%3)").arg(testUrl.toString()).arg(51).arg(21);

    QString exception2 = "Exception 2\n" +
            QString::fromLatin1("exceptionFail (%1:%2:%3)\n").arg(testUrl.toString()).arg(46).arg(17) +
            QString::fromLatin1("onCompleted (%1:%2:%3)").arg(testUrl.toString()).arg(56).arg(9);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

QTEST_MAIN(tst_qqmlconsole)

#include "tst_qqmlconsole.moc"
