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
#include <QtCore/qobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdebug.h>
#include <QtGui/qcolor.h>

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
    void mathMax();
    void mathMin();

private:
    QQmlEngine engine;
};

void tst_v4::initTestCase()
{
    QQmlDataTest::initTestCase();
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

    QQmlComponent component(&engine, testFileUrl(file));

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
    //QCOMPARE(o->property("test12").toBool(), true);   //QTBUG-24706

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
    //QCOMPARE(o->property("test4").toBool(), true);    //QTBUG-24706
    QCOMPARE(o->property("test5").toBool(), true);
    QCOMPARE(o->property("test6").toReal(), qreal(83));
    //QCOMPARE(o->property("test7").toBool(), true);    //QTBUG-24706
    //QCOMPARE(o->property("test8").toBool(), true);    //QTBUG-24706
    QCOMPARE(o->property("test9").toInt(), 0);
    //QCOMPARE(o->property("test10").toBool(), true);   //QTBUG-24706

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
    //QCOMPARE(o->property("test9").toBool(), true);    //QTBUG-24706
    QCOMPARE(o->property("test10").toReal(), qreal(0));
    QCOMPARE(o->property("test11").toReal(), qreal(4.4));
    QCOMPARE(o->property("test12").toReal(), qreal(7));

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
    //QCOMPARE(o->property("test9").toBool(), true);    //QTBUG-24706
    QCOMPARE(o->property("test10").toReal(), qreal(-3.7));
    QCOMPARE(o->property("test11").toReal(), qreal(0));
    QCOMPARE(o->property("test12").toReal(), qreal(-3.7));
    delete o;
}

QTEST_MAIN(tst_v4)

#include "tst_v4.moc"
