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
#include <QDebug>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include "../../shared/util.h"

class tst_qdeclarativeconsole : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeconsole() {}

private slots:
    void init();
    void consoleLogExtended();

private:
    QDeclarativeEngine engine;
};

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(TESTDATA(filename));
}

void tst_qdeclarativeconsole::init()
{
    qputenv("QML_CONSOLE_EXTENDED", QByteArray("1"));
}

void tst_qdeclarativeconsole::consoleLogExtended()
{
    int startLineNumber = 15;
    QUrl testFileUrl = TEST_FILE("consoleLog.qml");
    QString testString = QString(QLatin1String("completed ok (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testString.arg(startLineNumber++)));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testString.arg(startLineNumber++)));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testString.arg(startLineNumber++)));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(testString.arg(startLineNumber++)));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(testString.arg(startLineNumber++)));

    QString testArray = QString(QLatin1String("[1,2] (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testArray.arg(startLineNumber++)));
    QString testObject = QString(QLatin1String("Object (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testObject.arg(startLineNumber++)));
    QString testUndefined = QString(QLatin1String("undefined (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testUndefined.arg(startLineNumber++)));
    QString testNumber = QString(QLatin1String("12 (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testNumber.arg(startLineNumber++)));
    QString testFunction = QString(QLatin1String("function () { return 5;} (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testFunction.arg(startLineNumber++)));
    QString testBoolean = QString(QLatin1String("true (%1:%2)")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testBoolean.arg(startLineNumber++)));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testObject.arg(startLineNumber++)));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testObject.arg(startLineNumber++)));
    QString testMix = QString::fromLatin1("1 pong! Object (%1:%2)").arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testMix.arg(startLineNumber++)));
    testMix = QString::fromLatin1("1 [ping,pong] Object 2 (%1:%2)").arg(testFileUrl.toString());
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testMix.arg(startLineNumber++)));

    QString testException = QString(QLatin1String("%1:%2: ReferenceError: Can't find variable: exception")).arg(testFileUrl.toString());
    QTest::ignoreMessage(QtWarningMsg, qPrintable(testException.arg(startLineNumber++)));

    QDeclarativeComponent component(&engine, testFileUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

QTEST_MAIN(tst_qdeclarativeconsole)

#include "tst_qdeclarativeconsole.moc"
