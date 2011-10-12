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
#include <QtCore/qobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtCore/qdebug.h>

#include <private/qv4compiler_p.h>

#include "../shared/util.h"
#include "testtypes.h"

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(TESTDATA(filename));
}

inline QUrl TEST_FILE(const char *filename)
{
    return TEST_FILE(QLatin1String(filename));
}

class tst_v4 : public QObject
{
    Q_OBJECT
public:
    tst_v4() {}

private slots:
    void initTestCase();

    void unnecessaryReeval();
    void logicalOr();
    void conditionalExpr();
    void qtscript();
    void qtscript_data();
    void nestedObjectAccess();
    void subscriptionsInConditionalExpressions();

private:
    QDeclarativeEngine engine;
};

void tst_v4::initTestCase()
{
    registerTypes();
}

static int v4ErrorsMsgCount = 0;
static void v4ErrorsMsgHandler(QtMsgType, const char *message)
{
    QByteArray m(message);
    if (m.contains("QV4"))
        v4ErrorsMsgCount++;
}

void tst_v4::qtscript()
{
    QFETCH(QString, file);
    QV4Compiler::enableBindingsTest(true);

    QDeclarativeComponent component(&engine, TEST_FILE(file));

    v4ErrorsMsgCount = 0;
    QtMsgHandler old = qInstallMsgHandler(v4ErrorsMsgHandler);

    QObject *o = component.create();
    delete o;

    qInstallMsgHandler(old);

    QCOMPARE(v4ErrorsMsgCount, 0);

    QV4Compiler::enableBindingsTest(false);
}

void tst_v4::qtscript_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("qreal -> int rounding") << "qrealToIntRounding.qml";
    QTest::newRow("exception on fetch") << "fetchException.qml";
    QTest::newRow("logical or") << "logicalOr.qml";
    QTest::newRow("conditional expressions") << "conditionalExpr.qml";
    QTest::newRow("double bool jump") << "doubleBoolJump.qml";
    QTest::newRow("unary minus") << "unaryMinus.qml";
    QTest::newRow("null qobject") << "nullQObject.qml";
}

void tst_v4::unnecessaryReeval()
{
    QDeclarativeComponent component(&engine, TEST_FILE("unnecessaryReeval.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    ResultObject *ro = qobject_cast<ResultObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->resultCounter(),  1);
    QCOMPARE(ro->result(), 19);
    ro->resetResultCounter();

    ro->setProperty("b", 6);

    QCOMPARE(ro->resultCounter(),  1);
    QCOMPARE(ro->result(), 6);
    ro->resetResultCounter();

    ro->setProperty("a", 14);

    QCOMPARE(ro->resultCounter(),  1);
    QCOMPARE(ro->result(), 7);
    ro->resetResultCounter();

    ro->setProperty("b", 14);
    QCOMPARE(ro->resultCounter(),  0);
    QCOMPARE(ro->result(), 7);

    delete o;
}

void tst_v4::logicalOr()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("logicalOr.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 0);
        delete o;
    }

    {
        QDeclarativeComponent component(&engine, TEST_FILE("logicalOr.2.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }
}

void tst_v4::conditionalExpr()
{
    {
        QDeclarativeComponent component(&engine, TEST_FILE("conditionalExpr.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 0);
        delete o;
    }
}

// This would previously use the metaObject of the root element to result the nested access.
// That is, the index for accessing "result" would have been RootObject::result, instead of
// NestedObject::result.
void tst_v4::nestedObjectAccess()
{
    QDeclarativeComponent component(&engine, TEST_FILE("nestedObjectAccess.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    ResultObject *ro = qobject_cast<ResultObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->result(), 37);

    delete o;
}

void tst_v4::subscriptionsInConditionalExpressions()
{
    QDeclarativeComponent component(&engine, TEST_FILE("subscriptionsInConditionalExpressions.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QObject *ro = qobject_cast<QObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->property("result").toReal(), qreal(2));

    delete o;
}

QTEST_MAIN(tst_v4)

#include "tst_v4.moc"
