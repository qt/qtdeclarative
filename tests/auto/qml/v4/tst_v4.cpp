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
#include <QtCore/qobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdebug.h>
#include <QtGui/qcolor.h>
#include <QtCore/qnumeric.h>

#include <private/qv4compiler_p.h>

#include "../../shared/util.h"
#include "testtypes.h"

class tst_v4 : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_v4() {}

private slots:
    void initTestCase();

    void unnecessaryReeval();
    void logicalOr();
    void nestedLogicalOr();
    void logicalAnd();
    void nestedLogicalAnd();
    void conditionalExpr();
    void qtscript();
    void qtscript_data();
    void nestedObjectAccess();
    void subscriptionsInConditionalExpressions();
    void qtbug_21883();
    void qtbug_22816();
    void stringComparison();
    void unaryMinus();
    void unaryPlus();
    void colorType();
    void mathAbs();
    void mathCeil();
    void mathFloor();
    void mathMax();
    void mathMin();
    void mathCos();
    void mathSin();
    void singletonType();
    void integerOperations();

    void conversions_data();
    void conversions();
    void subscriptions();

    void debuggingDumpInstructions(); // this test should be last.

private:
    QQmlEngine engine;
};

void tst_v4::initTestCase()
{
    QQmlDataTest::initTestCase();
    registerTypes();
}

void tst_v4::qtscript()
{
    QFETCH(QString, file);
    QV4Compiler::enableBindingsTest(true);

    QQmlComponent component(&engine, testFileUrl(file));

    QQmlTestMessageHandler messageHandler;

    QObject *o = component.create();
    delete o;

    QEXPECT_FAIL("jsvalueHandling", "QTBUG-26951 - QJSValue has a different representation of NULL to QV8Engine", Continue);
    const int v4ErrorCount = messageHandler.messages().filter(QLatin1String("QV4")).size();
    QVERIFY2(v4ErrorCount == 0, qPrintable(messageHandler.messageString()));

    QV4Compiler::enableBindingsTest(false);
}

void tst_v4::qtscript_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("equals") << "equals.qml";
    QTest::newRow("strict equals") << "strictEquals.qml";
    QTest::newRow("qreal -> int rounding") << "qrealToIntRounding.qml";
    QTest::newRow("exception on fetch") << "fetchException.qml";
    QTest::newRow("logical or") << "logicalOr.qml";
    QTest::newRow("conditional expressions") << "conditionalExpr.qml";
    QTest::newRow("double bool jump") << "doubleBoolJump.qml";
    QTest::newRow("unary minus") << "unaryMinus.qml";
    QTest::newRow("null qobject") << "nullQObject.qml";
    QTest::newRow("qobject -> bool") << "objectToBool.qml";
    QTest::newRow("conversion from bool") << "conversions.1.qml";
    QTest::newRow("conversion from int") << "conversions.2.qml";
    QTest::newRow("conversion from float") << "conversions.3.qml";
    QTest::newRow("conversion from double") << "conversions.4.qml";
    QTest::newRow("conversion from real") << "conversions.5.qml";
    QTest::newRow("conversion from string") << "conversions.6.qml";
    QTest::newRow("conversion from url") << "conversions.7.qml";
    QTest::newRow("conversion from vec3") << "conversions.8.qml";
    QTest::newRow("variantHandling") << "variantHandling.qml";
    QTest::newRow("varHandling") << "varHandling.qml";
    QTest::newRow("jsvalueHandling") << "jsvalueHandling.qml";
    QTest::newRow("integerOperations") << "integerOperations.qml";
}

void tst_v4::unnecessaryReeval()
{
    QQmlComponent component(&engine, testFileUrl("unnecessaryReeval.qml"));

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
        QQmlComponent component(&engine, testFileUrl("logicalOr.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 0);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalOr.2.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }
}

void tst_v4::nestedLogicalOr()
{
    //we are primarily testing that v4 does not get caught in a loop (QTBUG-24038)
    QQmlComponent component(&engine, testFileUrl("nestedLogicalOr.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    ResultObject *ro = qobject_cast<ResultObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->result(), 1);
    delete o;
}

void tst_v4::logicalAnd()
{
    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 0);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.2.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.3.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }

    {
        // QTBUG-24660
        QQmlComponent component(&engine, testFileUrl("logicalAnd.4.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.5.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.6.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("logicalAnd.7.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 1);
        delete o;
    }
}

void tst_v4::nestedLogicalAnd()
{
    QQmlComponent component(&engine, testFileUrl("nestedLogicalAnd.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    ResultObject *ro = qobject_cast<ResultObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->result(), 1);
    delete o;
}

void tst_v4::conditionalExpr()
{
    {
        QQmlComponent component(&engine, testFileUrl("conditionalExpr.qml"));

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
    {
        QQmlComponent component(&engine, testFileUrl("nestedObjectAccess.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 37);

        delete o;
    }

    {
        QQmlComponent component(&engine, testFileUrl("nestedObjectAccess2.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        ResultObject *ro = qobject_cast<ResultObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->result(), 37);

        delete o;
    }
}

void tst_v4::subscriptionsInConditionalExpressions()
{
    QQmlComponent component(&engine, testFileUrl("subscriptionsInConditionalExpressions.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QObject *ro = qobject_cast<QObject *>(o);
    QVERIFY(ro != 0);

    QCOMPARE(ro->property("result").toReal(), qreal(2));

    delete o;
}

// Crash test
void tst_v4::qtbug_21883()
{
    QQmlComponent component(&engine, testFileUrl("qtbug_21883.qml"));

    QString warning = component.url().toString() + ":4: Unable to assign null to ResultObject*";
    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1().constData());

    QObject *o = component.create();
    QVERIFY(o != 0);
    delete o;
}

void tst_v4::qtbug_22816()
{
    QQmlComponent component(&engine, testFileUrl("qtbug_22816.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").toBool(), false);
    QCOMPARE(o->property("test2").toBool(), false);
    delete o;
}

void tst_v4::stringComparison()
{
    QQmlComponent component(&engine, testFileUrl("stringComparison.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toBool(), true);
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QCOMPARE(o->property("test11").toBool(), true);
    QCOMPARE(o->property("test12").toBool(), true);
    QCOMPARE(o->property("test13").toBool(), true);
    QCOMPARE(o->property("test14").toBool(), true);
    QCOMPARE(o->property("test15").toBool(), true);
    QCOMPARE(o->property("test16").toBool(), true);
    QCOMPARE(o->property("test17").toBool(), true);
    QCOMPARE(o->property("test18").toBool(), true);
    QCOMPARE(o->property("test19").toBool(), true);
    QCOMPARE(o->property("test20").toBool(), true);
    QCOMPARE(o->property("test21").toBool(), true);
    QCOMPARE(o->property("test22").toBool(), true);
    delete o;
}

void tst_v4::unaryMinus()
{
    QQmlComponent component(&engine, testFileUrl("unaryMinus.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(-18));
    QCOMPARE(o->property("test2").toInt(), -18);
    QCOMPARE(o->property("test3").toReal(), qreal(3.7));
    QCOMPARE(o->property("test4").toInt(), 4);
    QCOMPARE(o->property("test5").toReal(), qreal(3.3));
    QCOMPARE(o->property("test6").toInt(), 3);
    QCOMPARE(o->property("test7").toReal(), qreal(7));
    QCOMPARE(o->property("test8").toInt(), 7);
    QCOMPARE(o->property("test9").toReal(), qreal(-4.4));
    QCOMPARE(o->property("test10").toInt(), -4);

    delete o;
}

void tst_v4::unaryPlus()
{
    QQmlComponent component(&engine, testFileUrl("unaryPlus.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(18));
    QCOMPARE(o->property("test2").toInt(), 18);
    QCOMPARE(o->property("test3").toReal(), qreal(-3.7));
    QCOMPARE(o->property("test4").toInt(), -4);
    QCOMPARE(o->property("test5").toReal(), qreal(-3.3));
    QCOMPARE(o->property("test6").toInt(), -3);
    QCOMPARE(o->property("test7").toReal(), qreal(-7));
    QCOMPARE(o->property("test8").toInt(), -7);
    QCOMPARE(o->property("test9").toReal(), qreal(4.4));
    QCOMPARE(o->property("test10").toInt(), 4);

    delete o;
}

void tst_v4::colorType()
{
    QQmlComponent component(&engine, testFileUrl("colorType.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("test1").value<QColor>(), QColor("red"));
    QCOMPARE(o->property("test2").value<QColor>(), QColor("red"));
    QCOMPARE(o->property("test3").value<QColor>(), QColor("red"));
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toBool(), true);
    QCOMPARE(o->property("test7").toBool(), true);
    delete o;
}

void tst_v4::mathAbs()
{
    QQmlComponent component(&engine, testFileUrl("mathAbs.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(3.7));
    QCOMPARE(o->property("test2").toReal(), qreal(4.5));
    QCOMPARE(o->property("test3").toInt(), 18);
    QCOMPARE(o->property("test4").toInt(), 72);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toBool(), true);
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toInt(), 82);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QCOMPARE(o->property("test11").toInt(), 0);
    QCOMPARE(o->property("test12").toBool(), true);

    delete o;
}

void tst_v4::mathCeil()
{
    QQmlComponent component(&engine, testFileUrl("mathCeil.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(-3));
    QCOMPARE(o->property("test2").toReal(), qreal(5));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toReal(), qreal(83));
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toInt(), 0);
    QCOMPARE(o->property("test10").toBool(), true);
    QCOMPARE(o->property("test11").toBool(), true);

    delete o;
}

void tst_v4::mathFloor()
{
    QQmlComponent component(&engine, testFileUrl("mathFloor.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(-4));
    QCOMPARE(o->property("test2").toReal(), qreal(4));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toReal(), qreal(82));
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toInt(), 0);
    QCOMPARE(o->property("test10").toBool(), true);

    delete o;
}

void tst_v4::mathMax()
{
    QQmlComponent component(&engine, testFileUrl("mathMax.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(4.4));
    QCOMPARE(o->property("test2").toReal(), qreal(7));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toReal(), qreal(82.6));
    QCOMPARE(o->property("test7").toReal(), qreal(4.4));
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QCOMPARE(o->property("test11").toReal(), qreal(0));
    QCOMPARE(o->property("test12").toReal(), qreal(4.4));
    QCOMPARE(o->property("test13").toReal(), qreal(7));

    delete o;
}

void tst_v4::mathMin()
{
    QQmlComponent component(&engine, testFileUrl("mathMin.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toReal(), qreal(-3.7));
    QCOMPARE(o->property("test2").toReal(), qreal(4.4));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toReal(), qreal(82.6));
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toReal(), qreal(4.4));
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QCOMPARE(o->property("test11").toReal(), qreal(-3.7));
    QCOMPARE(o->property("test12").toReal(), qreal(0));
    QCOMPARE(o->property("test13").toReal(), qreal(-3.7));
    delete o;
}

static bool fuzzyCompare(qreal a, qreal b)
{
    const qreal EPSILON = 0.0001;
    return (a + EPSILON > b) && (a - EPSILON < b);
}

void tst_v4::mathCos()
{
    QQmlComponent component(&engine, testFileUrl("mathCos.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QVERIFY(fuzzyCompare(o->property("test1").toReal(), qreal(-0.848100)));
    QVERIFY(fuzzyCompare(o->property("test2").toReal(), qreal(-0.307333)));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QVERIFY(fuzzyCompare(o->property("test6").toReal(), qreal(0.606941)));
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QVERIFY(fuzzyCompare(o->property("test11").toReal(), qreal(0.890792)));

    delete o;
}

void tst_v4::mathSin()
{
    QQmlComponent component(&engine, testFileUrl("mathSin.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QVERIFY(fuzzyCompare(o->property("test1").toReal(), qreal(0.529836)));
    QVERIFY(fuzzyCompare(o->property("test2").toReal(), qreal(-0.951602)));
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    QVERIFY(fuzzyCompare(o->property("test6").toReal(), qreal(0.794747)));
    QCOMPARE(o->property("test7").toBool(), true);
    QCOMPARE(o->property("test8").toBool(), true);
    QCOMPARE(o->property("test9").toBool(), true);
    QCOMPARE(o->property("test10").toBool(), true);
    QVERIFY(fuzzyCompare(o->property("test11").toReal(), qreal(0.454411)));

    delete o;
}

void tst_v4::integerOperations()
{
    QQmlComponent component(&engine, testFileUrl("integerOperations.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("testa1").toInt(), 333);
    QCOMPARE(o->property("testa2").toInt(), -666);

    QCOMPARE(o->property("testb1").toInt(), 0);
    QCOMPARE(o->property("testb2").toInt(), 2);
    QCOMPARE(o->property("testb3").toInt(), 0);
    QCOMPARE(o->property("testb4").toInt(), 2);
    QCOMPARE(o->property("testb5").toInt(), 0);
    QCOMPARE(o->property("testb6").toInt(), 2);
    QCOMPARE(o->property("testb7").toInt(), 0);
    QCOMPARE(o->property("testb8").toInt(), 2);

    QCOMPARE(o->property("testc1").toInt(), 335);
    QCOMPARE(o->property("testc2").toInt(), -666);
    QCOMPARE(o->property("testc3").toInt(), 335);
    QCOMPARE(o->property("testc4").toInt(), -666);
    QCOMPARE(o->property("testc5").toInt(), 335);
    QCOMPARE(o->property("testc6").toInt(), -666);
    QCOMPARE(o->property("testc7").toInt(), 335);
    QCOMPARE(o->property("testc8").toInt(), -666);

    QCOMPARE(o->property("testd1").toInt(), 330);
    QCOMPARE(o->property("testd2").toInt(), 330);
    QCOMPARE(o->property("testd3").toInt(), 330);
    QCOMPARE(o->property("testd4").toInt(), 330);

    QCOMPARE(o->property("teste1").toInt(), 28);
    QCOMPARE(o->property("teste2").toInt(), -28);
    QCOMPARE(o->property("teste3").toInt(), 256);
    QCOMPARE(o->property("teste4").toInt(), 28);
    QCOMPARE(o->property("teste5").toInt(), -28);
    QCOMPARE(o->property("teste6").toInt(), 256);

    QCOMPARE(o->property("testf1").toInt(), 1);
    QCOMPARE(o->property("testf2").toInt(), -2);
    QCOMPARE(o->property("testf3").toInt(), 0);
    QCOMPARE(o->property("testf4").toInt(), 1);
    QCOMPARE(o->property("testf5").toInt(), -2);
    QCOMPARE(o->property("testf6").toInt(), 0);

    QCOMPARE(o->property("testg1").toInt(), 1);
    QCOMPARE(o->property("testg2").toInt(), 0x3FFFFFFE);
    QCOMPARE(o->property("testg3").toInt(), 0);
    QCOMPARE(o->property("testg4").toInt(), 1);
    QCOMPARE(o->property("testg5").toInt(), 0x3FFFFFFE);
    QCOMPARE(o->property("testg6").toInt(), 0);

    delete o;
}

class V4SingletonType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int ip READ ip WRITE setIp NOTIFY ipChanged FINAL)
public:
    V4SingletonType() : m_ip(12) {}
    ~V4SingletonType() {}

    Q_INVOKABLE int random() { static int prng = 3; prng++; m_ip++; emit ipChanged(); return prng; }

    int ip() const { return m_ip; }
    void setIp(int v) { m_ip = v; emit ipChanged(); }

signals:
    void ipChanged();

private:
    int m_ip;
};

static QObject *v4_module_api_factory(QQmlEngine*, QJSEngine*)
{
    return new V4SingletonType;
}

void tst_v4::singletonType()
{
    // register singleton type, providing typeinfo via template
    qmlRegisterSingletonType<V4SingletonType>("Qt.test", 1, 0, "V4", v4_module_api_factory);
    QQmlComponent component(&engine, testFileUrl("singletonType.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("testProp").toInt(), 12);
    QCOMPARE(o->property("testProp2").toInt(), 2);
    QMetaObject::invokeMethod(o, "getRandom");
    QCOMPARE(o->property("testProp").toInt(), 13);
    QCOMPARE(o->property("testProp2").toInt(), 4);
    delete o;
}

void tst_v4::conversions_data()
{
    QTest::addColumn<QUrl>("file");
    QTest::addColumn<QStringList>("warnings");
    QTest::addColumn<bool>("boolProp");
    QTest::addColumn<int>("intProp");
    QTest::addColumn<float>("floatProp");
    QTest::addColumn<double>("doubleProp");
    QTest::addColumn<qreal>("qrealProp");
    QTest::addColumn<QString>("qstringProp");
    QTest::addColumn<QUrl>("qurlProp");
    QTest::addColumn<QVector3D>("vec3Prop");

    QTest::newRow("from bool") << testFileUrl("conversions.1.qml")
            << (QStringList() << (testFileUrl("conversions.1.qml").toString() + QLatin1String(":11:15: Unable to assign bool to QUrl")))
            << true
            << (int)true
            << (float)1.0
            << (double)1.0
            << (qreal)1.0
            << QString(QLatin1String("true"))
            << QUrl() // cannot assign bool to url.
            << QVector3D(1, 1, 1);

    QTest::newRow("from integer") << testFileUrl("conversions.2.qml")
            << (QStringList() << (testFileUrl("conversions.2.qml").toString() + QLatin1String(":11:15: Unable to assign int to QUrl")))
            << (bool)4
            << 4
            << (float)4.0
            << (double)4.0
            << (qreal)4.0
            << QString(QLatin1String("4"))
            << QUrl() // cannot assign int to url.
            << QVector3D(4, 4, 4);

    QTest::newRow("from float") << testFileUrl("conversions.3.qml")
            << (QStringList() << (testFileUrl("conversions.3.qml").toString() + QLatin1String(":11:15: Unable to assign number to QUrl")))
            << (bool)4.4
            << (int)4.4
            << (float)4.4
            << (double)((float)4.4)
            << (qreal)((float)4.4)
            << QString::number((double)((float)4.4), 'g', 16)
            << QUrl() // cannot assign number to url.
            << QVector3D(4.4, 4.4, 4.4);

    QTest::newRow("from double") << testFileUrl("conversions.4.qml")
            << (QStringList() << (testFileUrl("conversions.4.qml").toString() + QLatin1String(":11:15: Unable to assign number to QUrl")))
            << (bool)4.444444444
            << (int)4.444444444
            << (float)4.444444444
            << (double)4.444444444
            << (qreal)4.444444444
            << QString::number((double)4.444444444, 'g', 16)
            << QUrl() // cannot assign number to url.
            << QVector3D(4.444444444, 4.444444444, 4.444444444);

    QTest::newRow("from qreal") << testFileUrl("conversions.5.qml")
            << (QStringList() << (testFileUrl("conversions.5.qml").toString() + QLatin1String(":11:15: Unable to assign number to QUrl")))
            << (bool)4.44
            << (int)4.44
            << (float)4.44
            << (double)4.44
            << (qreal)4.44
            << QString(QLatin1String("4.44"))
            << QUrl() // cannot assign number to url.
            << QVector3D(4.44, 4.44, 4.44);

    QTest::newRow("from string") << testFileUrl("conversions.6.qml")
            << (QStringList())
            << true
            << 4
            << (float)4.0
            << (double)4.0
            << (qreal)4.0
            << QString(QLatin1String("4"))
            << QUrl(testFileUrl("").toString() + QString(QLatin1String("4")))
            << QVector3D(4, 4, 4);

    QTest::newRow("from url") << testFileUrl("conversions.7.qml")
            << (QStringList() << (testFileUrl("conversions.7.qml").toString() + QLatin1String(":6:14: Unable to assign QUrl to int"))
                              << (testFileUrl("conversions.7.qml").toString() + QLatin1String(":7:16: Unable to assign QUrl to number"))
                              << (testFileUrl("conversions.7.qml").toString() + QLatin1String(":8:17: Unable to assign QUrl to number"))
                              << (testFileUrl("conversions.7.qml").toString() + QLatin1String(":9:16: Unable to assign QUrl to number")))
            << true
            << 0
            << (float) 0
            << (double) 0
            << (qreal) 0
            << QString(testFileUrl("").toString() + QString(QLatin1String("4")))
            << QUrl(testFileUrl("").toString() + QString(QLatin1String("4")))
            << QVector3D(qQNaN(), qQNaN(), qQNaN());

    QTest::newRow("from vector") << testFileUrl("conversions.8.qml")
            << (QStringList() << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":11: Unable to assign QVector3D to QUrl"))
                              << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":10: Unable to assign QVector3D to QString"))
                              << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":9: Unable to assign QVector3D to double"))
                              << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":8: Unable to assign QVector3D to double"))
                              << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":7: Unable to assign QVector3D to float"))
                              << (testFileUrl("conversions.8.qml").toString() + QLatin1String(":6: Unable to assign QVector3D to int")))
            << true                // non-null therefore true
            << (int)0              // the other values should be the default-ctor values.
            << (float)0
            << (double)0
            << (qreal)0
            << QString()
            << QUrl()
            << QVector3D(4, 4, 4); // except this one.
}

#define COMPARE_NUMBER(type, prop, expected) \
    if (qIsNaN(expected)) \
        QVERIFY(qIsNaN(qvariant_cast<type>(prop))); \
    else \
        QCOMPARE((prop), QVariant::fromValue<type>(expected));

void tst_v4::conversions()
{
    QFETCH(QUrl, file);
    QFETCH(QStringList, warnings);
    QFETCH(bool, boolProp);
    QFETCH(int, intProp);
    QFETCH(float, floatProp);
    QFETCH(double, doubleProp);
    QFETCH(qreal, qrealProp);
    QFETCH(QString, qstringProp);
    QFETCH(QUrl, qurlProp);
    QFETCH(QVector3D, vec3Prop);

    foreach (const QString &w, warnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(w));

    QQmlComponent component(&engine, file);
    QObject *o = component.create();
    QVERIFY(o != 0);
    QCOMPARE(o->property("boolProp"), QVariant::fromValue<bool>(boolProp));
    QCOMPARE(o->property("intProp"), QVariant::fromValue<int>(intProp));
    COMPARE_NUMBER(float, o->property("floatProp"), floatProp);
    COMPARE_NUMBER(double, o->property("doubleProp"), doubleProp);
    COMPARE_NUMBER(qreal, o->property("qrealProp"), qrealProp);
    QCOMPARE(o->property("qstringProp"), QVariant::fromValue<QString>(qstringProp));
    QCOMPARE(o->property("qurlProp"), QVariant::fromValue<QUrl>(qurlProp));

    QVector3D vec3 = qvariant_cast<QVector3D>(o->property("vec3Prop"));
    COMPARE_NUMBER(qreal, QVariant::fromValue<qreal>(vec3.x()), vec3Prop.x());
    COMPARE_NUMBER(qreal, QVariant::fromValue<qreal>(vec3.y()), vec3Prop.y());
    COMPARE_NUMBER(qreal, QVariant::fromValue<qreal>(vec3.z()), vec3Prop.z());
    delete o;
}

void tst_v4::subscriptions()
{
    {
        QQmlComponent component(&engine, testFileUrl("subscriptions.1.qml"));

        QObject *o = component.create();
        QVERIFY(o != 0);

        QObject *ro = qobject_cast<QObject *>(o);
        QVERIFY(ro != 0);

        QCOMPARE(ro->property("targetHeight"), QVariant::fromValue<qreal>(201));

        delete o;
    }
}

static QByteArray getAddress(int address)
{
    return QByteArray::number(address);
}

static QByteArray getLeading(int address)
{
    QByteArray leading;
    if (address != -1) {
        leading = getAddress(address);
        leading.prepend(QByteArray(8 - leading.count(), ' '));
    }
    return leading;
}

#include <private/qv4instruction_p.h>
void tst_v4::debuggingDumpInstructions()
{
    QStringList expectedPreAddress;
    expectedPreAddress << "\t\tNoop";
    expectedPreAddress << "\t0:0:";
    expectedPreAddress << "\t\tSubscribeId\t\tId_Offset(0) -> Subscribe_Slot(0)";
    expectedPreAddress << "\t\tFetchAndSubscribe\tObject_Reg(0) Fast_Accessor(0x0) -> Output_Reg(0) Subscription_Slot(0)";
    expectedPreAddress << "\t\tLoadId\t\t\tId_Offset(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadScope\t\t-> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadRoot\t\t-> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadSingletonObject\t\t) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadAttached\t\tObject_Reg(0) Attached_Index(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tUnaryNot\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tUnaryMinusNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tUnaryMinusInt\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tUnaryPlusNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tUnaryPlusInt\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToInt\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToNumber\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToString\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertBoolToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToNumber\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToString\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertIntToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertJSValueToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToInt\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToString\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNumberToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToInt\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToNumber\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToUrl\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToColor\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertStringToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertUrlToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertUrlToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertUrlToString\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertUrlToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertUrlToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertColorToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertColorToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertColorToString\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertColorToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertColorToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertObjectToBool\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertObjectToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertObjectToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertObjectToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertVarToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNullToJSValue\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNullToObject\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNullToVariant\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tConvertNullToVar\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tResolveUrl\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathSinNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathCosNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathAbsNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathRoundNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathFloorNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathCeilNumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathPINumber\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadNull\t\tConstant(null) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadNumber\t\tConstant(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadInt\t\t\tConstant(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadBool\t\tConstant(false) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLoadString\t\tString_DataIndex(0) String_Length(0) -> Output_Register(0)";
    expectedPreAddress << "\t\tEnableV4Test\t\tString_DataIndex(0) String_Length(0)";
    expectedPreAddress << "\t\tTestV4Store\t\tInput_Reg(0) Reg_Type(0)";
    expectedPreAddress << "\t\tBitAndInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tBitOrInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tBitXorInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tAddNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tAddString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tSubNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMulNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tDivNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tModNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLShiftInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tRShiftInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tURShiftInt\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tGtNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLtNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tGeNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLeNumber\t\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tEqualNumber\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tNotEqualNumber\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictEqualNumber\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictNotEqualNumber\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tGtString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLtString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tGeString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tLeString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tEqualString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tNotEqualString\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictEqualString\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictNotEqualString\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tEqualObject\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tNotEqualObject\t\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictEqualObject\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStrictNotEqualObject\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathMaxNumber\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tMathMinNumber\tInput_Reg(0) Input_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tNewString\t\tRegister(0)";
    expectedPreAddress << "\t\tNewUrl\t\t\tRegister(0)";
    expectedPreAddress << "\t\tCleanupRegister\t\tRegister(0)";
    expectedPreAddress << "\t\tCopy\t\t\tInput_Reg(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tFetch\t\t\tObject_Reg(0) Property_Index(0) -> Output_Reg(0)";
    expectedPreAddress << "\t\tStore\t\t\tInput_Reg(0) -> Object_Reg(0) Property_Index(0)";
    expectedPreAddress << "\t\tJump\t\t\tAddress(UNIT_TEST_JUMP_ADDRESS) [if false == Input_Reg(0)]";         //(address + size() + i->jump.count)
    expectedPreAddress << "\t\tBranchTrue\t\tAddress(UNIT_TEST_BRANCH_ADDRESS) [if true == Input_Reg(0)]";    //(address + size() + i->branchop.offset)
    expectedPreAddress << "\t\tBranchFalse\t\tAddress(UNIT_TEST_BRANCH_ADDRESS) [if false == Input_Reg(0)]";  //(address + size() + i->branchop.offset)
    expectedPreAddress << "\t\tBranch\t\t\tAddress(UNIT_TEST_BRANCH_ADDRESS)";                                //(address + size() + i->branchop.offset)
    expectedPreAddress << "\t\tBlock\t\t\tMask(0)";
    expectedPreAddress << "\t\tThrow\t\t\tInputReg(0)";
    expectedPreAddress << "\t\tInitString\t\tString_DataIndex(0) -> String_Slot(0)";
    QStringList expected;

    QQmlTestMessageHandler messageHandler;

    QQmlJS::Bytecode bc;
#define DUMP_INSTR_IN_UNIT_TEST(I, FMT) { QQmlJS::V4InstrData<QQmlJS::V4Instr::I> i; memset(&i, 0, sizeof(i)); bc.append(i); }
    FOR_EACH_V4_INSTR(DUMP_INSTR_IN_UNIT_TEST);
#undef DUMP_INSTR_IN_UNIT_TEST // NOTE: we memset in order to ensure stable output.
    const char *start = bc.constData();
    const char *end = start + bc.size();
    const char *codeAddr = start;
    int whichExpected = 0;
#define DUMP_INSTR_SIZE_IN_UNIT_TEST(I, FMT) {                              \
            QString currExpected = whichExpected < expectedPreAddress.size() ? expectedPreAddress.at(whichExpected++) : QString();  \
            currExpected.prepend(getLeading(codeAddr - start));             \
            expected.append(currExpected);                                  \
            codeAddr += QQmlJS::V4Instr::size(static_cast<QQmlJS::V4Instr::Type>(QQmlJS::V4Instr::I)); \
        }
    FOR_EACH_V4_INSTR(DUMP_INSTR_SIZE_IN_UNIT_TEST);
#undef DUMP_INSTR_SIZE_IN_UNIT_TEST // so that we generate the correct address for each instruction comparison
    bc.dump(start, end);

    // ensure that the output was expected.
    const int messageCount = messageHandler.messages().count();
    QCOMPARE(messageCount, expected.count());
    for (int ii = 0; ii < messageCount; ++ii) {
        // Calculating the destination address of a null jump/branch instruction is tricky
        // so instead we simply don't compare that part of those instructions.
        QRegExp ignoreAddress("\\bAddress\\((\\w*)\\)");
        ignoreAddress.setMinimal(true);
        QString expectOut = expected.at(ii); expectOut.replace(ignoreAddress, "");
        QString actualOut = messageHandler.messages().at(ii); actualOut.replace(ignoreAddress, "");
        QCOMPARE(actualOut, expectOut);
    }
}


QTEST_MAIN(tst_v4)

#include "tst_v4.moc"
