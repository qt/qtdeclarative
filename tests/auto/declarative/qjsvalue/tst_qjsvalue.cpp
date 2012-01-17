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

#include "tst_qjsvalue.h"
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE
extern bool qt_script_isJITEnabled();
QT_END_NAMESPACE

tst_QJSValue::tst_QJSValue()
    : engine(0)
{
}

tst_QJSValue::~tst_QJSValue()
{
    if (engine)
        delete engine;
}

void tst_QJSValue::ctor_invalid()
{
    QJSEngine eng;
    {
        QJSValue v;
        QVERIFY(v.isUndefined());
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
}

void tst_QJSValue::ctor_undefinedWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.evaluate("undefined");
        QVERIFY(v.isUndefined());
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_nullWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.evaluate("null");
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNull(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_boolWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.toScriptValue(false);
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isBool(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toBool(), false);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_intWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.toScriptValue(int(1));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_int()
{
    {
        QJSValue v(int(0x43211234));
        QVERIFY(v.isNumber());
        QCOMPARE(v.toInt(), 0x43211234);
    }
    {
        QJSValue v(int(1));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
}

void tst_QJSValue::ctor_uintWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.toScriptValue(uint(1));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_uint()
{
    {
        QJSValue v(uint(0x43211234));
        QVERIFY(v.isNumber());
        QCOMPARE(v.toUInt(), uint(0x43211234));
    }
    {
        QJSValue v(uint(1));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
}

void tst_QJSValue::ctor_floatWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.toScriptValue(float(1.0));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_float()
{
    {
        QJSValue v(12345678910.5);
        QVERIFY(v.isNumber());
        QCOMPARE(v.toNumber(), 12345678910.5);
    }
    {
        QJSValue v(1.0);
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isNumber(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toNumber(), 1.0);
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
}

void tst_QJSValue::ctor_stringWithEngine()
{
    QJSEngine eng;
    {
        QJSValue v = eng.toScriptValue(QString::fromLatin1("ciao"));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isString(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toString(), QLatin1String("ciao"));
        QCOMPARE(v.engine(), &eng);
    }
}

void tst_QJSValue::ctor_string()
{
    {
        QJSValue v(QString("ciao"));
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isString(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toString(), QLatin1String("ciao"));
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
    {
        QJSValue v("ciao");
        QVERIFY(!v.isUndefined());
        QCOMPARE(v.isString(), true);
        QCOMPARE(v.isObject(), false);
        QCOMPARE(v.toString(), QLatin1String("ciao"));
        QCOMPARE(v.engine(), (QJSEngine *)0);
    }
}

void tst_QJSValue::ctor_copyAndAssignWithEngine()
{
    QJSEngine eng;
    // copy constructor, operator=
    {
        QJSValue v = eng.toScriptValue(1.0);
        QJSValue v2(v);
        QCOMPARE(v2.strictlyEquals(v), true);
        QCOMPARE(v2.engine(), &eng);

        QJSValue v3(v);
        QCOMPARE(v3.strictlyEquals(v), true);
        QCOMPARE(v3.strictlyEquals(v2), true);
        QCOMPARE(v3.engine(), &eng);

        QJSValue v4 = eng.toScriptValue(2.0);
        QCOMPARE(v4.strictlyEquals(v), false);
        v3 = v4;
        QCOMPARE(v3.strictlyEquals(v), false);
        QCOMPARE(v3.strictlyEquals(v4), true);

        v2 = QJSValue();
        QCOMPARE(v2.strictlyEquals(v), false);
        QCOMPARE(v.toNumber(), 1.0);

        QJSValue v5(v);
        QCOMPARE(v5.strictlyEquals(v), true);
        v = QJSValue();
        QCOMPARE(v5.strictlyEquals(v), false);
        QCOMPARE(v5.toNumber(), 1.0);
    }
}

void tst_QJSValue::ctor_undefined()
{
    QJSValue v(QJSValue::UndefinedValue);
    QVERIFY(v.isUndefined());
    QCOMPARE(v.isObject(), false);
    QCOMPARE(v.engine(), (QJSEngine *)0);
}

void tst_QJSValue::ctor_null()
{
    QJSValue v(QJSValue::NullValue);
    QVERIFY(!v.isUndefined());
    QCOMPARE(v.isNull(), true);
    QCOMPARE(v.isObject(), false);
    QCOMPARE(v.engine(), (QJSEngine *)0);
}

void tst_QJSValue::ctor_bool()
{
    QJSValue v(false);
    QVERIFY(!v.isUndefined());
    QCOMPARE(v.isBool(), true);
    QCOMPARE(v.isBool(), true);
    QCOMPARE(v.isObject(), false);
    QCOMPARE(v.toBool(), false);
    QCOMPARE(v.engine(), (QJSEngine *)0);
}

void tst_QJSValue::ctor_copyAndAssign()
{
    QJSValue v(1.0);
    QJSValue v2(v);
    QCOMPARE(v2.strictlyEquals(v), true);
    QCOMPARE(v2.engine(), (QJSEngine *)0);

    QJSValue v3(v);
    QCOMPARE(v3.strictlyEquals(v), true);
    QCOMPARE(v3.strictlyEquals(v2), true);
    QCOMPARE(v3.engine(), (QJSEngine *)0);

    QJSValue v4(2.0);
    QCOMPARE(v4.strictlyEquals(v), false);
    v3 = v4;
    QCOMPARE(v3.strictlyEquals(v), false);
    QCOMPARE(v3.strictlyEquals(v4), true);

    v2 = QJSValue();
    QCOMPARE(v2.strictlyEquals(v), false);
    QCOMPARE(v.toNumber(), 1.0);

    QJSValue v5(v);
    QCOMPARE(v5.strictlyEquals(v), true);
    v = QJSValue();
    QCOMPARE(v5.strictlyEquals(v), false);
    QCOMPARE(v5.toNumber(), 1.0);
}

#if 0 // FIXME: No c-style callbacks currently
static QJSValue myFunction(QScriptContext *, QScriptEngine *eng)
{
    return eng->undefinedValue();
}
#endif

void tst_QJSValue::toString()
{
    QJSEngine eng;

    QJSValue undefined = eng.undefinedValue();
    QCOMPARE(undefined.toString(), QString("undefined"));
    QCOMPARE(qjsvalue_cast<QString>(undefined), QString());

    QJSValue null = eng.nullValue();
    QCOMPARE(null.toString(), QString("null"));
    QCOMPARE(qjsvalue_cast<QString>(null), QString());

    {
        QJSValue falskt = eng.toScriptValue(false);
        QCOMPARE(falskt.toString(), QString("false"));
        QCOMPARE(qjsvalue_cast<QString>(falskt), QString("false"));

        QJSValue sant = eng.toScriptValue(true);
        QCOMPARE(sant.toString(), QString("true"));
        QCOMPARE(qjsvalue_cast<QString>(sant), QString("true"));
    }
    {
        QJSValue number = eng.toScriptValue(123);
        QCOMPARE(number.toString(), QString("123"));
        QCOMPARE(qjsvalue_cast<QString>(number), QString("123"));
    }
    {
        QJSValue number = eng.toScriptValue(6.37e-8);
        QCOMPARE(number.toString(), QString("6.37e-8"));
    }
    {
        QJSValue number = eng.toScriptValue(-6.37e-8);
        QCOMPARE(number.toString(), QString("-6.37e-8"));

        QJSValue str = eng.toScriptValue(QString("ciao"));
        QCOMPARE(str.toString(), QString("ciao"));
        QCOMPARE(qjsvalue_cast<QString>(str), QString("ciao"));
    }

    QJSValue object = eng.newObject();
    QCOMPARE(object.toString(), QString("[object Object]"));
    QCOMPARE(qjsvalue_cast<QString>(object), QString("[object Object]"));

    // FIXME: No c-style callbacks currently
#if 0
    QJSValue fun = eng.newFunction(myFunction);
    QCOMPARE(fun.toString().simplified(), QString("function () { [native code] }"));
    QCOMPARE(qscriptvalue_cast<QString>(fun).simplified(), QString("function () { [native code] }"));
#endif

    // toString() that throws exception
    {
        QJSValue objectObject = eng.evaluate(
            "(function(){"
            "  o = { };"
            "  o.toString = function() { throw new Error('toString'); };"
            "  return o;"
            "})()");
        QCOMPARE(objectObject.toString(), QLatin1String("Error: toString"));
        QVERIFY(eng.hasUncaughtException());
        QCOMPARE(eng.uncaughtException().toString(), QLatin1String("Error: toString"));
    }
    {
        eng.clearExceptions();
        QJSValue objectObject = eng.evaluate(
            "(function(){"
            "  var f = function() {};"
            "  f.prototype = Date;"
            "  return new f;"
            "})()");
        QVERIFY(!eng.hasUncaughtException());
        QVERIFY(objectObject.isObject());
        QCOMPARE(objectObject.toString(), QString::fromLatin1("TypeError: Function.prototype.toString is not generic"));
        QVERIFY(eng.hasUncaughtException());
        eng.clearExceptions();
    }

    QJSValue inv = QJSValue();
    QCOMPARE(inv.toString(), QString::fromLatin1("undefined"));

    // V2 constructors
    {
        QJSValue falskt = QJSValue(false);
        QCOMPARE(falskt.toString(), QString("false"));
        QCOMPARE(qjsvalue_cast<QString>(falskt), QString("false"));

        QJSValue sant = QJSValue(true);
        QCOMPARE(sant.toString(), QString("true"));
        QCOMPARE(qjsvalue_cast<QString>(sant), QString("true"));

        QJSValue number = QJSValue(123);
        QCOMPARE(number.toString(), QString("123"));
        QCOMPARE(qjsvalue_cast<QString>(number), QString("123"));

        QJSValue number2(int(0x43211234));
        QCOMPARE(number2.toString(), QString("1126240820"));

        QJSValue str = QJSValue(QString("ciao"));
        QCOMPARE(str.toString(), QString("ciao"));
        QCOMPARE(qjsvalue_cast<QString>(str), QString("ciao"));
    }

    // variant should use internal valueOf(), then fall back to QVariant::toString(),
    // then fall back to "QVariant(typename)"
    QJSValue variant = eng.toScriptValue(QPoint(10, 20));
    QVERIFY(variant.isVariant());
    QCOMPARE(variant.toString(), QString::fromLatin1("QVariant(QPoint)"));
    variant = eng.toScriptValue(QUrl());
    QVERIFY(variant.isVariant());
    QVERIFY(variant.toString().isEmpty());
}

void tst_QJSValue::toNumber()
{
    QJSEngine eng;

    QJSValue undefined = eng.undefinedValue();
    QCOMPARE(qIsNaN(undefined.toNumber()), true);
    QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(undefined)), true);

    QJSValue null = eng.nullValue();
    QCOMPARE(null.toNumber(), 0.0);
    QCOMPARE(qjsvalue_cast<qreal>(null), 0.0);

    {
        QJSValue falskt = eng.toScriptValue(false);
        QCOMPARE(falskt.toNumber(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSValue sant = eng.toScriptValue(true);
        QCOMPARE(sant.toNumber(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSValue number = eng.toScriptValue(123.0);
        QCOMPARE(number.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSValue str = eng.toScriptValue(QString("ciao"));
        QCOMPARE(qIsNaN(str.toNumber()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSValue str2 = eng.toScriptValue(QString("123"));
        QCOMPARE(str2.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }

    QJSValue object = eng.newObject();
    QCOMPARE(qIsNaN(object.toNumber()), true);
    QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(object)), true);

    // FIXME: No c-style callbacks currently
#if 0
    QJSValue fun = eng.newFunction(myFunction);
    QCOMPARE(qIsNaN(fun.toNumber()), true);
    QCOMPARE(qIsNaN(qscriptvalue_cast<qreal>(fun)), true);
#endif

    QJSValue inv = QJSValue();
    QVERIFY(qIsNaN(inv.toNumber()));
    QVERIFY(qIsNaN(qjsvalue_cast<qreal>(inv)));

    // V2 constructors
    {
        QJSValue falskt = QJSValue(false);
        QCOMPARE(falskt.toNumber(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSValue sant = QJSValue(true);
        QCOMPARE(sant.toNumber(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSValue number = QJSValue(123.0);
        QCOMPARE(number.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSValue number2(int(0x43211234));
        QCOMPARE(number2.toNumber(), 1126240820.0);

        QJSValue str = QJSValue(QString("ciao"));
        QCOMPARE(qIsNaN(str.toNumber()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSValue str2 = QJSValue(QString("123"));
        QCOMPARE(str2.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }
}

void tst_QJSValue::toBoolean() // deprecated
{
    QJSEngine eng;

    QJSValue undefined = eng.undefinedValue();
    QCOMPARE(undefined.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(undefined), false);

    QJSValue null = eng.nullValue();
    QCOMPARE(null.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(null), false);

    {
        QJSValue falskt = eng.toScriptValue(false);
        QCOMPARE(falskt.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSValue sant = eng.toScriptValue(true);
        QCOMPARE(sant.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSValue number = eng.toScriptValue(0.0);
        QCOMPARE(number.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSValue number2 = eng.toScriptValue(qSNaN());
        QCOMPARE(number2.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSValue number3 = eng.toScriptValue(123.0);
        QCOMPARE(number3.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSValue number4 = eng.toScriptValue(-456.0);
        QCOMPARE(number4.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSValue str = eng.toScriptValue(QString(""));
        QCOMPARE(str.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSValue str2 = eng.toScriptValue(QString("123"));
        QCOMPARE(str2.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }

    QJSValue object = eng.newObject();
    QCOMPARE(object.toBool(), true);
    QCOMPARE(qjsvalue_cast<bool>(object), true);

    // FIXME: No c-style callbacks currently
#if 0
    QJSValue fun = eng.newFunction(myFunction);
    QCOMPARE(fun.toBoolean(), true);
    QCOMPARE(qscriptvalue_cast<bool>(fun), true);
#endif

    QJSValue inv = QJSValue();
    QCOMPARE(inv.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(inv), false);

    // V2 constructors
    {
        QJSValue falskt = QJSValue(false);
        QCOMPARE(falskt.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSValue sant = QJSValue(true);
        QCOMPARE(sant.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSValue number = QJSValue(0.0);
        QCOMPARE(number.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSValue number2 = QJSValue(qSNaN());
        QCOMPARE(number2.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSValue number3 = QJSValue(123.0);
        QCOMPARE(number3.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSValue number4 = QJSValue(-456.0);
        QCOMPARE(number4.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSValue number5 = QJSValue(0x43211234);
        QCOMPARE(number5.toBool(), true);

        QJSValue str = QJSValue(QString(""));
        QCOMPARE(str.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSValue str2 = QJSValue(QString("123"));
        QCOMPARE(str2.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }
}

void tst_QJSValue::toBool()
{
    QJSEngine eng;

    QJSValue undefined = eng.undefinedValue();
    QCOMPARE(undefined.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(undefined), false);

    QJSValue null = eng.nullValue();
    QCOMPARE(null.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(null), false);

    {
        QJSValue falskt = eng.toScriptValue(false);
        QCOMPARE(falskt.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSValue sant = eng.toScriptValue(true);
        QCOMPARE(sant.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSValue number = eng.toScriptValue(0.0);
        QCOMPARE(number.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSValue number2 = eng.toScriptValue(qSNaN());
        QCOMPARE(number2.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSValue number3 = eng.toScriptValue(123.0);
        QCOMPARE(number3.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSValue number4 = eng.toScriptValue(-456.0);
        QCOMPARE(number4.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSValue str = eng.toScriptValue(QString(""));
        QCOMPARE(str.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSValue str2 = eng.toScriptValue(QString("123"));
        QCOMPARE(str2.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }

    QJSValue object = eng.newObject();
    QCOMPARE(object.toBool(), true);
    QCOMPARE(qjsvalue_cast<bool>(object), true);

    // FIXME: No c-style callbacks currently
#if 0
    QJSValue fun = eng.newFunction(myFunction);
    QCOMPARE(fun.toBool(), true);
    QCOMPARE(qscriptvalue_cast<bool>(fun), true);
#endif

    QJSValue inv = QJSValue();
    QCOMPARE(inv.toBool(), false);
    QCOMPARE(qjsvalue_cast<bool>(inv), false);

    // V2 constructors
    {
        QJSValue falskt = QJSValue(false);
        QCOMPARE(falskt.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSValue sant = QJSValue(true);
        QCOMPARE(sant.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSValue number = QJSValue(0.0);
        QCOMPARE(number.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSValue number2 = QJSValue(qSNaN());
        QCOMPARE(number2.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSValue number3 = QJSValue(123.0);
        QCOMPARE(number3.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSValue number4 = QJSValue(-456.0);
        QCOMPARE(number4.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSValue number5 = QJSValue(0x43211234);
        QCOMPARE(number5.toBool(), true);

        QJSValue str = QJSValue(QString(""));
        QCOMPARE(str.toBool(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSValue str2 = QJSValue(QString("123"));
        QCOMPARE(str2.toBool(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }
}

void tst_QJSValue::toInt()
{
    QJSEngine eng;

    {
        QJSValue zer0 = eng.toScriptValue(0.0);
        QCOMPARE(zer0.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(zer0), 0);

        QJSValue number = eng.toScriptValue(123.0);
        QCOMPARE(number.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(number), 123);

        QJSValue number2 = eng.toScriptValue(qSNaN());
        QCOMPARE(number2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number2), 0);

        QJSValue number3 = eng.toScriptValue(+qInf());
        QCOMPARE(number3.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number3), 0);

        QJSValue number3_2 = eng.toScriptValue(-qInf());
        QCOMPARE(number3_2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number3_2), 0);

        QJSValue number4 = eng.toScriptValue(0.5);
        QCOMPARE(number4.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number4), 0);

        QJSValue number5 = eng.toScriptValue(123.5);
        QCOMPARE(number5.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(number5), 123);

        QJSValue number6 = eng.toScriptValue(-456.5);
        QCOMPARE(number6.toInt(), -456);
        QCOMPARE(qjsvalue_cast<qint32>(number6), -456);

        QJSValue str = eng.toScriptValue(QString::fromLatin1("123.0"));
        QCOMPARE(str.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(str), 123);

        QJSValue str2 = eng.toScriptValue(QString::fromLatin1("NaN"));
        QCOMPARE(str2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str2), 0);

        QJSValue str3 = eng.toScriptValue(QString::fromLatin1("Infinity"));
        QCOMPARE(str3.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str3), 0);

        QJSValue str3_2 = eng.toScriptValue(QString::fromLatin1("-Infinity"));
        QCOMPARE(str3_2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str3_2), 0);

        QJSValue str4 = eng.toScriptValue(QString::fromLatin1("0.5"));
        QCOMPARE(str4.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str4), 0);

        QJSValue str5 = eng.toScriptValue(QString::fromLatin1("123.5"));
        QCOMPARE(str5.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(str5), 123);

        QJSValue str6 = eng.toScriptValue(QString::fromLatin1("-456.5"));
        QCOMPARE(str6.toInt(), -456);
        QCOMPARE(qjsvalue_cast<qint32>(str6), -456);
    }
    // V2 constructors
    {
        QJSValue zer0 = QJSValue(0.0);
        QCOMPARE(zer0.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(zer0), 0);

        QJSValue number = QJSValue(123.0);
        QCOMPARE(number.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(number), 123);

        QJSValue number2 = QJSValue(qSNaN());
        QCOMPARE(number2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number2), 0);

        QJSValue number3 = QJSValue(+qInf());
        QCOMPARE(number3.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number3), 0);

        QJSValue number3_2 = QJSValue(-qInf());
        QCOMPARE(number3_2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number3_2), 0);

        QJSValue number4 = QJSValue(0.5);
        QCOMPARE(number4.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(number4), 0);

        QJSValue number5 = QJSValue(123.5);
        QCOMPARE(number5.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(number5), 123);

        QJSValue number6 = QJSValue(-456.5);
        QCOMPARE(number6.toInt(), -456);
        QCOMPARE(qjsvalue_cast<qint32>(number6), -456);

        QJSValue number7 = QJSValue(0x43211234);
        QCOMPARE(number7.toInt(), 0x43211234);

        QJSValue str = QJSValue("123.0");
        QCOMPARE(str.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(str), 123);

        QJSValue str2 = QJSValue("NaN");
        QCOMPARE(str2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str2), 0);

        QJSValue str3 = QJSValue("Infinity");
        QCOMPARE(str3.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str3), 0);

        QJSValue str3_2 = QJSValue("-Infinity");
        QCOMPARE(str3_2.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str3_2), 0);

        QJSValue str4 = QJSValue("0.5");
        QCOMPARE(str4.toInt(), 0);
        QCOMPARE(qjsvalue_cast<qint32>(str4), 0);

        QJSValue str5 = QJSValue("123.5");
        QCOMPARE(str5.toInt(), 123);
        QCOMPARE(qjsvalue_cast<qint32>(str5), 123);

        QJSValue str6 = QJSValue("-456.5");
        QCOMPARE(str6.toInt(), -456);
        QCOMPARE(qjsvalue_cast<qint32>(str6), -456);
    }

    QJSValue inv;
    QCOMPARE(inv.toInt(), 0);
    QCOMPARE(qjsvalue_cast<qint32>(inv), 0);
}

void tst_QJSValue::toUInt()
{
    QJSEngine eng;

    {
        QJSValue zer0 = eng.toScriptValue(0.0);
        QCOMPARE(zer0.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(zer0), quint32(0));

        QJSValue number = eng.toScriptValue(123.0);
        QCOMPARE(number.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(number), quint32(123));

        QJSValue number2 = eng.toScriptValue(qSNaN());
        QCOMPARE(number2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number2), quint32(0));

        QJSValue number3 = eng.toScriptValue(+qInf());
        QCOMPARE(number3.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number3), quint32(0));

        QJSValue number3_2 = eng.toScriptValue(-qInf());
        QCOMPARE(number3_2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number3_2), quint32(0));

        QJSValue number4 = eng.toScriptValue(0.5);
        QCOMPARE(number4.toUInt(), quint32(0));

        QJSValue number5 = eng.toScriptValue(123.5);
        QCOMPARE(number5.toUInt(), quint32(123));

        QJSValue number6 = eng.toScriptValue(-456.5);
        QCOMPARE(number6.toUInt(), quint32(-456));
        QCOMPARE(qjsvalue_cast<quint32>(number6), quint32(-456));

        QJSValue str = eng.toScriptValue(QString::fromLatin1("123.0"));
        QCOMPARE(str.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(str), quint32(123));

        QJSValue str2 = eng.toScriptValue(QString::fromLatin1("NaN"));
        QCOMPARE(str2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str2), quint32(0));

        QJSValue str3 = eng.toScriptValue(QString::fromLatin1("Infinity"));
        QCOMPARE(str3.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str3), quint32(0));

        QJSValue str3_2 = eng.toScriptValue(QString::fromLatin1("-Infinity"));
        QCOMPARE(str3_2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str3_2), quint32(0));

        QJSValue str4 = eng.toScriptValue(QString::fromLatin1("0.5"));
        QCOMPARE(str4.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str4), quint32(0));

        QJSValue str5 = eng.toScriptValue(QString::fromLatin1("123.5"));
        QCOMPARE(str5.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(str5), quint32(123));

        QJSValue str6 = eng.toScriptValue(QString::fromLatin1("-456.5"));
        QCOMPARE(str6.toUInt(), quint32(-456));
        QCOMPARE(qjsvalue_cast<quint32>(str6), quint32(-456));
    }
    // V2 constructors
    {
        QJSValue zer0 = QJSValue(0.0);
        QCOMPARE(zer0.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(zer0), quint32(0));

        QJSValue number = QJSValue(123.0);
        QCOMPARE(number.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(number), quint32(123));

        QJSValue number2 = QJSValue(qSNaN());
        QCOMPARE(number2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number2), quint32(0));

        QJSValue number3 = QJSValue(+qInf());
        QCOMPARE(number3.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number3), quint32(0));

        QJSValue number3_2 = QJSValue(-qInf());
        QCOMPARE(number3_2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(number3_2), quint32(0));

        QJSValue number4 = QJSValue(0.5);
        QCOMPARE(number4.toUInt(), quint32(0));

        QJSValue number5 = QJSValue(123.5);
        QCOMPARE(number5.toUInt(), quint32(123));

        QJSValue number6 = QJSValue(-456.5);
        QCOMPARE(number6.toUInt(), quint32(-456));
        QCOMPARE(qjsvalue_cast<quint32>(number6), quint32(-456));

        QJSValue number7 = QJSValue(0x43211234);
        QCOMPARE(number7.toUInt(), quint32(0x43211234));

        QJSValue str = QJSValue(QLatin1String("123.0"));
        QCOMPARE(str.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(str), quint32(123));

        QJSValue str2 = QJSValue(QLatin1String("NaN"));
        QCOMPARE(str2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str2), quint32(0));

        QJSValue str3 = QJSValue(QLatin1String("Infinity"));
        QCOMPARE(str3.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str3), quint32(0));

        QJSValue str3_2 = QJSValue(QLatin1String("-Infinity"));
        QCOMPARE(str3_2.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str3_2), quint32(0));

        QJSValue str4 = QJSValue(QLatin1String("0.5"));
        QCOMPARE(str4.toUInt(), quint32(0));
        QCOMPARE(qjsvalue_cast<quint32>(str4), quint32(0));

        QJSValue str5 = QJSValue(QLatin1String("123.5"));
        QCOMPARE(str5.toUInt(), quint32(123));
        QCOMPARE(qjsvalue_cast<quint32>(str5), quint32(123));

        QJSValue str6 = QJSValue(QLatin1String("-456.5"));
        QCOMPARE(str6.toUInt(), quint32(-456));
        QCOMPARE(qjsvalue_cast<quint32>(str6), quint32(-456));
    }

    QJSValue inv;
    QCOMPARE(inv.toUInt(), quint32(0));
    QCOMPARE(qjsvalue_cast<quint32>(inv), quint32(0));
}

#if defined Q_CC_MSVC && _MSC_VER < 1300
Q_DECLARE_METATYPE(QVariant)
#endif

void tst_QJSValue::toVariant()
{
    QJSEngine eng;

    QJSValue undefined = eng.undefinedValue();
    QCOMPARE(undefined.toVariant(), QVariant());
    QCOMPARE(qjsvalue_cast<QVariant>(undefined), QVariant());

    QJSValue null = eng.nullValue();
    QCOMPARE(null.toVariant(), QVariant());
    QCOMPARE(qjsvalue_cast<QVariant>(null), QVariant());

    {
        QJSValue number = eng.toScriptValue(123.0);
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSValue intNumber = eng.toScriptValue((qint32)123);
        QCOMPARE(intNumber.toVariant().type(), QVariant((qint32)123).type());
        QCOMPARE((qjsvalue_cast<QVariant>(number)).type(), QVariant((qint32)123).type());

        QJSValue falskt = eng.toScriptValue(false);
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSValue sant = eng.toScriptValue(true);
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSValue str = eng.toScriptValue(QString("ciao"));
        QCOMPARE(str.toVariant(), QVariant(QString("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QString("ciao")));
    }

    QJSValue object = eng.newObject();
    QCOMPARE(object.toVariant(), QVariant(QVariantMap()));

    QJSValue qobject = eng.newQObject(this);
    {
        QVariant var = qobject.toVariant();
        QCOMPARE(var.userType(), int(QMetaType::QObjectStar));
        QCOMPARE(qVariantValue<QObject*>(var), (QObject *)this);
    }

    {
        QDateTime dateTime = QDateTime(QDate(1980, 10, 4));
        QJSValue dateObject = eng.toScriptValue(dateTime);
        QVariant var = dateObject.toVariant();
        QCOMPARE(var, QVariant(dateTime));
    }

    {
        QRegExp rx = QRegExp("[0-9a-z]+", Qt::CaseSensitive, QRegExp::RegExp2);
        QJSValue rxObject = eng.toScriptValue(rx);
        QVERIFY(rxObject.isRegExp());
        QVariant var = rxObject.toVariant();
        QCOMPARE(var, QVariant(rx));
    }

    QJSValue inv;
    QCOMPARE(inv.toVariant(), QVariant());
    QCOMPARE(qjsvalue_cast<QVariant>(inv), QVariant());

    // V2 constructors
    {
        QJSValue number = QJSValue(123.0);
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSValue falskt = QJSValue(false);
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSValue sant = QJSValue(true);
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSValue str = QJSValue(QString("ciao"));
        QCOMPARE(str.toVariant(), QVariant(QString("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QString("ciao")));
    }

#if 0 // FIXME: No automatic sequence conversion
    // array
    {
        QVariantList listIn;
        listIn << 123 << "hello";
        QJSValue array = qScriptValueFromValue(&eng, listIn);
        QVERIFY(array.isArray());
        QCOMPARE(array.property("length").toInt(), 2);
        QVariant ret = array.toVariant();
        QCOMPARE(ret.type(), QVariant::List);
        QVariantList listOut = ret.toList();
        QCOMPARE(listOut.size(), listIn.size());
        for (int i = 0; i < listIn.size(); ++i)
            QVERIFY(listOut.at(i) == listIn.at(i));
        // round-trip conversion
        QJSValue array2 = qScriptValueFromValue(&eng, ret);
        QVERIFY(array2.isArray());
        QCOMPARE(array2.property("length").toInt(), array.property("length").toInt());
        for (int i = 0; i < array.property("length").toInt(); ++i)
            QVERIFY(array2.property(i).strictlyEquals(array.property(i)));
    }
#endif
}

void tst_QJSValue::toQObject_nonQObject_data()
{
    newEngine();
    QTest::addColumn<QJSValue>("value");

    QTest::newRow("invalid") << QJSValue();
    QTest::newRow("bool(false)") << QJSValue(false);
    QTest::newRow("bool(true)") << QJSValue(true);
    QTest::newRow("int") << QJSValue(123);
    QTest::newRow("string") << QJSValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);
    QTest::newRow("null") << QJSValue(QJSValue::NullValue);

    QTest::newRow("bool bound(false)") << engine->toScriptValue(false);
    QTest::newRow("bool bound(true)") << engine->toScriptValue(true);
    QTest::newRow("int bound") << engine->toScriptValue(123);
    QTest::newRow("string bound") << engine->toScriptValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined bound") << engine->undefinedValue();
    QTest::newRow("null bound") << engine->nullValue();
    QTest::newRow("object") << engine->newObject();
    QTest::newRow("array") << engine->newArray();
    QTest::newRow("date") << engine->evaluate("new Date(124)");
    QTest::newRow("variant(12345)") << engine->toScriptValue(QVariant(12345));
    QTest::newRow("variant((QObject*)0)") << engine->toScriptValue(qVariantFromValue((QObject*)0));
    QTest::newRow("newQObject(0)") << engine->newQObject(0);
}


void tst_QJSValue::toQObject_nonQObject()
{
    QFETCH(QJSValue, value);
    QCOMPARE(value.toQObject(), (QObject *)0);
    QCOMPARE(qjsvalue_cast<QObject*>(value), (QObject *)0);
}

// unfortunately, this is necessary in order to do qscriptvalue_cast<QPushButton*>(...)
Q_DECLARE_METATYPE(QPushButton*);

void tst_QJSValue::toQObject()
{
    QJSEngine eng;

    QJSValue qobject = eng.newQObject(this);
    QCOMPARE(qobject.toQObject(), (QObject *)this);
    QCOMPARE(qjsvalue_cast<QObject*>(qobject), (QObject *)this);
    QCOMPARE(qjsvalue_cast<QWidget*>(qobject), (QWidget *)0);

    QWidget widget;
    QJSValue qwidget = eng.newQObject(&widget);
    QCOMPARE(qwidget.toQObject(), (QObject *)&widget);
    QCOMPARE(qjsvalue_cast<QObject*>(qwidget), (QObject *)&widget);
    QCOMPARE(qjsvalue_cast<QWidget*>(qwidget), &widget);

    QPushButton button;
    QJSValue qbutton = eng.newQObject(&button);
    QCOMPARE(qbutton.toQObject(), (QObject *)&button);
    QCOMPARE(qjsvalue_cast<QObject*>(qbutton), (QObject *)&button);
    QCOMPARE(qjsvalue_cast<QWidget*>(qbutton), (QWidget *)&button);
    QCOMPARE(qjsvalue_cast<QPushButton*>(qbutton), &button);
}

void tst_QJSValue::toDateTime()
{
    QJSEngine eng;
    QDateTime dt = eng.evaluate("new Date(0)").toDateTime();
    QVERIFY(dt.isValid());
    QCOMPARE(dt.timeSpec(), Qt::LocalTime);
    QCOMPARE(dt.toUTC(), QDateTime(QDate(1970, 1, 1), QTime(0, 0, 0), Qt::UTC));

    QVERIFY(!eng.evaluate("[]").toDateTime().isValid());
    QVERIFY(!eng.evaluate("{}").toDateTime().isValid());
    QVERIFY(!eng.globalObject().toDateTime().isValid());
    QVERIFY(!QJSValue().toDateTime().isValid());
    QVERIFY(!QJSValue(123).toDateTime().isValid());
    QVERIFY(!QJSValue(false).toDateTime().isValid());
    QVERIFY(!eng.nullValue().toDateTime().isValid());
    QVERIFY(!eng.undefinedValue().toDateTime().isValid());
}

void tst_QJSValue::toRegExp()
{
    QJSEngine eng;
    {
        QRegExp rx = qjsvalue_cast<QRegExp>(eng.evaluate("/foo/"));
        QVERIFY(rx.isValid());
        QCOMPARE(rx.patternSyntax(), QRegExp::RegExp2);
        QCOMPARE(rx.pattern(), QString::fromLatin1("foo"));
        QCOMPARE(rx.caseSensitivity(), Qt::CaseSensitive);
        QVERIFY(!rx.isMinimal());
    }
    {
        QRegExp rx = qjsvalue_cast<QRegExp>(eng.evaluate("/bar/gi"));
        QVERIFY(rx.isValid());
        QCOMPARE(rx.patternSyntax(), QRegExp::RegExp2);
        QCOMPARE(rx.pattern(), QString::fromLatin1("bar"));
        QCOMPARE(rx.caseSensitivity(), Qt::CaseInsensitive);
        QVERIFY(!rx.isMinimal());
    }

    QVERIFY(qjsvalue_cast<QRegExp>(eng.evaluate("[]")).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(eng.evaluate("{}")).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(eng.globalObject()).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(QJSValue()).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(QJSValue(123)).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(QJSValue(false)).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(eng.nullValue()).isEmpty());
    QVERIFY(qjsvalue_cast<QRegExp>(eng.undefinedValue()).isEmpty());
}

void tst_QJSValue::isArray_data()
{
    newEngine();

    QTest::addColumn<QJSValue>("value");
    QTest::addColumn<bool>("array");

    QTest::newRow("[]") << engine->evaluate("[]") << true;
    QTest::newRow("{}") << engine->evaluate("{}") << false;
    QTest::newRow("globalObject") << engine->globalObject() << false;
    QTest::newRow("invalid") << QJSValue() << false;
    QTest::newRow("number") << QJSValue(123) << false;
    QTest::newRow("bool") << QJSValue(false) << false;
    QTest::newRow("null") << engine->nullValue() << false;
    QTest::newRow("undefined") << engine->undefinedValue() << false;
}

void tst_QJSValue::isArray()
{
    QFETCH(QJSValue, value);
    QFETCH(bool, array);

    QCOMPARE(value.isArray(), array);
}

void tst_QJSValue::isDate_data()
{
    newEngine();

    QTest::addColumn<QJSValue>("value");
    QTest::addColumn<bool>("date");

    QTest::newRow("date") << engine->evaluate("new Date()") << true;
    QTest::newRow("[]") << engine->evaluate("[]") << false;
    QTest::newRow("{}") << engine->evaluate("{}") << false;
    QTest::newRow("globalObject") << engine->globalObject() << false;
    QTest::newRow("invalid") << QJSValue() << false;
    QTest::newRow("number") << QJSValue(123) << false;
    QTest::newRow("bool") << QJSValue(false) << false;
    QTest::newRow("null") << engine->nullValue() << false;
    QTest::newRow("undefined") << engine->undefinedValue() << false;
}

void tst_QJSValue::isDate()
{
    QFETCH(QJSValue, value);
    QFETCH(bool, date);

    QCOMPARE(value.isDate(), date);
}

void tst_QJSValue::isError_propertiesOfGlobalObject()
{
    QStringList errors;
    errors << "Error"
           << "EvalError"
           << "RangeError"
           << "ReferenceError"
           << "SyntaxError"
           << "TypeError"
           << "URIError";
    QJSEngine eng;
    for (int i = 0; i < errors.size(); ++i) {
        QJSValue ctor = eng.globalObject().property(errors.at(i));
        QVERIFY(ctor.isCallable());
        QVERIFY(ctor.property("prototype").isError());
    }
}

void tst_QJSValue::isError_data()
{
    newEngine();

    QTest::addColumn<QJSValue>("value");
    QTest::addColumn<bool>("error");

    QTest::newRow("syntax error") << engine->evaluate("%fsdg's") << true;
    QTest::newRow("[]") << engine->evaluate("[]") << false;
    QTest::newRow("{}") << engine->evaluate("{}") << false;
    QTest::newRow("globalObject") << engine->globalObject() << false;
    QTest::newRow("invalid") << QJSValue() << false;
    QTest::newRow("number") << QJSValue(123) << false;
    QTest::newRow("bool") << QJSValue(false) << false;
    QTest::newRow("null") << engine->nullValue() << false;
    QTest::newRow("undefined") << engine->undefinedValue() << false;
    QTest::newRow("newObject") << engine->newObject() << false;
    QTest::newRow("new Object") << engine->evaluate("new Object()") << false;
}

void tst_QJSValue::isError()
{
    QFETCH(QJSValue, value);
    QFETCH(bool, error);

    QCOMPARE(value.isError(), error);
}

void tst_QJSValue::isRegExp_data()
{
    newEngine();

    QTest::addColumn<QJSValue>("value");
    QTest::addColumn<bool>("regexp");

    QTest::newRow("/foo/") << engine->evaluate("/foo/") << true;
    QTest::newRow("[]") << engine->evaluate("[]") << false;
    QTest::newRow("{}") << engine->evaluate("{}") << false;
    QTest::newRow("globalObject") << engine->globalObject() << false;
    QTest::newRow("invalid") << QJSValue() << false;
    QTest::newRow("number") << QJSValue(123) << false;
    QTest::newRow("bool") << QJSValue(false) << false;
    QTest::newRow("null") << engine->nullValue() << false;
    QTest::newRow("undefined") << engine->undefinedValue() << false;
}

void tst_QJSValue::isRegExp()
{
    QFETCH(QJSValue, value);
    QFETCH(bool, regexp);

    QCOMPARE(value.isRegExp(), regexp);
}

#if 0 // FIXME: No c-style callbacks currently
static QJSValue getter(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->thisObject().property("x");
}

static QJSValue setter(QScriptContext *ctx, QScriptEngine *)
{
    ctx->thisObject().setProperty("x", ctx->argument(0));
    return ctx->argument(0);
}

static QJSValue getterSetter(QScriptContext *ctx, QScriptEngine *)
{
    if (ctx->argumentCount() > 0)
        ctx->thisObject().setProperty("x", ctx->argument(0));
    return ctx->thisObject().property("x");
}

static QJSValue getterSetterThrowingError(QScriptContext *ctx, QScriptEngine *)
{
    if (ctx->argumentCount() > 0)
        return ctx->throwError("set foo");
    else
        return ctx->throwError("get foo");
}

static QJSValue getSet__proto__(QScriptContext *ctx, QScriptEngine *)
{
    if (ctx->argumentCount() > 0)
        ctx->callee().setProperty("value", ctx->argument(0));
    return ctx->callee().property("value");
}
#endif

void tst_QJSValue::hasProperty_basic()
{
    QJSEngine eng;
    QJSValue obj = eng.newObject();
    QVERIFY(obj.hasProperty("hasOwnProperty")); // inherited from Object.prototype
    QVERIFY(!obj.hasOwnProperty("hasOwnProperty"));

    QVERIFY(!obj.hasProperty("foo"));
    QVERIFY(!obj.hasOwnProperty("foo"));
    obj.setProperty("foo", 123);
    QVERIFY(obj.hasProperty("foo"));
    QVERIFY(obj.hasOwnProperty("foo"));

    QVERIFY(!obj.hasProperty("bar"));
    QVERIFY(!obj.hasOwnProperty("bar"));
}

void tst_QJSValue::hasProperty_globalObject()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();
    QVERIFY(global.hasProperty("Math"));
    QVERIFY(global.hasOwnProperty("Math"));
    QVERIFY(!global.hasProperty("NoSuchStandardProperty"));
    QVERIFY(!global.hasOwnProperty("NoSuchStandardProperty"));

    QVERIFY(!global.hasProperty("foo"));
    QVERIFY(!global.hasOwnProperty("foo"));
    global.setProperty("foo", 123);
    QVERIFY(global.hasProperty("foo"));
    QVERIFY(global.hasOwnProperty("foo"));
}

void tst_QJSValue::hasProperty_changePrototype()
{
    QJSEngine eng;
    QJSValue obj = eng.newObject();
    QJSValue proto = eng.newObject();
    obj.setPrototype(proto);

    QVERIFY(!obj.hasProperty("foo"));
    QVERIFY(!obj.hasOwnProperty("foo"));
    proto.setProperty("foo", 123);
    QVERIFY(obj.hasProperty("foo"));
    QVERIFY(!obj.hasOwnProperty("foo"));

    obj.setProperty("foo", 456); // override prototype property
    QVERIFY(obj.hasProperty("foo"));
    QVERIFY(obj.hasOwnProperty("foo"));
}

void tst_QJSValue::deleteProperty_basic()
{
    QJSEngine eng;
    QJSValue obj = eng.newObject();
    // deleteProperty() behavior matches JS delete operator
    QVERIFY(obj.deleteProperty("foo"));

    obj.setProperty("foo", 123);
    QVERIFY(obj.deleteProperty("foo"));
    QVERIFY(!obj.hasOwnProperty("foo"));
}

void tst_QJSValue::deleteProperty_globalObject()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();
    // deleteProperty() behavior matches JS delete operator
    QVERIFY(global.deleteProperty("foo"));

    global.setProperty("foo", 123);
    QVERIFY(global.deleteProperty("foo"));
    QVERIFY(!global.hasProperty("foo"));

    QVERIFY(global.deleteProperty("Math"));
    QVERIFY(!global.hasProperty("Math"));

    QVERIFY(!global.deleteProperty("NaN")); // read-only
    QVERIFY(global.hasProperty("NaN"));
}

void tst_QJSValue::deleteProperty_inPrototype()
{
    QJSEngine eng;
    QJSValue obj = eng.newObject();
    QJSValue proto = eng.newObject();
    obj.setPrototype(proto);

    proto.setProperty("foo", 123);
    QVERIFY(obj.hasProperty("foo"));
    // deleteProperty() behavior matches JS delete operator
    QVERIFY(obj.deleteProperty("foo"));
    QVERIFY(obj.hasProperty("foo"));
}

void tst_QJSValue::getSetProperty_HooliganTask162051()
{
    QJSEngine eng;
    // task 162051 -- detecting whether the property is an array index or not
    QVERIFY(eng.evaluate("a = []; a['00'] = 123; a['00']").strictlyEquals(eng.toScriptValue(123)));
    QVERIFY(eng.evaluate("a.length").strictlyEquals(eng.toScriptValue(0)));
    QVERIFY(eng.evaluate("a.hasOwnProperty('00')").strictlyEquals(eng.toScriptValue(true)));
    QVERIFY(eng.evaluate("a.hasOwnProperty('0')").strictlyEquals(eng.toScriptValue(false)));
    QVERIFY(eng.evaluate("a[0]").isUndefined());
    QVERIFY(eng.evaluate("a[0.5] = 456; a[0.5]").strictlyEquals(eng.toScriptValue(456)));
    QVERIFY(eng.evaluate("a.length").strictlyEquals(eng.toScriptValue(0)));
    QVERIFY(eng.evaluate("a.hasOwnProperty('0.5')").strictlyEquals(eng.toScriptValue(true)));
    QVERIFY(eng.evaluate("a[0]").isUndefined());
    QVERIFY(eng.evaluate("a[0] = 789; a[0]").strictlyEquals(eng.toScriptValue(789)));
    QVERIFY(eng.evaluate("a.length").strictlyEquals(eng.toScriptValue(1)));
}

void tst_QJSValue::getSetProperty_HooliganTask183072()
{
    QJSEngine eng;
    // task 183072 -- 0x800000000 is not an array index
    eng.evaluate("a = []; a[0x800000000] = 123");
    QVERIFY(eng.evaluate("a.length").strictlyEquals(eng.toScriptValue(0)));
    QVERIFY(eng.evaluate("a[0]").isUndefined());
    QVERIFY(eng.evaluate("a[0x800000000]").strictlyEquals(eng.toScriptValue(123)));
}

void tst_QJSValue::getSetProperty_propertyRemoval()
{
    QJSEngine eng;
    QJSValue object = eng.newObject();
    QJSValue str = eng.toScriptValue(QString::fromLatin1("bar"));
    QJSValue num = eng.toScriptValue(123.0);

    object.setProperty("foo", num);
    QCOMPARE(object.property("foo").strictlyEquals(num), true);
    object.setProperty("bar", str);
    QCOMPARE(object.property("bar").strictlyEquals(str), true);
    QVERIFY(object.deleteProperty("foo"));
    QVERIFY(!object.hasOwnProperty("foo"));
    QCOMPARE(object.property("bar").strictlyEquals(str), true);
    object.setProperty("foo", num);
    QCOMPARE(object.property("foo").strictlyEquals(num), true);
    QCOMPARE(object.property("bar").strictlyEquals(str), true);
    QVERIFY(object.deleteProperty("bar"));
    QVERIFY(!object.hasOwnProperty("bar"));
    QCOMPARE(object.property("foo").strictlyEquals(num), true);
    QVERIFY(object.deleteProperty("foo"));
    QVERIFY(!object.hasOwnProperty("foo"));

    eng.globalObject().setProperty("object3", object);
    QCOMPARE(eng.evaluate("object3.hasOwnProperty('foo')")
             .strictlyEquals(eng.toScriptValue(false)), true);
    object.setProperty("foo", num);
    QCOMPARE(eng.evaluate("object3.hasOwnProperty('foo')")
             .strictlyEquals(eng.toScriptValue(true)), true);
    QVERIFY(eng.globalObject().deleteProperty("object3"));
    QCOMPARE(eng.evaluate("this.hasOwnProperty('object3')")
             .strictlyEquals(eng.toScriptValue(false)), true);
}

void tst_QJSValue::getSetProperty_resolveMode()
{
    // test ResolveMode
    QJSEngine eng;
    QJSValue object = eng.newObject();
    QJSValue prototype = eng.newObject();
    object.setPrototype(prototype);
    QJSValue num2 = eng.toScriptValue(456.0);
    prototype.setProperty("propertyInPrototype", num2);
    // default is ResolvePrototype
    QCOMPARE(object.property("propertyInPrototype")
             .strictlyEquals(num2), true);
#if 0 // FIXME: ResolveFlags removed from API
    QCOMPARE(object.property("propertyInPrototype", QJSValue::ResolvePrototype)
             .strictlyEquals(num2), true);
    QCOMPARE(object.property("propertyInPrototype", QJSValue::ResolveLocal)
             .isValid(), false);
    QCOMPARE(object.property("propertyInPrototype", QJSValue::ResolveScope)
             .strictlyEquals(num2), false);
    QCOMPARE(object.property("propertyInPrototype", QJSValue::ResolveFull)
             .strictlyEquals(num2), true);
#endif
}

void tst_QJSValue::getSetProperty_twoEngines()
{
    QJSEngine engine;
    QJSValue object = engine.newObject();

    QJSEngine otherEngine;
    QJSValue otherNum = otherEngine.toScriptValue(123);
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setProperty(oof) failed: cannot set value created in a different engine");
    object.setProperty("oof", otherNum);
    QVERIFY(!object.hasOwnProperty("oof"));
    QVERIFY(object.property("oof").isUndefined());
}


void tst_QJSValue::getSetProperty_gettersAndSetters()
{
#if 0 // FIXME: No setters/getters right now
    QScriptEngine eng;
    QJSValue str = eng.toScriptValue(QString::fromLatin1("bar"));
    QJSValue num = eng.toScriptValue(123.0);
    QJSValue object = eng.newObject();
    for (int x = 0; x < 2; ++x) {
        object.deleteProperty("foo");
        // getter() returns this.x
        object.setProperty("foo", eng.newFunction(getter),
                            QJSValue::PropertyGetter | QJSValue::UserRange);
        QCOMPARE(object.propertyFlags("foo") & ~QJSValue::UserRange,
                 QJSValue::PropertyGetter );

        QEXPECT_FAIL("", "QTBUG-17615: User-range flags are not retained for getter/setter properties", Continue);
        QCOMPARE(object.propertyFlags("foo"),
                 QJSValue::PropertyGetter | QJSValue::UserRange);
        object.setProperty("x", num);
        QCOMPARE(object.property("foo").strictlyEquals(num), true);

        // setter() sets this.x
        object.setProperty("foo", eng.newFunction(setter),
                            QJSValue::PropertySetter);
        QCOMPARE(object.propertyFlags("foo") & ~QJSValue::UserRange,
                 QJSValue::PropertySetter | QJSValue::PropertyGetter);

        QCOMPARE(object.propertyFlags("foo"),
                 QJSValue::PropertySetter | QJSValue::PropertyGetter);
        object.setProperty("foo", str);
        QCOMPARE(object.property("x").strictlyEquals(str), true);
        QCOMPARE(object.property("foo").strictlyEquals(str), true);

        // kill the getter
        object.setProperty("foo", QJSValue(), QJSValue::PropertyGetter);
        QVERIFY(!(object.propertyFlags("foo") & QJSValue::PropertyGetter));
        QVERIFY(object.propertyFlags("foo") & QJSValue::PropertySetter);
        QCOMPARE(object.property("foo").isUndefined(), true);

        // setter should still work
        object.setProperty("foo", num);
        QCOMPARE(object.property("x").strictlyEquals(num), true);

        // kill the setter too
        object.setProperty("foo", QJSValue(), QJSValue::PropertySetter);
        QVERIFY(!(object.propertyFlags("foo") & QJSValue::PropertySetter));
        // now foo is just a regular property
        object.setProperty("foo", str);
        QCOMPARE(object.property("x").strictlyEquals(num), true);
        QCOMPARE(object.property("foo").strictlyEquals(str), true);
    }

    for (int x = 0; x < 2; ++x) {
        object.deleteProperty("foo");
        // setter() sets this.x
        object.setProperty("foo", eng.newFunction(setter), QJSValue::PropertySetter);
        object.setProperty("foo", str);
        QCOMPARE(object.property("x").strictlyEquals(str), true);
        QCOMPARE(object.property("foo").isUndefined(), true);

        // getter() returns this.x
        object.setProperty("foo", eng.newFunction(getter), QJSValue::PropertyGetter);
        object.setProperty("x", num);
        QCOMPARE(object.property("foo").strictlyEquals(num), true);

        // kill the setter
        object.setProperty("foo", QJSValue(), QJSValue::PropertySetter);
        object.setProperty("foo", str);

        // getter should still work
        QCOMPARE(object.property("foo").strictlyEquals(num), true);

        // kill the getter too
        object.setProperty("foo", QJSValue(), QJSValue::PropertyGetter);
        // now foo is just a regular property
        object.setProperty("foo", str);
        QCOMPARE(object.property("x").strictlyEquals(num), true);
        QCOMPARE(object.property("foo").strictlyEquals(str), true);
    }

    // use a single function as both getter and setter
    object.deleteProperty("foo");
    object.setProperty("foo", eng.newFunction(getterSetter),
                        QJSValue::PropertyGetter | QJSValue::PropertySetter);
    QCOMPARE(object.propertyFlags("foo"),
             QJSValue::PropertyGetter | QJSValue::PropertySetter);
    object.setProperty("x", num);
    QCOMPARE(object.property("foo").strictlyEquals(num), true);

    // killing the getter will preserve the setter, even though they are the same function
    object.setProperty("foo", QJSValue(), QJSValue::PropertyGetter);
    QVERIFY(object.propertyFlags("foo") & QJSValue::PropertySetter);
    QCOMPARE(object.property("foo").isUndefined(), true);
#endif
}

void tst_QJSValue::getSetProperty_gettersAndSettersThrowErrorNative()
{
#if 0 // FIXME: No setters/getters right now
    // getter/setter that throws an error
    QScriptEngine eng;
    QJSValue str = eng.toScriptValue("bar");
    QJSValue object = eng.newObject();

    object.setProperty("foo", eng.newFunction(getterSetterThrowingError),
                        QJSValue::PropertyGetter | QJSValue::PropertySetter);
    QVERIFY(!eng.hasUncaughtException());
    QJSValue ret = object.property("foo");
    QVERIFY(ret.isError());
    QVERIFY(eng.hasUncaughtException());
    QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
    QCOMPARE(ret.toString(), QLatin1String("Error: get foo"));
    eng.evaluate("Object"); // clear exception state...
    QVERIFY(!eng.hasUncaughtException());
    object.setProperty("foo", str);
    QVERIFY(eng.hasUncaughtException());
    QCOMPARE(eng.uncaughtException().toString(), QLatin1String("Error: set foo"));
#endif
}

void tst_QJSValue::getSetProperty_gettersAndSettersThrowErrorJS()
{
    // getter/setter that throws an error (from js function)
    QJSEngine eng;
    QJSValue str = eng.toScriptValue(QString::fromLatin1("bar"));

    eng.evaluate("o = new Object; "
                 "o.__defineGetter__('foo', function() { throw new Error('get foo') }); "
                 "o.__defineSetter__('foo', function() { throw new Error('set foo') }); ");
    QJSValue object = eng.evaluate("o");
    QVERIFY(!eng.hasUncaughtException());
    QJSValue ret = object.property("foo");
    QVERIFY(ret.isError());
    QVERIFY(eng.hasUncaughtException());
    QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
    QCOMPARE(ret.toString(), QLatin1String("Error: get foo"));
    eng.evaluate("Object"); // clear exception state...
    QVERIFY(!eng.hasUncaughtException());
    object.setProperty("foo", str);
    QVERIFY(eng.hasUncaughtException());
    QCOMPARE(eng.uncaughtException().toString(), QLatin1String("Error: set foo"));
}

void tst_QJSValue::getSetProperty_gettersAndSettersOnNative()
{
#if 0 // FIXME: No c-style functions right now
    // attempt to install getter+setter on built-in (native) property
    QScriptEngine eng;
    QJSValue object = eng.newObject();
    QVERIFY(object.property("__proto__").strictlyEquals(object.prototype()));

    QJSValue fun = eng.newFunction(getSet__proto__);
    fun.setProperty("value", eng.toScriptValue("boo"));
/*    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setProperty() failed: "
                         "cannot set getter or setter of native property "
                         "`__proto__'");*/
    object.setProperty("__proto__", fun,
                        QJSValue::PropertyGetter | QJSValue::PropertySetter
                        | QJSValue::UserRange);
    QVERIFY(object.property("__proto__").strictlyEquals(object.prototype()));

    object.setProperty("__proto__", QJSValue(),
                        QJSValue::PropertyGetter | QJSValue::PropertySetter);
    QVERIFY(object.property("__proto__").strictlyEquals(object.prototype()));
#endif
}

void tst_QJSValue::getSetProperty_gettersAndSettersOnGlobalObject()
{
#if 0 // FIXME: No c-style functions right now
    // global property that's a getter+setter
    QScriptEngine eng;
    eng.globalObject().setProperty("globalGetterSetterProperty", eng.newFunction(getterSetter),
                                   QJSValue::PropertyGetter | QJSValue::PropertySetter);
    eng.evaluate("globalGetterSetterProperty = 123");
    {
        QJSValue ret = eng.evaluate("globalGetterSetterProperty");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(eng.toScriptValue(123)));
    }
    QCOMPARE(eng.evaluate("typeof globalGetterSetterProperty").toString(),
             QString::fromLatin1("number"));
    {
        QJSValue ret = eng.evaluate("this.globalGetterSetterProperty()");
        QVERIFY(ret.isError());
        QCOMPARE(ret.toString(), QString::fromLatin1("TypeError: Property 'globalGetterSetterProperty' of object #<Object> is not a function"));
    }
    {
        QJSValue ret = eng.evaluate("new this.globalGetterSetterProperty()");
        QVERIFY(ret.isError());
        QCOMPARE(ret.toString(), QString::fromLatin1("TypeError: number is not a function"));
    }
#endif
}

void tst_QJSValue::getSetProperty_gettersAndSettersChange()
{
#if 0 // FIXME: No setters/getters API right now
    // "upgrading" an existing property to become a getter+setter
    QScriptEngine eng;
    QJSValue object = eng.newObject();
    QJSValue num(&eng, 123);
    object.setProperty("foo", num);
    object.setProperty("foo", eng.newFunction(getterSetter),
                        QJSValue::PropertyGetter | QJSValue::PropertySetter);
    QVERIFY(!object.property("x").isValid());
    object.setProperty("foo", num);
    QVERIFY(object.property("x").equals(num));

    eng.globalObject().setProperty("object", object);
    QJSValue res = eng.evaluate("object.x = 89; var a = object.foo; object.foo = 65; a");
    QCOMPARE(res.toInt(), 89);
    QCOMPARE(object.property("x").toInt(), 65);
    QCOMPARE(object.property("foo").toInt(), 65);
#endif
}

void tst_QJSValue::getSetProperty_array()
{
    QJSEngine eng;
    QJSValue str = eng.toScriptValue(QString::fromLatin1("bar"));
    QJSValue num = eng.toScriptValue(123.0);
    QJSValue array = eng.newArray();

    QVERIFY(array.isArray());
    array.setProperty(0, num);
    QCOMPARE(array.property(0).toNumber(), num.toNumber());
    QCOMPARE(array.property("0").toNumber(), num.toNumber());
    QCOMPARE(array.property("length").toUInt(), quint32(1));
    array.setProperty(1, str);
    QCOMPARE(array.property(1).toString(), str.toString());
    QCOMPARE(array.property("1").toString(), str.toString());
    QCOMPARE(array.property("length").toUInt(), quint32(2));
    array.setProperty("length", eng.toScriptValue(1));
    QCOMPARE(array.property("length").toUInt(), quint32(1));
    QVERIFY(array.property(1).isUndefined());
}

void tst_QJSValue::getSetProperty_gettersAndSettersStupid()
{
#if 0 // FIXME: No setters/getters API right now
    //removing unexisting Setter or Getter should not crash.
    QScriptEngine eng;
    QJSValue num = eng.toScriptValue(123.0);

    {
        QJSValue object = eng.newObject();
        object.setProperty("foo", QJSValue(), QJSValue::PropertyGetter);
        QVERIFY(!object.property("foo").isValid());
        object.setProperty("foo", num);
        QCOMPARE(object.property("foo").strictlyEquals(num), true);
    }

    {
        QJSValue object = eng.newObject();
        object.setProperty("foo", QJSValue(), QJSValue::PropertySetter);
        QVERIFY(!object.property("foo").isValid());
        object.setProperty("foo", num);
        QCOMPARE(object.property("foo").strictlyEquals(num), true);
    }

    {
        QJSValue object = eng.globalObject();
        object.setProperty("foo", QJSValue(), QJSValue::PropertySetter);
        object.setProperty("foo", QJSValue(), QJSValue::PropertyGetter);
        QVERIFY(!object.property("foo").isValid());
        object.setProperty("foo", num);
        QCOMPARE(object.property("foo").strictlyEquals(num), true);
    }
#endif
}

void tst_QJSValue::getSetProperty()
{
    QJSEngine eng;

    QJSValue object = eng.newObject();

    QJSValue str = eng.toScriptValue(QString::fromLatin1("bar"));
    object.setProperty("foo", str);
    QCOMPARE(object.property("foo").toString(), str.toString());

    QJSValue num = eng.toScriptValue(123.0);
    object.setProperty("baz", num);
    QCOMPARE(object.property("baz").toNumber(), num.toNumber());

    QJSValue strstr = QJSValue("bar");
    QCOMPARE(strstr.engine(), (QJSEngine *)0);
    object.setProperty("foo", strstr);
    QCOMPARE(object.property("foo").toString(), strstr.toString());
    QCOMPARE(strstr.engine(), &eng); // the value has been bound to the engine

    QJSValue numnum = QJSValue(123.0);
    object.setProperty("baz", numnum);
    QCOMPARE(object.property("baz").toNumber(), numnum.toNumber());

    QJSValue inv;
    inv.setProperty("foo", num);
    QCOMPARE(inv.property("foo").isUndefined(), true);

    eng.globalObject().setProperty("object", object);

#if 0 // FIXME: no setProperty API with flags
  // ReadOnly
    object.setProperty("readOnlyProperty", num, QJSValue::ReadOnly);
    QCOMPARE(object.propertyFlags("readOnlyProperty"), QJSValue::ReadOnly);
    QCOMPARE(object.property("readOnlyProperty").strictlyEquals(num), true);
    eng.evaluate("object.readOnlyProperty = !object.readOnlyProperty");
    QCOMPARE(object.property("readOnlyProperty").strictlyEquals(num), true);
    // should still be part of enumeration
    {
        QJSValue ret = eng.evaluate(
            "found = false;"
            "for (var p in object) {"
            "  if (p == 'readOnlyProperty') {"
            "    found = true; break;"
            "  }"
            "} found");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(true)), true);
    }
    // should still be deletable
    {
        QJSValue ret = eng.evaluate("delete object.readOnlyProperty");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(true)), true);
        QCOMPARE(object.property("readOnlyProperty").isValid(), false);
    }

  // Undeletable
    object.setProperty("undeletableProperty", num, QJSValue::Undeletable);
    QCOMPARE(object.propertyFlags("undeletableProperty"), QJSValue::Undeletable);
    QCOMPARE(object.property("undeletableProperty").strictlyEquals(num), true);
    {
        QJSValue ret = eng.evaluate("delete object.undeletableProperty");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(true)), false);
        QCOMPARE(object.property("undeletableProperty").strictlyEquals(num), true);
    }
    // should still be writable
    eng.evaluate("object.undeletableProperty = object.undeletableProperty + 1");
    QCOMPARE(object.property("undeletableProperty").toNumber(), num.toNumber() + 1);
    // should still be part of enumeration
    {
        QJSValue ret = eng.evaluate(
            "found = false;"
            "for (var p in object) {"
            "  if (p == 'undeletableProperty') {"
            "    found = true; break;"
            "  }"
            "} found");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(true)), true);
    }
    // should still be deletable from C++
    object.deleteProperty("undeletableProperty");
    QEXPECT_FAIL("", "QTBUG-17617: With JSC-based back-end, undeletable properties can't be deleted from C++", Continue);
    QVERIFY(!object.property("undeletableProperty").isValid());
    QEXPECT_FAIL("", "QTBUG-17617: With JSC-based back-end, undeletable properties can't be deleted from C++", Continue);
    QCOMPARE(object.propertyFlags("undeletableProperty"), 0);

  // SkipInEnumeration
    object.setProperty("dontEnumProperty", num, QJSValue::SkipInEnumeration);
    QCOMPARE(object.propertyFlags("dontEnumProperty"), QJSValue::SkipInEnumeration);
    QCOMPARE(object.property("dontEnumProperty").strictlyEquals(num), true);
    // should not be part of enumeration
    {
        QJSValue ret = eng.evaluate(
            "found = false;"
            "for (var p in object) {"
            "  if (p == 'dontEnumProperty') {"
            "    found = true; break;"
            "  }"
            "} found");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(false)), true);
    }
    // should still be writable
    eng.evaluate("object.dontEnumProperty = object.dontEnumProperty + 1");
    QCOMPARE(object.property("dontEnumProperty").toNumber(), num.toNumber() + 1);
    // should still be deletable
    {
        QJSValue ret = eng.evaluate("delete object.dontEnumProperty");
        QCOMPARE(ret.strictlyEquals(eng.toScriptValue(true)), true);
        QCOMPARE(object.property("dontEnumProperty").isValid(), false);
    }

    // change flags
    object.setProperty("flagProperty", str);
    QCOMPARE(object.propertyFlags("flagProperty"), static_cast<QJSValue::PropertyFlags>(0));

    QEXPECT_FAIL("", "FIXME: v8 does not support changing flags of existing properties", Continue);
    //v8::i::JSObject::SetProperty(LookupResult* result, ... ) does not take in account the attributes
    // if the result->isFound()
    object.setProperty("flagProperty", str, QJSValue::ReadOnly);
    QCOMPARE(object.propertyFlags("flagProperty"), QJSValue::ReadOnly);

    QEXPECT_FAIL("", "FIXME: v8 does not support changing flags of existing properties", Continue);
    object.setProperty("flagProperty", str, object.propertyFlags("flagProperty") | QJSValue::SkipInEnumeration);
    QCOMPARE(object.propertyFlags("flagProperty"), QJSValue::ReadOnly | QJSValue::SkipInEnumeration);

    QEXPECT_FAIL("", "FIXME: v8 does not support changing flags of existing properties", Continue);
    object.setProperty("flagProperty", str, QJSValue::KeepExistingFlags);
    QCOMPARE(object.propertyFlags("flagProperty"), QJSValue::ReadOnly | QJSValue::SkipInEnumeration);

    QEXPECT_FAIL("", "FIXME: v8 does not support UserRange", Continue);
    object.setProperty("flagProperty", str, QJSValue::UserRange);
    QCOMPARE(object.propertyFlags("flagProperty"), QJSValue::UserRange);

    // flags of property in the prototype
    {
        QJSValue object2 = eng.newObject();
        object2.setPrototype(object);
        QCOMPARE(object2.propertyFlags("flagProperty", QJSValue::ResolveLocal), 0);
        QEXPECT_FAIL("", "FIXME: v8 does not support UserRange", Continue);
        QCOMPARE(object2.propertyFlags("flagProperty"), QJSValue::UserRange);
    }

    // using interned strings
    QScriptString foo = eng.toStringHandle("foo");

    QVERIFY(object.deleteProperty(foo));
    QVERIFY(!object.property(foo).isValid());

    object.setProperty(foo, num);
    QVERIFY(object.property(foo).strictlyEquals(num));
    QVERIFY(object.property("foo").strictlyEquals(num));
    QVERIFY(object.propertyFlags(foo) == 0);
#endif

    // Setting index property on non-Array
    object.setProperty(13, num);
    QVERIFY(object.property(13).equals(num));
}

void tst_QJSValue::arrayElementGetterSetter()
{
#if 0 // FIXME: No c-style functions
    QScriptEngine eng;
    QJSValue obj = eng.newObject();
    obj.setProperty(1, eng.newFunction(getterSetter), QJSValue::PropertyGetter|QJSValue::PropertySetter);
    {
        QJSValue num(123);
        obj.setProperty("x", num);
        QJSValue ret = obj.property(1);
        QVERIFY(ret.isValid());
        QVERIFY(ret.equals(num));
    }
    {
        QJSValue num(456);
        obj.setProperty(1, num);
        QJSValue ret = obj.property(1);
        QVERIFY(ret.isValid());
        QVERIFY(ret.equals(num));
        QVERIFY(ret.equals(obj.property("1")));
    }
    QCOMPARE(obj.propertyFlags("1"), QJSValue::PropertyGetter|QJSValue::PropertySetter);

    obj.setProperty(1, QJSValue(), QJSValue::PropertyGetter|QJSValue::PropertySetter);
    QVERIFY(obj.propertyFlags("1") == 0);
#endif
}

void tst_QJSValue::getSetPrototype_cyclicPrototype()
{
    QJSEngine eng;
    QJSValue prototype = eng.newObject();
    QJSValue object = eng.newObject();
    object.setPrototype(prototype);

    QJSValue previousPrototype = prototype.prototype();
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setPrototype() failed: cyclic prototype value");
    prototype.setPrototype(prototype);
    QCOMPARE(prototype.prototype().strictlyEquals(previousPrototype), true);

    object.setPrototype(prototype);
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setPrototype() failed: cyclic prototype value");
    prototype.setPrototype(object);
    QCOMPARE(prototype.prototype().strictlyEquals(previousPrototype), true);

}

void tst_QJSValue::getSetPrototype_evalCyclicPrototype()
{
    QJSEngine eng;
    QJSValue ret = eng.evaluate("o = { }; p = { }; o.__proto__ = p; p.__proto__ = o");
    QCOMPARE(eng.hasUncaughtException(), true);
    QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
    QCOMPARE(ret.isError(), true);
    QCOMPARE(ret.toString(), QLatin1String("Error: Cyclic __proto__ value"));
}

void tst_QJSValue::getSetPrototype_eval()
{
    QJSEngine eng;
    QJSValue ret = eng.evaluate("p = { }; p.__proto__ = { }");
    QCOMPARE(eng.hasUncaughtException(), false);
    QCOMPARE(ret.isError(), false);
}

void tst_QJSValue::getSetPrototype_invalidPrototype()
{
    QJSEngine eng;
    QJSValue inv;
    QJSValue object = eng.newObject();
    QJSValue proto = object.prototype();
    QVERIFY(object.prototype().strictlyEquals(proto));
    inv.setPrototype(object);
    QVERIFY(inv.prototype().isUndefined());
    object.setPrototype(inv);
    QVERIFY(object.prototype().strictlyEquals(proto));
}

void tst_QJSValue::getSetPrototype_twoEngines()
{
    QJSEngine eng;
    QJSValue prototype = eng.newObject();
    QJSValue object = eng.newObject();
    object.setPrototype(prototype);
    QJSEngine otherEngine;
    QJSValue newPrototype = otherEngine.newObject();
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setPrototype() failed: cannot set a prototype created in a different engine");
    object.setPrototype(newPrototype);
    QCOMPARE(object.prototype().strictlyEquals(prototype), true);

}

void tst_QJSValue::getSetPrototype_null()
{
    QJSEngine eng;
    QJSValue object = eng.newObject();
    object.setPrototype(QJSValue(QJSValue::NullValue));
    QVERIFY(object.prototype().isNull());

    QJSValue newProto = eng.newObject();
    object.setPrototype(newProto);
    QVERIFY(object.prototype().equals(newProto));

    object.setPrototype(eng.evaluate("null"));
    QVERIFY(object.prototype().isNull());
}

void tst_QJSValue::getSetPrototype_notObjectOrNull()
{
    QJSEngine eng;
    QJSValue object = eng.newObject();
    QJSValue originalProto = object.prototype();

    // bool
    object.setPrototype(true);
    QVERIFY(object.prototype().equals(originalProto));
    object.setPrototype(eng.toScriptValue(true));
    QVERIFY(object.prototype().equals(originalProto));

    // number
    object.setPrototype(123);
    QVERIFY(object.prototype().equals(originalProto));
    object.setPrototype(eng.toScriptValue(123));
    QVERIFY(object.prototype().equals(originalProto));

    // string
    object.setPrototype("foo");
    QVERIFY(object.prototype().equals(originalProto));
    object.setPrototype(eng.toScriptValue(QString::fromLatin1("foo")));
    QVERIFY(object.prototype().equals(originalProto));

    // undefined
    object.setPrototype(QJSValue(QJSValue::UndefinedValue));
    QVERIFY(object.prototype().equals(originalProto));
    object.setPrototype(eng.evaluate("undefined"));
    QVERIFY(object.prototype().equals(originalProto));
}

void tst_QJSValue::getSetPrototype()
{
    QJSEngine eng;
    QJSValue prototype = eng.newObject();
    QJSValue object = eng.newObject();
    object.setPrototype(prototype);
    QCOMPARE(object.prototype().strictlyEquals(prototype), true);
}

void tst_QJSValue::getSetScope()
{
#if 0 // FIXME: No QJSValue::scope
    QScriptEngine eng;

    QJSValue object = eng.newObject();
    QCOMPARE(object.scope().isValid(), false);

    QJSValue object2 = eng.newObject();
    object2.setScope(object);

    QEXPECT_FAIL("", "FIXME: scope not implemented yet", Abort);
    QCOMPARE(object2.scope().strictlyEquals(object), true);

    object.setProperty("foo", 123);
    QVERIFY(!object2.property("foo").isValid());
    {
        QJSValue ret = object2.property("foo", QJSValue::ResolveScope);
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }

    QJSValue inv;
    inv.setScope(object);
    QCOMPARE(inv.scope().isValid(), false);

    QScriptEngine otherEngine;
    QJSValue object3 = otherEngine.newObject();
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::setScope() failed: cannot set a scope object created in a different engine");
    object2.setScope(object3);
    QCOMPARE(object2.scope().strictlyEquals(object), true);

    object2.setScope(QJSValue());
    QVERIFY(!object2.scope().isValid());
#endif
}

void tst_QJSValue::getSetData_objects_data()
{
#if 0 // FIXME: no setData/data API
    newEngine();

    QTest::addColumn<QJSValue>("object");

    QTest::newRow("object from evaluate") << engine->evaluate("new Object()");
    QTest::newRow("object from engine") << engine->newObject();
    QTest::newRow("Array") << engine->newArray();
    QTest::newRow("Date") << engine->evaluate("new Date(12324)");
    QTest::newRow("QObject") << engine->newQObject(this);
    QTest::newRow("RegExp") << engine->newRegExp(QRegExp());
#endif
}

void tst_QJSValue::getSetData_objects()
{
#if 0 // FIXME: no setData/data API
    QFETCH(QJSValue, object);

    QVERIFY(!object.data().isValid());
    QJSValue v1(true);
    object.setData(v1);
    QVERIFY(object.data().strictlyEquals(v1));
    QJSValue v2(123);
    object.setData(v2);
    QVERIFY(object.data().strictlyEquals(v2));
    QJSValue v3 = engine->newObject();
    object.setData(v3);
    QVERIFY(object.data().strictlyEquals(v3));
    object.setData(QJSValue());
    QVERIFY(!object.data().isValid());
#endif
}

void tst_QJSValue::getSetData_nonObjects_data()
{
#if 0 // FIXME: no setData/data API
    newEngine();

    QTest::addColumn<QJSValue>("value");

    QTest::newRow("undefined (bound)") << engine->undefinedValue();
    QTest::newRow("null (bound)") << engine->nullValue();
    QTest::newRow("string (bound)") << engine->toScriptValue("Pong");
    QTest::newRow("bool (bound)") << engine->toScriptValue(false);

    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);
    QTest::newRow("null") << QJSValue(QJSValue::NullValue);
    QTest::newRow("string") << QJSValue("Pong");
    QTest::newRow("bool") << QJSValue(true);
#endif
}

void tst_QJSValue::getSetData_nonObjects()
{
#if 0 // FIXME: no setData/data API
    QFETCH(QJSValue, value);

    QVERIFY(!value.data().isValid());
    QJSValue v1(true);
    value.setData(v1);
    QVERIFY(!value.data().isValid());
    QJSValue v2(123);
    value.setData(v2);
    QVERIFY(!value.data().isValid());
    QJSValue v3 = engine->newObject();
    value.setData(v3);
    QVERIFY(!value.data().isValid());
    value.setData(QJSValue());
    QVERIFY(!value.data().isValid());
#endif
}

void tst_QJSValue::setData_QTBUG15144()
{
#if 0 // FIXME: no setData/data API
    QScriptEngine eng;
    QJSValue obj = eng.newObject();
    for (int i = 0; i < 10000; ++i) {
        // Create an object with property 'fooN' on it, and immediately kill
        // the reference to the object so it and the property name become garbage.
        eng.evaluate(QString::fromLatin1("o = {}; o.foo%0 = 10; o = null;").arg(i));
        // Setting the data will cause a JS string to be allocated, which could
        // trigger a GC. This should not cause a crash.
        obj.setData("foodfight");
    }
#endif
}

#if 0 // FIXME: no QScriptClass
class TestScriptClass : public QScriptClass
{
public:
    TestScriptClass(QScriptEngine *engine) : QScriptClass(engine) {}
};

void tst_QJSValue::getSetScriptClass_emptyClass_data()
{
    newEngine();
    QTest::addColumn<QJSValue>("value");

    QTest::newRow("invalid") << QJSValue();
    QTest::newRow("number") << QJSValue(123);
    QTest::newRow("string") << QJSValue("pong");
    QTest::newRow("bool") << QJSValue(false);
    QTest::newRow("null") << QJSValue(QJSValue::NullValue);
    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);

    QTest::newRow("number") << engine->toScriptValue(123);
    QTest::newRow("string") << engine->toScriptValue("pong");
    QTest::newRow("bool") << engine->toScriptValue(true);
    QTest::newRow("null") << QJSValue(engine->nullValue());
    QTest::newRow("undefined") << QJSValue(engine->undefinedValue());
    QTest::newRow("object") << QJSValue(engine->newObject());
    QTest::newRow("date") << QJSValue(engine->evaluate("new Date()"));
    QTest::newRow("qobject") << QJSValue(engine->newQObject(this));
}

void tst_QJSValue::getSetScriptClass_emptyClass()
{
    QFETCH(QJSValue, value);
    QCOMPARE(value.scriptClass(), (QScriptClass*)0);
}

void tst_QJSValue::getSetScriptClass_JSObjectFromCpp()
{
    QScriptEngine eng;
    TestScriptClass testClass(&eng);
    // object created in C++ (newObject())
    {
        QJSValue obj = eng.newObject();
        obj.setScriptClass(&testClass);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)&testClass);
        obj.setScriptClass(0);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)0);
    }
}

void tst_QJSValue::getSetScriptClass_JSObjectFromJS()
{
    QScriptEngine eng;
    TestScriptClass testClass(&eng);
    // object created in JS
    {
        QJSValue obj = eng.evaluate("new Object");
        QVERIFY(!eng.hasUncaughtException());
        QVERIFY(obj.isObject());
        QCOMPARE(obj.scriptClass(), (QScriptClass*)0);
        obj.setScriptClass(&testClass);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)&testClass);
        obj.setScriptClass(0);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)0);
    }
}

void tst_QJSValue::getSetScriptClass_QVariant()
{
    QScriptEngine eng;
    TestScriptClass testClass(&eng);
    // object that already has a(n internal) class
    {
        QJSValue obj = eng.toScriptValue(QUrl("http://example.com"));
        QVERIFY(obj.isVariant());
        QCOMPARE(obj.scriptClass(), (QScriptClass*)0);
        obj.setScriptClass(&testClass);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)&testClass);
        QVERIFY(obj.isObject());
        QVERIFY(!obj.isVariant());
        QCOMPARE(obj.toVariant(), QVariant(QVariantMap()));
    }
}

void tst_QJSValue::getSetScriptClass_QObject()
{
    QScriptEngine eng;
    TestScriptClass testClass(&eng);
    {
        QJSValue obj = eng.newQObject(this);
        QVERIFY(obj.isQObject());
        obj.setScriptClass(&testClass);
        QCOMPARE(obj.scriptClass(), (QScriptClass*)&testClass);
        QVERIFY(obj.isObject());
        QVERIFY(!obj.isQObject());
        QVERIFY(obj.toQObject() == 0);
    }
}
#endif

#if 0 // FIXME: No c-style callbacks
static QJSValue getArg(QScriptContext *ctx, QScriptEngine *)
{
    return ctx->argument(0);
}

static QJSValue evaluateArg(QScriptContext *, QScriptEngine *eng)
{
    return eng->evaluate("arguments[0]");
}

static QJSValue addArgs(QScriptContext *, QScriptEngine *eng)
{
    return eng->evaluate("arguments[0] + arguments[1]");
}

static QJSValue returnInvalidValue(QScriptContext *, QScriptEngine *)
{
    return QJSValue();
}
#endif

void tst_QJSValue::call_function()
{
    QJSEngine eng;
    QJSValue fun = eng.evaluate("(function() { return 1; })");
    QVERIFY(fun.isCallable());
    QJSValue result = fun.call();
    QVERIFY(result.isNumber());
    QCOMPARE(result.toInt(), 1);
}

void tst_QJSValue::call_object()
{
    QJSEngine eng;
    QJSValue Object = eng.evaluate("Object");
    QCOMPARE(Object.isCallable(), true);
    QJSValue result = Object.callWithInstance(Object);
    QCOMPARE(result.isObject(), true);
}

void tst_QJSValue::call_newObjects()
{
    QJSEngine eng;
    // test that call() doesn't construct new objects
    QJSValue Number = eng.evaluate("Number");
    QJSValue Object = eng.evaluate("Object");
    QCOMPARE(Object.isCallable(), true);
    QJSValueList args;
    args << eng.toScriptValue(123);
    QJSValue result = Number.callWithInstance(Object, args);
    QCOMPARE(result.strictlyEquals(args.at(0)), true);
}

void tst_QJSValue::call_this()
{
    QJSEngine eng;
    // test that correct "this" object is used
    QJSValue fun = eng.evaluate("(function() { return this; })");
    QCOMPARE(fun.isCallable(), true);

    QJSValue numberObject = eng.evaluate("new Number(123)");
    QJSValue result = fun.callWithInstance(numberObject);
    QCOMPARE(result.isObject(), true);
    QCOMPARE(result.toNumber(), 123.0);
}

void tst_QJSValue::call_arguments()
{
    QJSEngine eng;
    // test that correct arguments are passed

    QJSValue fun = eng.evaluate("(function() { return arguments[0]; })");
    QCOMPARE(fun.isCallable(), true);
    {
        QJSValue result = fun.callWithInstance(eng.undefinedValue());
        QCOMPARE(result.isUndefined(), true);
    }
    {
        QJSValueList args;
        args << eng.toScriptValue(123.0);
        QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
        QCOMPARE(result.isNumber(), true);
        QCOMPARE(result.toNumber(), 123.0);
    }
    // V2 constructors
    {
        QJSValueList args;
        args << QJSValue(123.0);
        QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
        QCOMPARE(result.isNumber(), true);
        QCOMPARE(result.toNumber(), 123.0);
    }
#if 0 // FIXME: The feature of interpreting a passed array as argument list has been removed from the API
    {
        QJSValue args = eng.newArray();
        args.setProperty(0, 123);
        QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
        QVERIFY(result.isNumber());
        QCOMPARE(result.toNumber(), 123.0);
    }
#endif
}

void tst_QJSValue::call()
{
    QJSEngine eng;
    {
        QJSValue fun = eng.evaluate("(function() { return arguments[1]; })");
        QCOMPARE(fun.isCallable(), true);

        {
            QJSValueList args;
            args << eng.toScriptValue(123.0) << eng.toScriptValue(456.0);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 456.0);
        }
#if 0 // FIXME: The feature of interpreting a passed array as argument list has been removed from the API
        {
            QJSValue args = eng.newArray();
            args.setProperty(0, 123);
            args.setProperty(1, 456);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QVERIFY(result.isNumber());
            QCOMPARE(result.toNumber(), 456.0);
        }
#endif
    }
    {
        QJSValue fun = eng.evaluate("(function() { throw new Error('foo'); })");
        QCOMPARE(fun.isCallable(), true);
        QVERIFY(!eng.hasUncaughtException());

        {
            QJSValue result = fun.call();
            QCOMPARE(result.isError(), true);
            QCOMPARE(eng.hasUncaughtException(), true);
            QVERIFY(result.strictlyEquals(eng.uncaughtException()));
        }
    }
#if 0 // FIXME: No c-style callbacks
    {
        eng.clearExceptions();
        QJSValue fun = eng.newFunction(getArg);
        {
            QJSValueList args;
            args << eng.toScriptValue(123.0);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QVERIFY(!eng.hasUncaughtException());
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 123.0);
        }
        // V2 constructors
        {
            QJSValueList args;
            args << QJSValue(123.0);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 123.0);
        }
#if 0 // FIXME: The feature of interpreting a passed array as argument list has been removed from the API
        {
            QJSValue args = eng.newArray();
            args.setProperty(0, 123);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QVERIFY(result.isNumber());
            QCOMPARE(result.toNumber(), 123.0);
        }
#endif
    }
    {
        QJSValue fun = eng.newFunction(evaluateArg);
        {
            QJSValueList args;
            args << eng.toScriptValue(123.0);
            QJSValue result = fun.callWithInstance(eng.undefinedValue(), args);
            QVERIFY(!eng.hasUncaughtException());
            QCOMPARE(result.isNumber(), true);
            QCOMPARE(result.toNumber(), 123.0);
        }
    }
#endif
}

void tst_QJSValue::call_invalidArguments()
{
#if 0 // FIXME: No c-style callbacks
    // test that invalid arguments are handled gracefully
    QScriptEngine eng;
    {
        QJSValue fun = eng.newFunction(getArg);
        {
            QJSValueList args;
            args << QJSValue();
            QJSValue ret = fun.callWithInstance(args);
            QVERIFY(!eng.hasUncaughtException());
            QVERIFY(ret.isUndefined());
        }
    }
    {
        QJSValue fun = eng.newFunction(evaluateArg);
        {
            QJSValueList args;
            args << QJSValue();
            QJSValue ret = fun.call(args);
            QVERIFY(ret.isUndefined());
        }
    }
    {
        QJSValue fun = eng.newFunction(addArgs);
        {
            QJSValueList args;
            args << QJSValue() << QJSValue();
            QJSValue ret = fun.call(args);
            QVERIFY(!ret.isUndefined());
            QCOMPARE(ret.isNumber(), true);
            QCOMPARE(qIsNaN(ret.toNumber()), true);
        }
    }
#endif
}

void tst_QJSValue::call_invalidReturn()
{
#if 0 // FIXME: No c-style callbacks
    // test that invalid return value is handled gracefully
    QScriptEngine eng;
    QJSValue fun = eng.newFunction(returnInvalidValue);
    eng.globalObject().setProperty("returnInvalidValue", fun);
    QJSValue ret = eng.evaluate("returnInvalidValue() + returnInvalidValue()");
    QVERIFY(!ret.isUndefined());
    QCOMPARE(ret.isNumber(), true);
    QCOMPARE(qIsNaN(ret.toNumber()), true);
#endif
}

void tst_QJSValue::call_twoEngines()
{
    QJSEngine eng;
    QJSValue object = eng.evaluate("Object");
    QJSEngine otherEngine;
    QJSValue fun = otherEngine.evaluate("(function() { return 1; })");
    QVERIFY(fun.isCallable());
    QTest::ignoreMessage(QtWarningMsg, "JSValue can't be rassigned to an another engine.");
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::call() failed: "
                         "cannot call function with thisObject created in "
                         "a different engine");
    QVERIFY(fun.callWithInstance(object).isUndefined());
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::call() failed: "
                         "cannot call function with argument created in "
                         "a different engine");
    QVERIFY(fun.call(QJSValueList() << eng.toScriptValue(123)).isUndefined());
    {
        QJSValue fun = eng.evaluate("Object");
        QVERIFY(fun.isCallable());
        QJSEngine eng2;
        QJSValue objectInDifferentEngine = eng2.newObject();
        QJSValueList args;
        args << objectInDifferentEngine;
        QTest::ignoreMessage(QtWarningMsg, "QJSValue::call() failed: cannot call function with argument created in a different engine");
        fun.call(args);
    }
}

void tst_QJSValue::call_array()
{
#if 0 // FIXME: The feature of interpreting an array as argument list has been removed from the API
    QScriptEngine eng;
    QJSValue fun = eng.evaluate("(function() { return arguments; })");
    QVERIFY(fun.isCallable());
    QJSValue array = eng.newArray(3);
    array.setProperty(0, eng.toScriptValue(123.0));
    array.setProperty(1, eng.toScriptValue(456.0));
    array.setProperty(2, eng.toScriptValue(789.0));
    // call with single array object as arguments
    QJSValue ret = fun.call(QJSValue(), array);
    QVERIFY(!eng.hasUncaughtException());
    QCOMPARE(ret.isError(), false);
    QCOMPARE(ret.property(0).strictlyEquals(array.property(0)), true);
    QCOMPARE(ret.property(1).strictlyEquals(array.property(1)), true);
    QCOMPARE(ret.property(2).strictlyEquals(array.property(2)), true);
    // call with arguments object as arguments
    QJSValue ret2 = fun.call(QJSValue(), ret);
    QCOMPARE(ret2.isError(), false);
    QCOMPARE(ret2.property(0).strictlyEquals(ret.property(0)), true);
    QCOMPARE(ret2.property(1).strictlyEquals(ret.property(1)), true);
    QCOMPARE(ret2.property(2).strictlyEquals(ret.property(2)), true);
    // call with null as arguments
    QJSValue ret3 = fun.call(QJSValue(), eng.nullValue());
    QCOMPARE(ret3.isError(), false);
    QCOMPARE(ret3.property("length").isNumber(), true);
    QCOMPARE(ret3.property("length").toNumber(), 0.0);
    // call with undefined as arguments
    QJSValue ret4 = fun.call(QJSValue(), eng.undefinedValue());
    QCOMPARE(ret4.isError(), false);
    QCOMPARE(ret4.property("length").isNumber(), true);
    QCOMPARE(ret4.property("length").toNumber(), 0.0);
    // call with something else as arguments
    QJSValue ret5 = fun.call(QJSValue(), eng.toScriptValue(123.0));
    QCOMPARE(ret5.isError(), true);
    // call with a non-array object as arguments
    QJSValue ret6 = fun.call(QJSValue(), eng.globalObject());
    QVERIFY(ret6.isError());
    QCOMPARE(ret6.toString(), QString::fromLatin1("TypeError: Arguments must be an array"));
#endif
}


void tst_QJSValue::call_nonFunction_data()
{
    newEngine();
    QTest::addColumn<QJSValue>("value");

    QTest::newRow("invalid") << QJSValue();
    QTest::newRow("bool") << QJSValue(false);
    QTest::newRow("int") << QJSValue(123);
    QTest::newRow("string") << QJSValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);
    QTest::newRow("null") << QJSValue(QJSValue::NullValue);

    QTest::newRow("bool bound") << engine->toScriptValue(false);
    QTest::newRow("int bound") << engine->toScriptValue(123);
    QTest::newRow("string bound") << engine->toScriptValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined bound") << engine->undefinedValue();
    QTest::newRow("null bound") << engine->nullValue();
}

void tst_QJSValue::call_nonFunction()
{
    // calling things that are not functions
    QFETCH(QJSValue, value);
    QVERIFY(value.call().isUndefined());
}

#if 0 // FIXME: no c-style callbacks
static QJSValue ctorReturningUndefined(QScriptContext *ctx, QScriptEngine *)
{
    ctx->thisObject().setProperty("foo", 123);
    return QJSValue(QJSValue::UndefinedValue);
}

static QJSValue ctorReturningNewObject(QScriptContext *, QScriptEngine *eng)
{
    QJSValue result = eng->newObject();
    result.setProperty("bar", 456);
    return result;
}
#endif

void tst_QJSValue::construct_nonFunction_data()
{
    newEngine();
    QTest::addColumn<QJSValue>("value");

    QTest::newRow("invalid") << QJSValue();
    QTest::newRow("bool") << QJSValue(false);
    QTest::newRow("int") << QJSValue(123);
    QTest::newRow("string") << QJSValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);
    QTest::newRow("null") << QJSValue(QJSValue::NullValue);

    QTest::newRow("bool bound") << engine->toScriptValue(false);
    QTest::newRow("int bound") << engine->toScriptValue(123);
    QTest::newRow("string bound") << engine->toScriptValue(QString::fromLatin1("ciao"));
    QTest::newRow("undefined bound") << engine->undefinedValue();
    QTest::newRow("null bound") << engine->nullValue();
}

void tst_QJSValue::construct_nonFunction()
{
    QFETCH(QJSValue, value);
    QVERIFY(value.callAsConstructor().isUndefined());
}

void tst_QJSValue::construct_simple()
{
    QJSEngine eng;
    QJSValue fun = eng.evaluate("(function () { this.foo = 123; })");
    QVERIFY(fun.isCallable());
    QJSValue ret = fun.callAsConstructor();
    QVERIFY(!ret.isUndefined());
    QVERIFY(ret.isObject());
    QVERIFY(ret.prototype().strictlyEquals(fun.property("prototype")));
    QCOMPARE(ret.property("foo").toInt(), 123);
}

void tst_QJSValue::construct_newObjectJS()
{
    QJSEngine eng;
    // returning a different object overrides the default-constructed one
    QJSValue fun = eng.evaluate("(function () { return { bar: 456 }; })");
    QVERIFY(fun.isCallable());
    QJSValue ret = fun.callAsConstructor();
    QVERIFY(ret.isObject());
    QVERIFY(!ret.prototype().strictlyEquals(fun.property("prototype")));
    QCOMPARE(ret.property("bar").toInt(), 456);
}

#if 0 // FIXME: no c-style callbacks
void tst_QJSValue::construct_undefined()
{
    QScriptEngine eng;
    QJSValue fun = eng.newFunction(ctorReturningUndefined);
    QJSValue ret = fun.callAsConstructor();
    QVERIFY(ret.isObject());
    QVERIFY(ret.instanceOf(fun));
    QCOMPARE(ret.property("foo").toInt(), 123);
}

void tst_QJSValue::construct_newObjectCpp()
{
    QScriptEngine eng;
    QJSValue fun = eng.newFunction(ctorReturningNewObject);
    QJSValue ret = fun.callAsConstructor();
    QVERIFY(ret.isObject());
    QVERIFY(!ret.instanceOf(fun));
    QCOMPARE(ret.property("bar").toInt(), 456);
}
#endif

void tst_QJSValue::construct_arg()
{
    QJSEngine eng;
    QJSValue Number = eng.evaluate("Number");
    QCOMPARE(Number.isCallable(), true);
    QJSValueList args;
    args << eng.toScriptValue(123);
    QJSValue ret = Number.callAsConstructor(args);
    QCOMPARE(ret.isObject(), true);
    QCOMPARE(ret.toNumber(), args.at(0).toNumber());
}

void tst_QJSValue::construct_proto()
{
    QJSEngine eng;
    // test that internal prototype is set correctly
    QJSValue fun = eng.evaluate("(function() { return this.__proto__; })");
    QCOMPARE(fun.isCallable(), true);
    QCOMPARE(fun.property("prototype").isObject(), true);
    QJSValue ret = fun.callAsConstructor();
    QCOMPARE(fun.property("prototype").strictlyEquals(ret), true);
}

void tst_QJSValue::construct_returnInt()
{
    QJSEngine eng;
    // test that we return the new object even if a non-object value is returned from the function
    QJSValue fun = eng.evaluate("(function() { return 123; })");
    QCOMPARE(fun.isCallable(), true);
    QJSValue ret = fun.callAsConstructor();
    QCOMPARE(ret.isObject(), true);
}

void tst_QJSValue::construct_throw()
{
    QJSEngine eng;
    QJSValue fun = eng.evaluate("(function() { throw new Error('foo'); })");
    QCOMPARE(fun.isCallable(), true);
    QJSValue ret = fun.callAsConstructor();
    QCOMPARE(ret.isError(), true);
    QCOMPARE(eng.hasUncaughtException(), true);
    QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
}

#if 0 // FIXME: The feature of interpreting an array as argument list has been removed from the API
void tst_QJSValue::construct()
{
    QScriptEngine eng;
    QJSValue fun = eng.evaluate("(function() { return arguments; })");
    QVERIFY(fun.isCallable());
    QJSValue array = eng.newArray(3);
    array.setProperty(0, eng.toScriptValue(123.0));
    array.setProperty(1, eng.toScriptValue(456.0));
    array.setProperty(2, eng.toScriptValue(789.0));
    // construct with single array object as arguments
    QJSValue ret = fun.callAsConstructor(array);
    QVERIFY(!eng.hasUncaughtException());
    QVERIFY(ret.isObject());
    QCOMPARE(ret.property(0).strictlyEquals(array.property(0)), true);
    QCOMPARE(ret.property(1).strictlyEquals(array.property(1)), true);
    QCOMPARE(ret.property(2).strictlyEquals(array.property(2)), true);
    // construct with arguments object as arguments
    QJSValue ret2 = fun.callAsConstructor(ret);
    QCOMPARE(ret2.property(0).strictlyEquals(ret.property(0)), true);
    QCOMPARE(ret2.property(1).strictlyEquals(ret.property(1)), true);
    QCOMPARE(ret2.property(2).strictlyEquals(ret.property(2)), true);
    // construct with null as arguments
    QJSValue ret3 = fun.callAsConstructor(eng.nullValue());
    QCOMPARE(ret3.isError(), false);
    QCOMPARE(ret3.property("length").isNumber(), true);
    QCOMPARE(ret3.property("length").toNumber(), 0.0);
    // construct with undefined as arguments
    QJSValue ret4 = fun.callAsConstructor(eng.undefinedValue());
    QCOMPARE(ret4.isError(), false);
    QCOMPARE(ret4.property("length").isNumber(), true);
    QCOMPARE(ret4.property("length").toNumber(), 0.0);
    // construct with something else as arguments
    QJSValue ret5 = fun.callAsConstructor(eng.toScriptValue(123.0));
    QCOMPARE(ret5.isError(), true);
    // construct with a non-array object as arguments
    QJSValue ret6 = fun.callAsConstructor(eng.globalObject());
    QVERIFY(ret6.isError());
    QCOMPARE(ret6.toString(), QString::fromLatin1("TypeError: Arguments must be an array"));
}
#endif

void tst_QJSValue::construct_twoEngines()
{
    QJSEngine engine;
    QJSEngine otherEngine;
    QJSValue ctor = engine.evaluate("(function (a, b) { this.foo = 123; })");
    QJSValue arg = otherEngine.toScriptValue(124567);
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::callAsConstructor() failed: cannot construct function with argument created in a different engine");
    QVERIFY(ctor.callAsConstructor(QJSValueList() << arg).isUndefined());
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::callAsConstructor() failed: cannot construct function with argument created in a different engine");
    QVERIFY(ctor.callAsConstructor(QJSValueList() << arg << otherEngine.newObject()).isUndefined());
}

void tst_QJSValue::construct_constructorThrowsPrimitive()
{
    QJSEngine eng;
    QJSValue fun = eng.evaluate("(function() { throw 123; })");
    QVERIFY(fun.isCallable());
    // construct(QJSValueList)
    {
        QJSValue ret = fun.callAsConstructor();
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), 123.0);
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
        eng.clearExceptions();
    }
#if 0 // FIXME: The feature of interpreting an array as argument list has been removed from the API
    // construct(QJSValue)
    {
        QJSValue ret = fun.callAsConstructor(eng.newArray());
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), 123.0);
        QVERIFY(eng.hasUncaughtException());
        QVERIFY(ret.strictlyEquals(eng.uncaughtException()));
        eng.clearExceptions();
    }
#endif
}

#if 0 // FIXME: No QJSValue::lessThan
void tst_QJSValue::lessThan()
{
    QScriptEngine eng;

    QVERIFY(!QJSValue().lessThan(QJSValue()));

    QJSValue num = eng.toScriptValue(123);
    QCOMPARE(num.lessThan(eng.toScriptValue(124)), true);
    QCOMPARE(num.lessThan(eng.toScriptValue(122)), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(123)), false);
    QCOMPARE(num.lessThan(eng.toScriptValue("124")), true);
    QCOMPARE(num.lessThan(eng.toScriptValue("122")), false);
    QCOMPARE(num.lessThan(eng.toScriptValue("123")), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(qSNaN())), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(+qInf())), true);
    QCOMPARE(num.lessThan(eng.toScriptValue(-qInf())), false);
    QCOMPARE(num.lessThan(num), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(124).toObject()), true);
    QCOMPARE(num.lessThan(eng.toScriptValue(122).toObject()), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(123).toObject()), false);
    QCOMPARE(num.lessThan(eng.toScriptValue("124").toObject()), true);
    QCOMPARE(num.lessThan(eng.toScriptValue("122").toObject()), false);
    QCOMPARE(num.lessThan(eng.toScriptValue("123").toObject()), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(qSNaN()).toObject()), false);
    QCOMPARE(num.lessThan(eng.toScriptValue(+qInf()).toObject()), true);
    QCOMPARE(num.lessThan(eng.toScriptValue(-qInf()).toObject()), false);
    QCOMPARE(num.lessThan(num.toObject()), false);
    QCOMPARE(num.lessThan(QJSValue()), false);

    QJSValue str = eng.toScriptValue("123");
    QCOMPARE(str.lessThan(eng.toScriptValue("124")), true);
    QCOMPARE(str.lessThan(eng.toScriptValue("122")), false);
    QCOMPARE(str.lessThan(eng.toScriptValue("123")), false);
    QCOMPARE(str.lessThan(eng.toScriptValue(124)), true);
    QCOMPARE(str.lessThan(eng.toScriptValue(122)), false);
    QCOMPARE(str.lessThan(eng.toScriptValue(123)), false);
    QCOMPARE(str.lessThan(str), false);
    QCOMPARE(str.lessThan(eng.toScriptValue("124").toObject()), true);
    QCOMPARE(str.lessThan(eng.toScriptValue("122").toObject()), false);
    QCOMPARE(str.lessThan(eng.toScriptValue("123").toObject()), false);
    QCOMPARE(str.lessThan(eng.toScriptValue(124).toObject()), true);
    QCOMPARE(str.lessThan(eng.toScriptValue(122).toObject()), false);
    QCOMPARE(str.lessThan(eng.toScriptValue(123).toObject()), false);
    QCOMPARE(str.lessThan(str.toObject()), false);
    QCOMPARE(str.lessThan(QJSValue()), false);

    // V2 constructors
    QJSValue num2 = QJSValue(123);
    QCOMPARE(num2.lessThan(QJSValue(124)), true);
    QCOMPARE(num2.lessThan(QJSValue(122)), false);
    QCOMPARE(num2.lessThan(QJSValue(123)), false);
    QCOMPARE(num2.lessThan(QJSValue("124")), true);
    QCOMPARE(num2.lessThan(QJSValue("122")), false);
    QCOMPARE(num2.lessThan(QJSValue("123")), false);
    QCOMPARE(num2.lessThan(QJSValue(qSNaN())), false);
    QCOMPARE(num2.lessThan(QJSValue(+qInf())), true);
    QCOMPARE(num2.lessThan(QJSValue(-qInf())), false);
    QCOMPARE(num2.lessThan(num), false);
    QCOMPARE(num2.lessThan(QJSValue()), false);

    QJSValue str2 = QJSValue("123");
    QCOMPARE(str2.lessThan(QJSValue("124")), true);
    QCOMPARE(str2.lessThan(QJSValue("122")), false);
    QCOMPARE(str2.lessThan(QJSValue("123")), false);
    QCOMPARE(str2.lessThan(QJSValue(124)), true);
    QCOMPARE(str2.lessThan(QJSValue(122)), false);
    QCOMPARE(str2.lessThan(QJSValue(123)), false);
    QCOMPARE(str2.lessThan(str), false);
    QCOMPARE(str2.lessThan(QJSValue()), false);

    QJSValue obj1 = eng.newObject();
    QJSValue obj2 = eng.newObject();
    QCOMPARE(obj1.lessThan(obj2), false);
    QCOMPARE(obj2.lessThan(obj1), false);
    QCOMPARE(obj1.lessThan(obj1), false);
    QCOMPARE(obj2.lessThan(obj2), false);

    QJSValue date1 = eng.toScriptValue(QDateTime(QDate(2000, 1, 1)));
    QJSValue date2 = eng.toScriptValue(QDateTime(QDate(1999, 1, 1)));
    QCOMPARE(date1.lessThan(date2), false);
    QCOMPARE(date2.lessThan(date1), true);
    QCOMPARE(date1.lessThan(date1), false);
    QCOMPARE(date2.lessThan(date2), false);
    QCOMPARE(date1.lessThan(QJSValue()), false);

    QCOMPARE(QJSValue().lessThan(date2), false);

    QScriptEngine otherEngine;
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::lessThan: "
                         "cannot compare to a value created in "
                         "a different engine");
    QCOMPARE(date1.lessThan(otherEngine.toScriptValue(123)), false);
}
#endif

void tst_QJSValue::equals()
{
    QJSEngine eng;

    QVERIFY(QJSValue().equals(QJSValue()));

    QJSValue num = eng.toScriptValue(123);
    QCOMPARE(num.equals(eng.toScriptValue(123)), true);
    QCOMPARE(num.equals(eng.toScriptValue(321)), false);
    QCOMPARE(num.equals(eng.toScriptValue(QString::fromLatin1("123"))), true);
    QCOMPARE(num.equals(eng.toScriptValue(QString::fromLatin1("321"))), false);
    QCOMPARE(num.equals(eng.evaluate("new Number(123)")), true);
    QCOMPARE(num.equals(eng.evaluate("new Number(321)")), false);
    QCOMPARE(num.equals(eng.evaluate("new String('123')")), true);
    QCOMPARE(num.equals(eng.evaluate("new String('321')")), false);
    QVERIFY(eng.evaluate("new Number(123)").equals(num));
    QCOMPARE(num.equals(QJSValue()), false);

    QJSValue str = eng.toScriptValue(QString::fromLatin1("123"));
    QCOMPARE(str.equals(eng.toScriptValue(QString::fromLatin1("123"))), true);
    QCOMPARE(str.equals(eng.toScriptValue(QString::fromLatin1("321"))), false);
    QCOMPARE(str.equals(eng.toScriptValue(123)), true);
    QCOMPARE(str.equals(eng.toScriptValue(321)), false);
    QCOMPARE(str.equals(eng.evaluate("new String('123')")), true);
    QCOMPARE(str.equals(eng.evaluate("new String('321')")), false);
    QCOMPARE(str.equals(eng.evaluate("new Number(123)")), true);
    QCOMPARE(str.equals(eng.evaluate("new Number(321)")), false);
    QVERIFY(eng.evaluate("new String('123')").equals(str));
    QCOMPARE(str.equals(QJSValue()), false);

    QJSValue num2 = QJSValue(123);
    QCOMPARE(num2.equals(QJSValue(123)), true);
    QCOMPARE(num2.equals(QJSValue(321)), false);
    QCOMPARE(num2.equals(QJSValue("123")), true);
    QCOMPARE(num2.equals(QJSValue("321")), false);
    QCOMPARE(num2.equals(QJSValue()), false);

    QJSValue str2 = QJSValue("123");
    QCOMPARE(str2.equals(QJSValue("123")), true);
    QCOMPARE(str2.equals(QJSValue("321")), false);
    QCOMPARE(str2.equals(QJSValue(123)), true);
    QCOMPARE(str2.equals(QJSValue(321)), false);
    QCOMPARE(str2.equals(QJSValue()), false);

    QJSValue date1 = eng.toScriptValue(QDateTime(QDate(2000, 1, 1)));
    QJSValue date2 = eng.toScriptValue(QDateTime(QDate(1999, 1, 1)));
    QCOMPARE(date1.equals(date2), false);
    QCOMPARE(date1.equals(date1), true);
    QCOMPARE(date2.equals(date2), true);

    QJSValue undefined = eng.undefinedValue();
    QJSValue null = eng.nullValue();
    QCOMPARE(undefined.equals(undefined), true);
    QCOMPARE(null.equals(null), true);
    QCOMPARE(undefined.equals(null), true);
    QCOMPARE(null.equals(undefined), true);
    QVERIFY(undefined.equals(QJSValue()));
    QVERIFY(null.equals(QJSValue()));
    QVERIFY(!null.equals(num));
    QVERIFY(!undefined.equals(num));

    QJSValue sant = eng.toScriptValue(true);
    QVERIFY(sant.equals(eng.toScriptValue(1)));
    QVERIFY(sant.equals(eng.toScriptValue(QString::fromLatin1("1"))));
    QVERIFY(sant.equals(sant));
    QVERIFY(sant.equals(eng.evaluate("new Number(1)")));
    QVERIFY(sant.equals(eng.evaluate("new String('1')")));
    QVERIFY(sant.equals(eng.evaluate("new Boolean(true)")));
    QVERIFY(eng.evaluate("new Boolean(true)").equals(sant));
    QVERIFY(!sant.equals(eng.toScriptValue(0)));
    QVERIFY(!sant.equals(undefined));
    QVERIFY(!sant.equals(null));

    QJSValue falskt = eng.toScriptValue(false);
    QVERIFY(falskt.equals(eng.toScriptValue(0)));
    QVERIFY(falskt.equals(eng.toScriptValue(QString::fromLatin1("0"))));
    QVERIFY(falskt.equals(falskt));
    QVERIFY(falskt.equals(eng.evaluate("new Number(0)")));
    QVERIFY(falskt.equals(eng.evaluate("new String('0')")));
    QVERIFY(falskt.equals(eng.evaluate("new Boolean(false)")));
    QVERIFY(eng.evaluate("new Boolean(false)").equals(falskt));
    QVERIFY(!falskt.equals(sant));
    QVERIFY(!falskt.equals(undefined));
    QVERIFY(!falskt.equals(null));

    QJSValue obj1 = eng.newObject();
    QJSValue obj2 = eng.newObject();
    QCOMPARE(obj1.equals(obj2), false);
    QCOMPARE(obj2.equals(obj1), false);
    QCOMPARE(obj1.equals(obj1), true);
    QCOMPARE(obj2.equals(obj2), true);

    QJSValue qobj1 = eng.newQObject(this);
    QJSValue qobj2 = eng.newQObject(this);
    QJSValue qobj3 = eng.newQObject(0);

    // FIXME: No ScriptOwnership: QJSValue qobj4 = eng.newQObject(new QObject(), QScriptEngine::ScriptOwnership);
    QJSValue qobj4 = eng.newQObject(new QObject());

    QVERIFY(qobj1.equals(qobj2)); // compares the QObject pointers
    QVERIFY(!qobj2.equals(qobj4)); // compares the QObject pointers
    QVERIFY(!qobj2.equals(obj2)); // compares the QObject pointers

    QJSValue compareFun = eng.evaluate("(function(a, b) { return a == b; })");
    QVERIFY(compareFun.isCallable());
    {
        QJSValue ret = compareFun.call(QJSValueList() << qobj1 << qobj2);
        QVERIFY(ret.isBool());
        ret = compareFun.call(QJSValueList() << qobj1 << qobj3);
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
        ret = compareFun.call(QJSValueList() << qobj1 << qobj4);
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
        ret = compareFun.call(QJSValueList() << qobj1 << obj1);
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
    }

    {
        QJSValue var1 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QJSValue var2 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QEXPECT_FAIL("", "FIXME: QVariant comparison does not work with v8", Continue);
        QVERIFY(var1.equals(var2));
    }
    {
        QJSValue var1 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QJSValue var2 = eng.toScriptValue(QVariant(QPoint(3, 4)));
        QVERIFY(!var1.equals(var2));
    }

    QJSEngine otherEngine;
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::equals: "
                         "cannot compare to a value created in "
                         "a different engine");
    QCOMPARE(date1.equals(otherEngine.toScriptValue(123)), false);
}

void tst_QJSValue::strictlyEquals()
{
    QJSEngine eng;

    QVERIFY(QJSValue().strictlyEquals(QJSValue()));

    QJSValue num = eng.toScriptValue(123);
    QCOMPARE(num.strictlyEquals(eng.toScriptValue(123)), true);
    QCOMPARE(num.strictlyEquals(eng.toScriptValue(321)), false);
    QCOMPARE(num.strictlyEquals(eng.toScriptValue(QString::fromLatin1("123"))), false);
    QCOMPARE(num.strictlyEquals(eng.toScriptValue(QString::fromLatin1("321"))), false);
    QCOMPARE(num.strictlyEquals(eng.evaluate("new Number(123)")), false);
    QCOMPARE(num.strictlyEquals(eng.evaluate("new Number(321)")), false);
    QCOMPARE(num.strictlyEquals(eng.evaluate("new String('123')")), false);
    QCOMPARE(num.strictlyEquals(eng.evaluate("new String('321')")), false);
    QVERIFY(!eng.evaluate("new Number(123)").strictlyEquals(num));
    QVERIFY(!num.strictlyEquals(QJSValue()));
    QVERIFY(!QJSValue().strictlyEquals(num));

    QJSValue str = eng.toScriptValue(QString::fromLatin1("123"));
    QCOMPARE(str.strictlyEquals(eng.toScriptValue(QString::fromLatin1("123"))), true);
    QCOMPARE(str.strictlyEquals(eng.toScriptValue(QString::fromLatin1("321"))), false);
    QCOMPARE(str.strictlyEquals(eng.toScriptValue(123)), false);
    QCOMPARE(str.strictlyEquals(eng.toScriptValue(321)), false);
    QCOMPARE(str.strictlyEquals(eng.evaluate("new String('123')")), false);
    QCOMPARE(str.strictlyEquals(eng.evaluate("new String('321')")), false);
    QCOMPARE(str.strictlyEquals(eng.evaluate("new Number(123)")), false);
    QCOMPARE(str.strictlyEquals(eng.evaluate("new Number(321)")), false);
    QVERIFY(!eng.evaluate("new String('123')").strictlyEquals(str));
    QVERIFY(!str.strictlyEquals(QJSValue()));

    QJSValue num2 = QJSValue(123);
    QCOMPARE(num2.strictlyEquals(QJSValue(123)), true);
    QCOMPARE(num2.strictlyEquals(QJSValue(321)), false);
    QCOMPARE(num2.strictlyEquals(QJSValue("123")), false);
    QCOMPARE(num2.strictlyEquals(QJSValue("321")), false);
    QVERIFY(!num2.strictlyEquals(QJSValue()));

    QJSValue str2 = QJSValue("123");
    QCOMPARE(str2.strictlyEquals(QJSValue("123")), true);
    QCOMPARE(str2.strictlyEquals(QJSValue("321")), false);
    QCOMPARE(str2.strictlyEquals(QJSValue(123)), false);
    QCOMPARE(str2.strictlyEquals(QJSValue(321)), false);
    QVERIFY(!str2.strictlyEquals(QJSValue()));

    QJSValue date1 = eng.toScriptValue(QDateTime(QDate(2000, 1, 1)));
    QJSValue date2 = eng.toScriptValue(QDateTime(QDate(1999, 1, 1)));
    QCOMPARE(date1.strictlyEquals(date2), false);
    QCOMPARE(date1.strictlyEquals(date1), true);
    QCOMPARE(date2.strictlyEquals(date2), true);
    QVERIFY(!date1.strictlyEquals(QJSValue()));

    QJSValue undefined = eng.undefinedValue();
    QJSValue null = eng.nullValue();
    QCOMPARE(undefined.strictlyEquals(undefined), true);
    QCOMPARE(null.strictlyEquals(null), true);
    QCOMPARE(undefined.strictlyEquals(null), false);
    QCOMPARE(null.strictlyEquals(undefined), false);
    QVERIFY(!null.strictlyEquals(QJSValue()));

    QJSValue sant = eng.toScriptValue(true);
    QVERIFY(!sant.strictlyEquals(eng.toScriptValue(1)));
    QVERIFY(!sant.strictlyEquals(eng.toScriptValue(QString::fromLatin1("1"))));
    QVERIFY(sant.strictlyEquals(sant));
    QVERIFY(!sant.strictlyEquals(eng.evaluate("new Number(1)")));
    QVERIFY(!sant.strictlyEquals(eng.evaluate("new String('1')")));
    QVERIFY(!sant.strictlyEquals(eng.evaluate("new Boolean(true)")));
    QVERIFY(!eng.evaluate("new Boolean(true)").strictlyEquals(sant));
    QVERIFY(!sant.strictlyEquals(eng.toScriptValue(0)));
    QVERIFY(!sant.strictlyEquals(undefined));
    QVERIFY(!sant.strictlyEquals(null));
    QVERIFY(!sant.strictlyEquals(QJSValue()));

    QJSValue falskt = eng.toScriptValue(false);
    QVERIFY(!falskt.strictlyEquals(eng.toScriptValue(0)));
    QVERIFY(!falskt.strictlyEquals(eng.toScriptValue(QString::fromLatin1("0"))));
    QVERIFY(falskt.strictlyEquals(falskt));
    QVERIFY(!falskt.strictlyEquals(eng.evaluate("new Number(0)")));
    QVERIFY(!falskt.strictlyEquals(eng.evaluate("new String('0')")));
    QVERIFY(!falskt.strictlyEquals(eng.evaluate("new Boolean(false)")));
    QVERIFY(!eng.evaluate("new Boolean(false)").strictlyEquals(falskt));
    QVERIFY(!falskt.strictlyEquals(sant));
    QVERIFY(!falskt.strictlyEquals(undefined));
    QVERIFY(!falskt.strictlyEquals(null));
    QVERIFY(!falskt.strictlyEquals(QJSValue()));

    QVERIFY(!QJSValue(false).strictlyEquals(123));
    QVERIFY(!QJSValue(QJSValue::UndefinedValue).strictlyEquals(123));
    QVERIFY(!QJSValue(QJSValue::NullValue).strictlyEquals(123));
    QVERIFY(!QJSValue(false).strictlyEquals("ciao"));
    QVERIFY(!QJSValue(QJSValue::UndefinedValue).strictlyEquals("ciao"));
    QVERIFY(!QJSValue(QJSValue::NullValue).strictlyEquals("ciao"));
    QVERIFY(eng.toScriptValue(QString::fromLatin1("ciao")).strictlyEquals("ciao"));
    QVERIFY(QJSValue("ciao").strictlyEquals(eng.toScriptValue(QString::fromLatin1("ciao"))));
    QVERIFY(!QJSValue("ciao").strictlyEquals(123));
    QVERIFY(!QJSValue("ciao").strictlyEquals(eng.toScriptValue(123)));
    QVERIFY(!QJSValue(123).strictlyEquals("ciao"));
    QVERIFY(!QJSValue(123).strictlyEquals(eng.toScriptValue(QString::fromLatin1("ciao"))));
    QVERIFY(!eng.toScriptValue(123).strictlyEquals("ciao"));

    QJSValue obj1 = eng.newObject();
    QJSValue obj2 = eng.newObject();
    QCOMPARE(obj1.strictlyEquals(obj2), false);
    QCOMPARE(obj2.strictlyEquals(obj1), false);
    QCOMPARE(obj1.strictlyEquals(obj1), true);
    QCOMPARE(obj2.strictlyEquals(obj2), true);
    QVERIFY(!obj1.strictlyEquals(QJSValue()));

    QJSValue qobj1 = eng.newQObject(this);
    QJSValue qobj2 = eng.newQObject(this);
    QVERIFY(qobj1.strictlyEquals(qobj2));

    {
        QJSValue var1 = eng.toScriptValue(QVariant(QStringList() << "a"));
        QJSValue var2 = eng.toScriptValue(QVariant(QStringList() << "a"));
        QVERIFY(var1.isArray());
        QVERIFY(var2.isArray());
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSValue var1 = eng.toScriptValue(QVariant(QStringList() << "a"));
        QJSValue var2 = eng.toScriptValue(QVariant(QStringList() << "b"));
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSValue var1 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QJSValue var2 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSValue var1 = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QJSValue var2 = eng.toScriptValue(QVariant(QPoint(3, 4)));
        QVERIFY(!var1.strictlyEquals(var2));
    }

    QJSEngine otherEngine;
    QTest::ignoreMessage(QtWarningMsg, "QJSValue::strictlyEquals: "
                         "cannot compare to a value created in "
                         "a different engine");
    QCOMPARE(date1.strictlyEquals(otherEngine.toScriptValue(123)), false);
}

Q_DECLARE_METATYPE(int*)
Q_DECLARE_METATYPE(double*)
Q_DECLARE_METATYPE(QColor*)
Q_DECLARE_METATYPE(QBrush*)

void tst_QJSValue::castToPointer()
{
    QJSEngine eng;
    {
        QColor c(123, 210, 231);
        QJSValue v = eng.toScriptValue(c);
        QColor *cp = qjsvalue_cast<QColor*>(v);
        QVERIFY(cp != 0);
        QCOMPARE(*cp, c);

        QBrush *bp = qjsvalue_cast<QBrush*>(v);
        QVERIFY(bp == 0);

        QJSValue v2 = eng.toScriptValue(qVariantFromValue(cp));
        QCOMPARE(qjsvalue_cast<QColor*>(v2), cp);
    }
}

void tst_QJSValue::prettyPrinter_data()
{
    QTest::addColumn<QString>("function");
    QTest::addColumn<QString>("expected");
    QTest::newRow("function() { }") << QString("function() { }") << QString("function () { }");
    QTest::newRow("function foo() { }") << QString("(function foo() { })") << QString("function foo() { }");
    QTest::newRow("function foo(bar) { }") << QString("(function foo(bar) { })") << QString("function foo(bar) { }");
    QTest::newRow("function foo(bar, baz) { }") << QString("(function foo(bar, baz) { })") << QString("function foo(bar, baz) { }");
    QTest::newRow("this") << QString("function() { this; }") << QString("function () { this; }");
    QTest::newRow("identifier") << QString("function(a) { a; }") << QString("function (a) { a; }");
    QTest::newRow("null") << QString("function() { null; }") << QString("function () { null; }");
    QTest::newRow("true") << QString("function() { true; }") << QString("function () { true; }");
    QTest::newRow("false") << QString("function() { false; }") << QString("function () { false; }");
    QTest::newRow("string") << QString("function() { 'test'; }") << QString("function () { \'test\'; }");
    QTest::newRow("string") << QString("function() { \"test\"; }") << QString("function () { \"test\"; }");
    QTest::newRow("number") << QString("function() { 123; }") << QString("function () { 123; }");
    QTest::newRow("number") << QString("function() { 123.456; }") << QString("function () { 123.456; }");
    QTest::newRow("regexp") << QString("function() { /hello/; }") << QString("function () { /hello/; }");
    QTest::newRow("regexp") << QString("function() { /hello/gim; }") << QString("function () { /hello/gim; }");
    QTest::newRow("array") << QString("function() { []; }") << QString("function () { []; }");
    QTest::newRow("array") << QString("function() { [10]; }") << QString("function () { [10]; }");
    QTest::newRow("array") << QString("function() { [10, 20, 30]; }") << QString("function () { [10, 20, 30]; }");
    QTest::newRow("array") << QString("function() { [10, 20, , 40]; }") << QString("function () { [10, 20, , 40]; }");
    QTest::newRow("array") << QString("function() { [,]; }") << QString("function () { [,]; }");
    QTest::newRow("array") << QString("function() { [, 10]; }") << QString("function () { [, 10]; }");
    QTest::newRow("array") << QString("function() { [, 10, ]; }") << QString("function () { [, 10, ]; }");
    QTest::newRow("array") << QString("function() { [, 10, ,]; }") << QString("function () { [, 10, ,]; }");
    QTest::newRow("array") << QString("function() { [[10], [20]]; }") << QString("function () { [[10], [20]]; }");
    QTest::newRow("member") << QString("function() { a.b; }") << QString("function () { a.b; }");
    QTest::newRow("member") << QString("function() { a.b.c; }") << QString("function () { a.b.c; }");
    QTest::newRow("call") << QString("function() { f(); }") << QString("function () { f(); }");
    QTest::newRow("call") << QString("function() { f(a); }") << QString("function () { f(a); }");
    QTest::newRow("call") << QString("function() { f(a, b); }") << QString("function () { f(a, b); }");
    QTest::newRow("new") << QString("function() { new C(); }") << QString("function () { new C(); }");
    QTest::newRow("new") << QString("function() { new C(a); }") << QString("function () { new C(a); }");
    QTest::newRow("new") << QString("function() { new C(a, b); }") << QString("function () { new C(a, b); }");
    QTest::newRow("++") << QString("function() { a++; }") << QString("function () { a++; }");
    QTest::newRow("++") << QString("function() { ++a; }") << QString("function () { ++a; }");
    QTest::newRow("--") << QString("function() { a--; }") << QString("function () { a--; }");
    QTest::newRow("--") << QString("function() { --a; }") << QString("function () { --a; }");
    QTest::newRow("delete") << QString("function() { delete a; }") << QString("function () { delete a; }");
    QTest::newRow("void") << QString("function() { void a; }") << QString("function () { void a; }");
    QTest::newRow("typeof") << QString("function() { typeof a; }") << QString("function () { typeof a; }");
    QTest::newRow("+") << QString("function() { +a; }") << QString("function () { +a; }");
    QTest::newRow("-") << QString("function() { -a; }") << QString("function () { -a; }");
    QTest::newRow("~") << QString("function() { ~a; }") << QString("function () { ~a; }");
    QTest::newRow("!") << QString("function() { !a; }") << QString("function () { !a; }");
    QTest::newRow("+") << QString("function() { a + b; }") << QString("function () { a + b; }");
    QTest::newRow("&&") << QString("function() { a && b; }") << QString("function () { a && b; }");
    QTest::newRow("&=") << QString("function() { a &= b; }") << QString("function () { a &= b; }");
    QTest::newRow("=") << QString("function() { a = b; }") << QString("function () { a = b; }");
    QTest::newRow("&") << QString("function() { a & b; }") << QString("function () { a & b; }");
    QTest::newRow("|") << QString("function() { a | b; }") << QString("function () { a | b; }");
    QTest::newRow("^") << QString("function() { a ^ b; }") << QString("function () { a ^ b; }");
    QTest::newRow("-=") << QString("function() { a -= b; }") << QString("function () { a -= b; }");
    QTest::newRow("/") << QString("function() { a / b; }") << QString("function () { a / b; }");
    QTest::newRow("/=") << QString("function() { a /= b; }") << QString("function () { a /= b; }");
    QTest::newRow("==") << QString("function() { a == b; }") << QString("function () { a == b; }");
    QTest::newRow(">=") << QString("function() { a >= b; }") << QString("function () { a >= b; }");
    QTest::newRow(">") << QString("function() { a > b; }") << QString("function () { a > b; }");
    QTest::newRow("in") << QString("function() { a in b; }") << QString("function () { a in b; }");
    QTest::newRow("+=") << QString("function() { a += b; }") << QString("function () { a += b; }");
    QTest::newRow("instanceof") << QString("function() { a instanceof b; }") << QString("function () { a instanceof b; }");
    QTest::newRow("<=") << QString("function() { a <= b; }") << QString("function () { a <= b; }");
    QTest::newRow("<<") << QString("function() { a << b; }") << QString("function () { a << b; }");
    QTest::newRow("<<=") << QString("function() { a <<= b; }") << QString("function () { a <<= b; }");
    QTest::newRow("<") << QString("function() { a < b; }") << QString("function () { a < b; }");
    QTest::newRow("%") << QString("function() { a % b; }") << QString("function () { a % b; }");
    QTest::newRow("%=") << QString("function() { a %= b; }") << QString("function () { a %= b; }");
    QTest::newRow("*") << QString("function() { a * b; }") << QString("function () { a * b; }");
    QTest::newRow("*=") << QString("function() { a *= b; }") << QString("function () { a *= b; }");
    QTest::newRow("!=") << QString("function() { a != b; }") << QString("function () { a != b; }");
    QTest::newRow("||") << QString("function() { a || b; }") << QString("function () { a || b; }");
    QTest::newRow("|=") << QString("function() { a |= b; }") << QString("function () { a |= b; }");
    QTest::newRow(">>") << QString("function() { a >> b; }") << QString("function () { a >> b; }");
    QTest::newRow(">>=") << QString("function() { a >>= b; }") << QString("function () { a >>= b; }");
    QTest::newRow("===") << QString("function() { a === b; }") << QString("function () { a === b; }");
    QTest::newRow("!==") << QString("function() { a !== b; }") << QString("function () { a !== b; }");
    QTest::newRow("-") << QString("function() { a - b; }") << QString("function () { a - b; }");
    QTest::newRow(">>>") << QString("function() { a >>> b; }") << QString("function () { a >>> b; }");
    QTest::newRow(">>>=") << QString("function() { a >>>= b; }") << QString("function () { a >>>= b; }");
    QTest::newRow("^=") << QString("function() { a ^= b; }") << QString("function () { a ^= b; }");
    QTest::newRow("? :") << QString("function() { a ? b : c; }") << QString("function () { a ? b : c; }");
    QTest::newRow("a; b; c") << QString("function() { a; b; c; }") << QString("function () { a; b; c; }");
    QTest::newRow("var a;") << QString("function() { var a; }") << QString("function () { var a; }");
    QTest::newRow("var a, b;") << QString("function() { var a, b; }") << QString("function () { var a, b; }");
    QTest::newRow("var a = 10;") << QString("function() { var a = 10; }") << QString("function () { var a = 10; }");
    QTest::newRow("var a, b = 20;") << QString("function() { var a, b = 20; }") << QString("function () { var a, b = 20; }");
    QTest::newRow("var a = 10, b = 20;") << QString("function() { var a = 10, b = 20; }") << QString("function () { var a = 10, b = 20; }");
    QTest::newRow("if") << QString("function() { if (a) b; }") << QString("function () { if (a) b; }");
    QTest::newRow("if") << QString("function() { if (a) { b; c; } }") << QString("function () { if (a) { b; c; } }");
    QTest::newRow("if-else") << QString("function() { if (a) b; else c; }") << QString("function () { if (a) b; else c; }");
    QTest::newRow("if-else") << QString("function() { if (a) { b; c; } else { d; e; } }") << QString("function () { if (a) { b; c; } else { d; e; } }");
    QTest::newRow("do-while") << QString("function() { do { a; } while (b); }") << QString("function () { do { a; } while (b); }");
    QTest::newRow("do-while") << QString("function() { do { a; b; c; } while (d); }") << QString("function () { do { a; b; c; } while (d); }");
    QTest::newRow("while") << QString("function() { while (a) { b; } }") << QString("function () { while (a) { b; } }");
    QTest::newRow("while") << QString("function() { while (a) { b; c; } }") << QString("function () { while (a) { b; c; } }");
    QTest::newRow("for") << QString("function() { for (a; b; c) { } }") << QString("function () { for (a; b; c) { } }");
    QTest::newRow("for") << QString("function() { for (; a; b) { } }") << QString("function () { for (; a; b) { } }");
    QTest::newRow("for") << QString("function() { for (; ; a) { } }") << QString("function () { for (; ; a) { } }");
    QTest::newRow("for") << QString("function() { for (; ; ) { } }") << QString("function () { for (; ; ) { } }");
    QTest::newRow("for") << QString("function() { for (var a; b; c) { } }") << QString("function () { for (var a; b; c) { } }");
    QTest::newRow("for") << QString("function() { for (var a, b, c; d; e) { } }") << QString("function () { for (var a, b, c; d; e) { } }");
    QTest::newRow("continue") << QString("function() { for (; ; ) { continue; } }") << QString("function () { for (; ; ) { continue; } }");
    QTest::newRow("break") << QString("function() { for (; ; ) { break; } }") << QString("function () { for (; ; ) { break; } }");
    QTest::newRow("return") << QString("function() { return; }") << QString("function () { return; }");
    QTest::newRow("return") << QString("function() { return 10; }") << QString("function () { return 10; }");
    QTest::newRow("with") << QString("function() { with (a) { b; } }") << QString("function () { with (a) { b; } }");
    QTest::newRow("with") << QString("function() { with (a) { b; c; } }") << QString("function () { with (a) { b; c; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { } }") << QString("function () { switch (a) { } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: ; } }") << QString("function () { switch (a) { case 1: ; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: b; break; } }") << QString("function () { switch (a) { case 1: b; break; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: b; break; case 2: break; } }") << QString("function () { switch (a) { case 1: b; break; case 2: break; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: case 2: ; } }") << QString("function () { switch (a) { case 1: case 2: ; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: default: ; } }") << QString("function () { switch (a) { case 1: default: ; } }");
    QTest::newRow("switch") << QString("function() { switch (a) { case 1: default: ; case 3: ; } }") << QString("function () { switch (a) { case 1: default: ; case 3: ; } }");
    QTest::newRow("label") << QString("function() { a: b; }") << QString("function () { a: b; }");
    QTest::newRow("throw") << QString("function() { throw a; }") << QString("function () { throw a; }");
    QTest::newRow("try-catch") << QString("function() { try { a; } catch (e) { b; } }") << QString("function () { try { a; } catch (e) { b; } }");
    QTest::newRow("try-finally") << QString("function() { try { a; } finally { b; } }") << QString("function () { try { a; } finally { b; } }");
    QTest::newRow("try-catch-finally") << QString("function() { try { a; } catch (e) { b; } finally { c; } }") << QString("function () { try { a; } catch (e) { b; } finally { c; } }");
    QTest::newRow("a + b + c + d") << QString("function() { a + b + c + d; }") << QString("function () { a + b + c + d; }");
    QTest::newRow("a + b - c") << QString("function() { a + b - c; }") << QString("function () { a + b - c; }");
    QTest::newRow("a + -b") << QString("function() { a + -b; }") << QString("function () { a + -b; }");
    QTest::newRow("a + ~b") << QString("function() { a + ~b; }") << QString("function () { a + ~b; }");
    QTest::newRow("a + !b") << QString("function() { a + !b; }") << QString("function () { a + !b; }");
    QTest::newRow("a + +b") << QString("function() { a + +b; }") << QString("function () { a + +b; }");
    QTest::newRow("(a + b) - c") << QString("function() { (a + b) - c; }") << QString("function () { (a + b) - c; }");
    QTest::newRow("(a - b + c") << QString("function() { a - b + c; }") << QString("function () { a - b + c; }");
    QTest::newRow("(a - (b + c)") << QString("function() { a - (b + c); }") << QString("function () { a - (b + c); }");
    QTest::newRow("a + -(b + c)") << QString("function() { a + -(b + c); }") << QString("function () { a + -(b + c); }");
    QTest::newRow("a + ~(b + c)") << QString("function() { a + ~(b + c); }") << QString("function () { a + ~(b + c); }");
    QTest::newRow("a + !(b + c)") << QString("function() { a + !(b + c); }") << QString("function () { a + !(b + c); }");
    QTest::newRow("a + +(b + c)") << QString("function() { a + +(b + c); }") << QString("function () { a + +(b + c); }");
    QTest::newRow("a + b * c") << QString("function() { a + b * c; }") << QString("function () { a + b * c; }");
    QTest::newRow("(a + b) * c") << QString("function() { (a + b) * c; }") << QString("function () { (a + b) * c; }");
    QTest::newRow("(a + b) * (c + d)") << QString("function() { (a + b) * (c + d); }") << QString("function () { (a + b) * (c + d); }");
    QTest::newRow("a + (b * c)") << QString("function() { a + (b * c); }") << QString("function () { a + (b * c); }");
    QTest::newRow("a + (b / c)") << QString("function() { a + (b / c); }") << QString("function () { a + (b / c); }");
    QTest::newRow("(a / b) * c") << QString("function() { (a / b) * c; }") << QString("function () { (a / b) * c; }");
    QTest::newRow("a / (b * c)") << QString("function() { a / (b * c); }") << QString("function () { a / (b * c); }");
    QTest::newRow("a / (b % c)") << QString("function() { a / (b % c); }") << QString("function () { a / (b % c); }");
    QTest::newRow("a && b || c") << QString("function() { a && b || c; }") << QString("function () { a && b || c; }");
    QTest::newRow("a && (b || c)") << QString("function() { a && (b || c); }") << QString("function () { a && (b || c); }");
    QTest::newRow("a & b | c") << QString("function() { a & b | c; }") << QString("function () { a & b | c; }");
    QTest::newRow("a & (b | c)") << QString("function() { a & (b | c); }") << QString("function () { a & (b | c); }");
    QTest::newRow("a & b | c ^ d") << QString("function() { a & b | c ^ d; }") << QString("function () { a & b | c ^ d; }");
    QTest::newRow("a & (b | c ^ d)") << QString("function() { a & (b | c ^ d); }") << QString("function () { a & (b | c ^ d); }");
    QTest::newRow("(a & b | c) ^ d") << QString("function() { (a & b | c) ^ d; }") << QString("function () { (a & b | c) ^ d; }");
    QTest::newRow("a << b + c") << QString("function() { a << b + c; }") << QString("function () { a << b + c; }");
    QTest::newRow("(a << b) + c") << QString("function() { (a << b) + c; }") << QString("function () { (a << b) + c; }");
    QTest::newRow("a >> b + c") << QString("function() { a >> b + c; }") << QString("function () { a >> b + c; }");
    QTest::newRow("(a >> b) + c") << QString("function() { (a >> b) + c; }") << QString("function () { (a >> b) + c; }");
    QTest::newRow("a >>> b + c") << QString("function() { a >>> b + c; }") << QString("function () { a >>> b + c; }");
    QTest::newRow("(a >>> b) + c") << QString("function() { (a >>> b) + c; }") << QString("function () { (a >>> b) + c; }");
    QTest::newRow("a == b || c != d") << QString("function() { a == b || c != d; }") << QString("function () { a == b || c != d; }");
    QTest::newRow("a == (b || c != d)") << QString("function() { a == (b || c != d); }") << QString("function () { a == (b || c != d); }");
    QTest::newRow("a === b || c !== d") << QString("function() { a === b || c !== d; }") << QString("function () { a === b || c !== d; }");
    QTest::newRow("a === (b || c !== d)") << QString("function() { a === (b || c !== d); }") << QString("function () { a === (b || c !== d); }");
    QTest::newRow("a &= b + c") << QString("function() { a &= b + c; }") << QString("function () { a &= b + c; }");
    QTest::newRow("debugger") << QString("function() { debugger; }") << QString("function () { debugger; }");
}

void tst_QJSValue::prettyPrinter()
{
    QFETCH(QString, function);
    QFETCH(QString, expected);
    QJSEngine eng;
    QJSValue val = eng.evaluate("(" + function + ")");
    QVERIFY(val.isCallable());
    QString actual = val.toString();
    int count = qMin(actual.size(), expected.size());
//    qDebug() << actual << expected;
    for (int i = 0; i < count; ++i) {
//        qDebug() << i << actual.at(i) << expected.at(i);
        QCOMPARE(actual.at(i), expected.at(i));
    }
    QCOMPARE(actual.size(), expected.size());
}

void tst_QJSValue::engineDeleted()
{
    QJSEngine *eng = new QJSEngine;
    QJSValue v1 = eng->toScriptValue(123);
    QVERIFY(v1.isNumber());
    QJSValue v2 = eng->toScriptValue(QString("ciao"));
    QVERIFY(v2.isString());
    QJSValue v3 = eng->newObject();
    QVERIFY(v3.isObject());
    QJSValue v4 = eng->newQObject(this);
    QVERIFY(v4.isQObject());
    QJSValue v5 = "Hello";
    QVERIFY(v2.isString());

    delete eng;

    QVERIFY(v1.isUndefined());
    QVERIFY(v1.engine() == 0);
    QVERIFY(v2.isUndefined());
    QVERIFY(v2.engine() == 0);
    QVERIFY(v3.isUndefined());
    QVERIFY(v3.engine() == 0);
    QVERIFY(v4.isUndefined());
    QVERIFY(v4.engine() == 0);
    QVERIFY(v5.isString()); // was not bound to engine
    QVERIFY(v5.engine() == 0);

    QVERIFY(v3.property("foo").isUndefined());
}

void tst_QJSValue::valueOfWithClosure()
{
    QJSEngine eng;
    // valueOf()
    {
        QJSValue obj = eng.evaluate("o = {}; (function(foo) { o.valueOf = function() { return foo; } })(123); o");
        QVERIFY(obj.isObject());
        QCOMPARE(obj.toInt(), 123);
    }
    // toString()
    {
        QJSValue obj = eng.evaluate("o = {}; (function(foo) { o.toString = function() { return foo; } })('ciao'); o");
        QVERIFY(obj.isObject());
        QCOMPARE(obj.toString(), QString::fromLatin1("ciao"));
    }
}

#if 0 // FIXME: no objectId()
void tst_QJSValue::objectId()
{
    QCOMPARE(QJSValue().objectId(), (qint64)-1);
    QCOMPARE(QJSValue(QJSValue::UndefinedValue).objectId(), (qint64)-1);
    QCOMPARE(QJSValue(QJSValue::NullValue).objectId(), (qint64)-1);
    QCOMPARE(QJSValue(false).objectId(), (qint64)-1);
    QCOMPARE(QJSValue(123).objectId(), (qint64)-1);
    QCOMPARE(QJSValue(uint(123)).objectId(), (qint64)-1);
    QCOMPARE(QJSValue(123.5).objectId(), (qint64)-1);
    QCOMPARE(QJSValue("ciao").objectId(), (qint64)-1);

    QScriptEngine eng;
    QJSValue o1 = eng.newObject();
    QVERIFY(o1.objectId() != -1);
    QJSValue o2 = eng.newObject();
    QVERIFY(o2.objectId() != -1);
    QVERIFY(o1.objectId() != o2.objectId());

    QVERIFY(eng.objectById(o1.objectId()).strictlyEquals(o1));
    QVERIFY(eng.objectById(o2.objectId()).strictlyEquals(o2));

    qint64 globalObjectId = -1;
    {
        QJSValue global = eng.globalObject();
        globalObjectId = global.objectId();
        QVERIFY(globalObjectId != -1);
        QVERIFY(eng.objectById(globalObjectId).strictlyEquals(global));
    }
    QJSValue obj = eng.objectById(globalObjectId);
    QVERIFY(obj.isObject());
    QVERIFY(obj.strictlyEquals(eng.globalObject()));
}
#endif

void tst_QJSValue::nestedObjectToVariant_data()
{
    QTest::addColumn<QString>("program");
    QTest::addColumn<QVariant>("expected");

    // Array literals
    QTest::newRow("[[]]")
        << QString::fromLatin1("[[]]")
        << QVariant(QVariantList() << (QVariant(QVariantList())));
    QTest::newRow("[[123]]")
        << QString::fromLatin1("[[123]]")
        << QVariant(QVariantList() << (QVariant(QVariantList() << 123)));
    QTest::newRow("[[], 123]")
        << QString::fromLatin1("[[], 123]")
        << QVariant(QVariantList() << QVariant(QVariantList()) << 123);

    // Cyclic arrays
    QTest::newRow("var a=[]; a.push(a)")
        << QString::fromLatin1("var a=[]; a.push(a); a")
        << QVariant(QVariantList() << QVariant(QVariantList()));
    QTest::newRow("var a=[]; a.push(123, a)")
        << QString::fromLatin1("var a=[]; a.push(123, a); a")
        << QVariant(QVariantList() << 123 << QVariant(QVariantList()));
    QTest::newRow("var a=[]; var b=[]; a.push(b); b.push(a)")
        << QString::fromLatin1("var a=[]; var b=[]; a.push(b); b.push(a); a")
        << QVariant(QVariantList() << QVariant(QVariantList() << QVariant(QVariantList())));
    QTest::newRow("var a=[]; var b=[]; a.push(123, b); b.push(456, a)")
        << QString::fromLatin1("var a=[]; var b=[]; a.push(123, b); b.push(456, a); a")
        << QVariant(QVariantList() << 123 << QVariant(QVariantList() << 456 << QVariant(QVariantList())));

    // Object literals
    {
        QVariantMap m;
        QTest::newRow("{}")
            << QString::fromLatin1("({})")
            << QVariant(m);
    }
    {
        QVariantMap m;
        m["a"] = QVariantMap();
        QTest::newRow("{ a:{} }")
            << QString::fromLatin1("({ a:{} })")
            << QVariant(m);
    }
    {
        QVariantMap m, m2;
        m2["b"] = 10;
        m2["c"] = 20;
        m["a"] = m2;
        QTest::newRow("{ a:{b:10, c:20} }")
            << QString::fromLatin1("({ a:{b:10, c:20} })")
            << QVariant(m);
    }
    {
        QVariantMap m;
        m["a"] = 10;
        m["b"] = QVariantList() << 20 << 30;
        QTest::newRow("{ a:10, b:[20, 30]}")
            << QString::fromLatin1("({ a:10, b:[20,30]})")
            << QVariant(m);
    }

    // Cyclic objects
    {
        QVariantMap m;
        m["p"] = QVariantMap();
        QTest::newRow("var o={}; o.p=o")
            << QString::fromLatin1("var o={}; o.p=o; o")
            << QVariant(m);
    }
    {
        QVariantMap m;
        m["p"] = 123;
        m["q"] = QVariantMap();
        QTest::newRow("var o={}; o.p=123; o.q=o")
            << QString::fromLatin1("var o={}; o.p=123; o.q=o; o")
            << QVariant(m);
    }
}

void tst_QJSValue::nestedObjectToVariant()
{
    QJSEngine eng;
    QFETCH(QString, program);
    QFETCH(QVariant, expected);
    QJSValue o = eng.evaluate(program);
    QVERIFY(!o.isError());
    QVERIFY(o.isObject());
    QCOMPARE(o.toVariant(), expected);
}

QTEST_MAIN(tst_QJSValue)
