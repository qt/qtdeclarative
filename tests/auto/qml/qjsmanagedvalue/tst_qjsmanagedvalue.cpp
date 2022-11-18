// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qjsmanagedvalue.h"

#include <QtQml/private/qv4engine_p.h>
#include <QtQml/qjsmanagedvalue.h>

#include <QtCore/qthread.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpoint.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qsequentialiterable.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qthread.h>

#include <QtTest/qtest.h>

#include <memory>

void tst_QJSManagedValue::ctor_invalid()
{
    QJSManagedValue v;
    QCOMPARE(v.type(), QJSManagedValue::Undefined);
}

void tst_QJSManagedValue::ctor_undefinedWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(QVariant()));
    QCOMPARE(v.type(), QJSManagedValue::Undefined);
}

void tst_QJSManagedValue::ctor_nullWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.evaluate(QStringLiteral("null")), &eng);
    QCOMPARE(v.type(), QJSManagedValue::Object);
    QVERIFY(v.isNull());
}

void tst_QJSManagedValue::ctor_boolWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(false));
    QCOMPARE(v.type(), QJSManagedValue::Boolean);
    QCOMPARE(v.toBoolean(), false);
}

void tst_QJSManagedValue::ctor_intWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(int(1)));
    QCOMPARE(v.type(), QJSManagedValue::Number);
    QCOMPARE(v.toNumber(), 1.0);
}

void tst_QJSManagedValue::ctor_uintWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(uint(0x43211234)));
    QCOMPARE(v.type(), QJSManagedValue::Number);
    QCOMPARE(v.toNumber(), 0x43211234);
}

void tst_QJSManagedValue::ctor_floatWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(12345678910.5));
    QCOMPARE(v.type(), QJSManagedValue::Number);
    QCOMPARE(v.toNumber(), 12345678910.5);
}


void tst_QJSManagedValue::ctor_stringWithEngine()
{
    QJSEngine eng;
    QJSManagedValue v(eng.toManagedValue(QStringLiteral("ciao")));
    QCOMPARE(v.type(), QJSManagedValue::String);
    QCOMPARE(v.toString(), QStringLiteral("ciao"));
}

void tst_QJSManagedValue::ctor_copyAndAssignWithEngine()
{
    QJSEngine eng;
    // copy constructor, operator=

    QJSManagedValue v(eng.toManagedValue(1.0));
    QJSManagedValue v2(v.toJSValue(), &eng);
    QCOMPARE(v2.strictlyEquals(v), true);

    QJSManagedValue v3(v.toJSValue(), &eng);
    QCOMPARE(v3.strictlyEquals(v), true);
    QCOMPARE(v3.strictlyEquals(v2), true);

    QJSManagedValue v4(eng.toManagedValue(2.0));
    QCOMPARE(v4.strictlyEquals(v), false);
    v3 = QJSManagedValue(v4.toJSValue(), &eng);
    QCOMPARE(v3.strictlyEquals(v), false);
    QCOMPARE(v3.strictlyEquals(v4), true);

    v2 = QJSManagedValue(QJSPrimitiveValue(), &eng);
    QCOMPARE(v2.strictlyEquals(v), false);
    QCOMPARE(v.toNumber(), 1.0);

    QJSManagedValue v5(v.toJSValue(), &eng);
    QCOMPARE(v5.strictlyEquals(v), true);
    v = QJSManagedValue(QJSPrimitiveValue(), &eng);
    QCOMPARE(v5.strictlyEquals(v), false);
    QCOMPARE(v5.toNumber(), 1.0);
}

void tst_QJSManagedValue::toString()
{
    QJSEngine eng;

    {
        QJSManagedValue undefined(eng.toManagedValue(QVariant()));
        QCOMPARE(undefined.toString(), QStringLiteral("undefined"));
        QCOMPARE(qjsvalue_cast<QString>(undefined), QStringLiteral("undefined"));
    }

    {
        QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
        QCOMPARE(null.toString(), QStringLiteral("null"));
        QCOMPARE(qjsvalue_cast<QString>(null), QStringLiteral("null"));
    }

    {
        QJSManagedValue falskt(eng.toManagedValue(false));
        QCOMPARE(falskt.toString(), QStringLiteral("false"));
        QCOMPARE(qjsvalue_cast<QString>(falskt), QStringLiteral("false"));

        QJSManagedValue sant(eng.toManagedValue(true));
        QCOMPARE(sant.toString(), QStringLiteral("true"));
        QCOMPARE(qjsvalue_cast<QString>(sant), QStringLiteral("true"));
    }
    {
        QJSManagedValue number(eng.toManagedValue(123));
        QCOMPARE(number.toString(), QStringLiteral("123"));
        QCOMPARE(qjsvalue_cast<QString>(number), QStringLiteral("123"));
    }
    {
        QJSManagedValue number(eng.toManagedValue(6.37e-8));
        QCOMPARE(number.toString(), QStringLiteral("6.37e-8"));
    }
    {
        QJSManagedValue number(eng.toManagedValue(-6.37e-8));
        QCOMPARE(number.toString(), QStringLiteral("-6.37e-8"));

        QJSManagedValue str(eng.toManagedValue(QStringLiteral("ciao")));
        QCOMPARE(str.toString(), QStringLiteral("ciao"));
        QCOMPARE(qjsvalue_cast<QString>(str), QStringLiteral("ciao"));
    }

    {
        QJSManagedValue object(eng.newObject(), &eng);
        QCOMPARE(object.toString(), QStringLiteral("[object Object]"));
        QCOMPARE(qjsvalue_cast<QString>(object), QStringLiteral("[object Object]"));
    }

    // toString() that throws exception
    {
        QVERIFY(!eng.hasError());
        QJSManagedValue objectObject(eng.evaluate(QStringLiteral(
            "(function(){"
            "  o = { };"
            "  o.toString = function() { throw new Error('toString'); };"
            "  return o;"
            "})()")), &eng);
        QCOMPARE(objectObject.type(), QJSManagedValue::Object);
        QVERIFY(!objectObject.isNull());
        QVERIFY(!eng.hasError());

        QCOMPARE(objectObject.toString(), QStringLiteral("undefined"));

        QVERIFY(eng.hasError());
        QJSManagedValue errorObject(eng.catchError(), &eng);
        QCOMPARE(errorObject.type(), QJSManagedValue::Object);
        QVERIFY(!errorObject.isNull());
        QCOMPARE(errorObject.toString(), QStringLiteral("Error: toString"));
        QVERIFY(!eng.hasError());
    }
    {
        QVERIFY(!eng.hasError());
        QJSManagedValue objectObject(eng.evaluate(QStringLiteral(
            "(function(){"
            "  var f = function() {};"
            "  f.prototype = Date;"
            "  return new f;"
            "})()")), &eng);
        QCOMPARE(objectObject.type(), QJSManagedValue::Object);
        QVERIFY(!objectObject.isNull());
        QVERIFY(!eng.hasError());
        QCOMPARE(objectObject.toString(), QStringLiteral("undefined"));
        QVERIFY(eng.hasError());
        QJSManagedValue errorObject(eng.catchError(), &eng);
        QCOMPARE(errorObject.type(), QJSManagedValue::Object);
        QVERIFY(!errorObject.isNull());
        QCOMPARE(errorObject.toString(), QStringLiteral("TypeError: Type error"));
        QVERIFY(!eng.hasError());
    }

    QJSManagedValue inv(QJSPrimitiveValue(), &eng);
    QCOMPARE(inv.toString(), QStringLiteral("undefined"));

    // variant should use internal valueOf(), then fall back to QVariant::toString(),
    // then fall back to "QVariant(typename)"
    QJSManagedValue variant(eng.toManagedValue(QPoint(10, 20)));
    QCOMPARE(variant.type(), QJSManagedValue::Object);
    QCOMPARE(variant.toString(), QStringLiteral("QPoint(10, 20)"));
    variant = eng.toManagedValue(QUrl());
    QCOMPARE(variant.type(), QJSManagedValue::Object);
    QVERIFY(variant.toString().isEmpty());

    {
        QJSManagedValue o(eng.newObject(), &eng);
        o.setProperty(QStringLiteral("test"), QJSValue(42));
        QCOMPARE(o.toString(), QStringLiteral("[object Object]"));
    }

    {
        QJSManagedValue o(eng.newArray(), &eng);
        o.setProperty(0, 1);
        o.setProperty(1, 2);
        o.setProperty(2, 3);
        QCOMPARE(o.toString(), QStringLiteral("1,2,3"));
    }

    {
        QByteArray hello = QByteArrayLiteral("Hello World");
        QJSManagedValue jsValue(eng.toManagedValue(hello));
        QCOMPARE(jsValue.toString(), QString::fromUtf8(hello));
    }
}

void tst_QJSManagedValue::toNumber()
{
    QJSEngine eng;

    QJSManagedValue undefined(eng.toManagedValue(QVariant()));
    QCOMPARE(qIsNaN(undefined.toNumber()), true);
    QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(undefined)), true);

    QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
    QCOMPARE(null.toNumber(), 0.0);
    QCOMPARE(qjsvalue_cast<qreal>(null), 0.0);

    {
        QJSManagedValue falskt(eng.toManagedValue(false));
        QCOMPARE(falskt.toNumber(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSManagedValue sant(eng.toManagedValue(true));
        QCOMPARE(sant.toNumber(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSManagedValue number(eng.toManagedValue(123.0));
        QCOMPARE(number.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSManagedValue str(eng.toManagedValue(QStringLiteral("ciao")));
        QCOMPARE(qIsNaN(str.toNumber()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSManagedValue str2(eng.toManagedValue(QStringLiteral("123")));
        QCOMPARE(str2.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }

    QJSManagedValue object(eng.newObject(), &eng);
    QCOMPARE(qIsNaN(object.toNumber()), true);
    QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(object)), true);

    QJSManagedValue inv(QJSPrimitiveValue(), &eng);
    QVERIFY(qIsNaN(inv.toNumber()));
    QVERIFY(qIsNaN(qjsvalue_cast<qreal>(inv)));

    // V2 constructors
    {
        QJSManagedValue falskt(QJSPrimitiveValue(false), &eng);
        QCOMPARE(falskt.toNumber(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSManagedValue sant(QJSPrimitiveValue(true), &eng);
        QCOMPARE(sant.toNumber(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSManagedValue number(QJSPrimitiveValue(123.0), &eng);
        QCOMPARE(number.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSManagedValue number2(QJSPrimitiveValue(int(0x43211234)), &eng);
        QCOMPARE(number2.toNumber(), 1126240820.0);

        QJSManagedValue str(QStringLiteral("ciao"), &eng);
        QCOMPARE(qIsNaN(str.toNumber()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSManagedValue str2(QStringLiteral("123"), &eng);
        QCOMPARE(str2.toNumber(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }
}

void tst_QJSManagedValue::toBoolean()
{
    QJSEngine eng;

    QJSManagedValue undefined(eng.toManagedValue(QVariant()));
    QCOMPARE(undefined.type(), QJSManagedValue::Undefined);
    QCOMPARE(undefined.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(undefined), false);

    QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
    QCOMPARE(null.type(), QJSManagedValue::Object);
    QVERIFY(null.isNull());
    QCOMPARE(null.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(null), false);

    {
        QJSManagedValue falskt = eng.toManagedValue(false);
        QCOMPARE(falskt.type(), QJSManagedValue::Boolean);
        QCOMPARE(falskt.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSManagedValue sant(eng.toManagedValue(true));
        QCOMPARE(sant.type(), QJSManagedValue::Boolean);
        QCOMPARE(sant.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSManagedValue number(eng.toManagedValue(0.0));
        QCOMPARE(number.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSManagedValue number2(eng.toManagedValue(qQNaN()));
        QCOMPARE(number2.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSManagedValue number3(eng.toManagedValue(123.0));
        QCOMPARE(number3.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSManagedValue number4(eng.toManagedValue(-456.0));
        QCOMPARE(number4.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSManagedValue str(eng.toManagedValue(QStringLiteral("")));
        QCOMPARE(str.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSManagedValue str2(eng.toManagedValue(QStringLiteral("123")));
        QCOMPARE(str2.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }

    QJSManagedValue object(eng.newObject(), &eng);
    QCOMPARE(object.toBoolean(), true);
    QCOMPARE(qjsvalue_cast<bool>(object), true);

    QJSManagedValue inv(QJSPrimitiveValue(), &eng);
    QCOMPARE(inv.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(inv), false);

    // V2 constructors
    {
        QJSManagedValue falskt(QJSPrimitiveValue(false), &eng);
        QCOMPARE(falskt.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSManagedValue sant(QJSPrimitiveValue(true), &eng);
        QCOMPARE(sant.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSManagedValue number(QJSPrimitiveValue(0.0), &eng);
        QCOMPARE(number.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSManagedValue number2(QJSPrimitiveValue(qQNaN()), &eng);
        QCOMPARE(number2.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSManagedValue number3(QJSPrimitiveValue(123.0), &eng);
        QCOMPARE(number3.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSManagedValue number4(QJSPrimitiveValue(-456.0), &eng);
        QCOMPARE(number4.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSManagedValue number5(QJSPrimitiveValue(0x43211234), &eng);
        QCOMPARE(number5.toBoolean(), true);

        QJSManagedValue str(QJSPrimitiveValue(QStringLiteral("")), &eng);
        QCOMPARE(str.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSManagedValue str2(QJSPrimitiveValue(QStringLiteral("123")), &eng);
        QCOMPARE(str2.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }
}

void tst_QJSManagedValue::toVariant()
{
    QJSEngine eng;

    {
        QJSManagedValue undefined(eng.toManagedValue(QVariant()));
        QCOMPARE(undefined.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(undefined), QVariant());
    }

    {
        QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
        QCOMPARE(null.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(qjsvalue_cast<QVariant>(null), QVariant::fromValue(nullptr));
    }

    {
        QJSManagedValue number(eng.toManagedValue(123.0));
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSManagedValue intNumber(eng.toManagedValue(qint32(123)));
        QCOMPARE(intNumber.toVariant().typeId(), QVariant((qint32)123).typeId());
        QCOMPARE((qjsvalue_cast<QVariant>(intNumber)).typeId(),
                 QVariant(qint32(123)).typeId());

        QJSManagedValue falskt(eng.toManagedValue(false));
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSManagedValue sant(eng.toManagedValue(true));
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSManagedValue str(eng.toManagedValue(QStringLiteral("ciao")));
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QStringLiteral("ciao")));
    }

    {
        QJSManagedValue object(eng.newObject(), &eng);
        QVariant retained = object.toVariant();
        QCOMPARE(retained.metaType(), QMetaType::fromType<QJSValue>());
        QVERIFY(QJSManagedValue(retained.value<QJSValue>(), &eng).strictlyEquals(object));
    }

    {
        QObject temp;
        QJSManagedValue qobject(eng.newQObject(&temp), &eng);
        QVariant var = qobject.toVariant();
        QCOMPARE(var.typeId(), int(QMetaType::QObjectStar));
        QCOMPARE(qvariant_cast<QObject*>(var), (QObject *)&temp);
        QCOMPARE(qobject.toVariant(), var);
    }

    {
        QDateTime dateTime = QDate(1980, 10, 4).startOfDay();
        QJSManagedValue dateObject(eng.toManagedValue(dateTime));
        QVariant var = dateObject.toVariant();
        QCOMPARE(var, QVariant(dateTime));
        QCOMPARE(dateObject.toVariant(), var);
    }

    {
        QRegularExpression rx = QRegularExpression(QStringLiteral("[0-9a-z]+"));
        QJSManagedValue rxObject(eng.toManagedValue(rx));
        QVERIFY(rxObject.isRegularExpression());
        QVariant var = rxObject.toVariant();
        QCOMPARE(var, QVariant(rx));
        QCOMPARE(rxObject.toVariant(), var);
    }

    {
        QJSManagedValue inv;
        QCOMPARE(inv.toVariant(), QVariant());
        QCOMPARE(inv.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(inv), QVariant());
    }

    // V2 constructors
    {
        QJSManagedValue number(QJSPrimitiveValue(123.0), &eng);
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSManagedValue falskt(QJSPrimitiveValue(false), &eng);
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSManagedValue sant(QJSPrimitiveValue(true), &eng);
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSManagedValue str(QStringLiteral("ciao"), &eng);
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QStringLiteral("ciao")));

        QJSManagedValue undef(QJSPrimitiveUndefined(), &eng);
        QCOMPARE(undef.toVariant(), QVariant());
        QCOMPARE(undef.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(undef), QVariant());

        QJSManagedValue nil(QJSPrimitiveNull(), &eng);
        QCOMPARE(nil.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(nil.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(qjsvalue_cast<QVariant>(nil), QVariant::fromValue(nullptr));
    }

    // object
    {
        QVariantMap mapIn;
        mapIn[QStringLiteral("a")] = 123;
        mapIn[QStringLiteral("b")] = QStringLiteral("hello");
        QJSManagedValue object(eng.toManagedValue(mapIn));
        QCOMPARE(object.type(), QJSManagedValue::Object);

        QVariant retained = object.toVariant();
        QCOMPARE(retained.metaType(), QMetaType::fromType<QJSValue>());
        QVERIFY(QJSManagedValue(retained.value<QJSValue>(), &eng).strictlyEquals(object));

        QVariantMap mapOut = retained.toMap();
        QCOMPARE(mapOut.size(), mapIn.size());
        for (auto it = mapOut.begin(), end = mapOut.end(); it != end; ++it)
            QCOMPARE(*it, mapIn[it.key()]);

        // round-trip conversion
        QJSManagedValue object2(eng.toManagedValue(retained));
        QCOMPARE(object2.type(), QJSManagedValue::Object);
        QVERIFY(object2.property(QStringLiteral("a")).strictlyEquals(object.property(QStringLiteral("a"))));
        QVERIFY(object2.property(QStringLiteral("b")).strictlyEquals(object.property(QStringLiteral("b"))));
    }


    // array
    {
        auto handler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &) {
            if (type == QtMsgType::QtWarningMsg)
                QFAIL("Converting QJSManagedValue to QVariant should not cause error messages");
        });
        auto guard = qScopeGuard([&]() { qInstallMessageHandler(handler); });

        QVariantList listIn;
        listIn << 123 << QStringLiteral("hello");
        QJSManagedValue array(eng.toManagedValue(listIn));
        QVERIFY(array.isArray());
        QCOMPARE(array.property(QStringLiteral("length")).toInt(), 2);

        QVariant retained = array.toVariant();
        QCOMPARE(retained.metaType(), QMetaType::fromType<QJSValue>());
        QVERIFY(QJSManagedValue(retained.value<QJSValue>(), &eng).strictlyEquals(array));

        QVariantList listOut = retained.toList();
        QCOMPARE(listOut.size(), listIn.size());
        for (int i = 0; i < listIn.size(); ++i)
            QCOMPARE(listOut.at(i), listIn.at(i));
        // round-trip conversion
        QJSManagedValue array2(eng.toManagedValue(retained));
        QVERIFY(array2.isArray());
        QCOMPARE(array2.property(QStringLiteral("length")).toInt(), array.property(QStringLiteral("length")).toInt());
        for (int i = 0; i < array.property(QStringLiteral("length")).toInt(); ++i)
            QVERIFY(array2.property(i).strictlyEquals(array.property(i)));
    }

    // function
    {
        QJSManagedValue func(eng.evaluate(QStringLiteral("(function() { return 5 + 5 })")), &eng);
        QVERIFY(func.isFunction());
        QCOMPARE(func.call().toInt(), 10);

        QVariant funcVar = func.toVariant();
        QVERIFY(funcVar.isValid());
        QCOMPARE(funcVar.metaType(), QMetaType::fromType<QJSValue>());

        QJSManagedValue func2(eng.toManagedValue(funcVar));
        QVERIFY(func2.isFunction());
        QCOMPARE(func2.call().toInt(), 10);
    }
}

void tst_QJSManagedValue::hasProperty_basic()
{
    QJSEngine eng;
    QJSManagedValue obj(eng.newObject(), &eng);
    QVERIFY(obj.hasProperty(QStringLiteral("hasOwnProperty"))); // inherited from Object.prototype
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("hasOwnProperty")));

    QVERIFY(!obj.hasProperty(QStringLiteral("foo")));
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("foo")));
    obj.setProperty(QStringLiteral("foo"), 123);
    QVERIFY(obj.hasProperty(QStringLiteral("foo")));
    QVERIFY(obj.hasOwnProperty(QStringLiteral("foo")));

    QVERIFY(!obj.hasProperty(QStringLiteral("bar")));
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("bar")));
}

void tst_QJSManagedValue::hasProperty_globalObject()
{
    QJSEngine eng;
    QJSManagedValue global(eng.globalObject(), &eng);
    QVERIFY(global.hasProperty(QStringLiteral("Math")));
    QVERIFY(global.hasOwnProperty(QStringLiteral("Math")));
    QVERIFY(!global.hasProperty(QStringLiteral("NoSuchStandardProperty")));
    QVERIFY(!global.hasOwnProperty(QStringLiteral("NoSuchStandardProperty")));

    QVERIFY(!global.hasProperty(QStringLiteral("foo")));
    QVERIFY(!global.hasOwnProperty(QStringLiteral("foo")));
    global.setProperty(QStringLiteral("foo"), 123);
    QVERIFY(global.hasProperty(QStringLiteral("foo")));
    QVERIFY(global.hasOwnProperty(QStringLiteral("foo")));
}

void tst_QJSManagedValue::hasProperty_changePrototype()
{
    QJSEngine eng;
    QJSManagedValue obj(eng.newObject(), &eng);
    QJSManagedValue proto(eng.newObject(), &eng);
    obj.setPrototype(proto);

    QVERIFY(!obj.hasProperty(QStringLiteral("foo")));
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("foo")));
    proto.setProperty(QStringLiteral("foo"), 123);
    QVERIFY(obj.hasProperty(QStringLiteral("foo")));
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("foo")));

    obj.setProperty(QStringLiteral("foo"), 456); // override prototype property
    QVERIFY(obj.hasProperty(QStringLiteral("foo")));
    QVERIFY(obj.hasOwnProperty(QStringLiteral("foo")));
}

void tst_QJSManagedValue::hasProperty_QTBUG56830_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<QString>("lookup");

    QTest::newRow("bugreport-1") << QStringLiteral("240000000000") << QStringLiteral("3776798720");
    QTest::newRow("bugreport-2") << QStringLiteral("240000000001") << QStringLiteral("3776798721");
    QTest::newRow("biggest-ok-before-bug") << QStringLiteral("238609294221") << QStringLiteral("2386092941");
    QTest::newRow("smallest-bugged") << QStringLiteral("238609294222") << QStringLiteral("2386092942");
    QTest::newRow("biggest-bugged") << QStringLiteral("249108103166") << QStringLiteral("12884901886");
    QTest::newRow("smallest-ok-after-bug") << QStringLiteral("249108103167") << QStringLiteral("12884901887");
}

void tst_QJSManagedValue::hasProperty_QTBUG56830()
{
    QFETCH(QString, key);
    QFETCH(QString, lookup);

    QJSEngine eng;
    QJSManagedValue value(QJSPrimitiveValue(42), &eng);

    QJSManagedValue obj(eng.newObject(), &eng);
    obj.setProperty(key, QJSValue(std::move(value)));
    QVERIFY(obj.hasProperty(key));
    QVERIFY(!obj.hasProperty(lookup));
}

void tst_QJSManagedValue::deleteProperty_basic()
{
    QJSEngine eng;
    QJSManagedValue obj(eng.newObject(), &eng);
    // deleteProperty() behavior matches JS delete operator
    obj.deleteProperty(QStringLiteral("foo"));

    obj.setProperty(QStringLiteral("foo"), 123);
    obj.deleteProperty(QStringLiteral("foo"));
    QVERIFY(!obj.hasOwnProperty(QStringLiteral("foo")));
}

void tst_QJSManagedValue::deleteProperty_globalObject()
{
    QJSEngine eng;
    QJSManagedValue global(eng.globalObject(), &eng);
    // deleteProperty() behavior matches JS delete operator
    global.deleteProperty(QStringLiteral("foo"));

    global.setProperty(QStringLiteral("foo"), 123);
    global.deleteProperty(QStringLiteral("foo"));
    QVERIFY(!global.hasProperty(QStringLiteral("foo")));

    global.deleteProperty(QStringLiteral("Math"));
    QVERIFY(!global.hasProperty(QStringLiteral("Math")));

    global.deleteProperty(QStringLiteral("NaN")); // read-only
    QVERIFY(global.hasProperty(QStringLiteral("NaN")));
}

void tst_QJSManagedValue::deleteProperty_inPrototype()
{
    QJSEngine eng;
    QJSManagedValue obj(eng.newObject(), &eng);
    QJSManagedValue proto(eng.newObject(), &eng);
    obj.setPrototype(proto);

    proto.setProperty(QStringLiteral("foo"), 123);
    QVERIFY(obj.hasProperty(QStringLiteral("foo")));
    // deleteProperty() behavior matches JS delete operator
    obj.deleteProperty(QStringLiteral("foo"));
    QVERIFY(obj.hasProperty(QStringLiteral("foo")));
}

void tst_QJSManagedValue::getSetProperty_propertyRemoval()
{
    QJSEngine eng;
    QJSManagedValue object(eng.newObject(), &eng);
    QJSManagedValue str(eng.toManagedValue(QStringLiteral("bar")));
    QJSManagedValue num(eng.toManagedValue(123.0));

    object.setProperty(QStringLiteral("foo"), num.toJSValue());
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("foo")), &eng).strictlyEquals(num));
    object.setProperty(QStringLiteral("bar"), str.toJSValue());
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("bar")), &eng).strictlyEquals(str));
    object.deleteProperty(QStringLiteral("foo"));
    QVERIFY(!object.hasOwnProperty(QStringLiteral("foo")));
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("bar")), &eng).strictlyEquals(str));
    object.setProperty(QStringLiteral("foo"), num.toJSValue());
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("foo")), &eng).strictlyEquals(num));
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("bar")), &eng).strictlyEquals(str));
    object.deleteProperty(QStringLiteral("bar"));
    QVERIFY(!object.hasOwnProperty(QStringLiteral("bar")));
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("foo")), &eng).strictlyEquals(num));
    object.deleteProperty(QStringLiteral("foo"));
    QVERIFY(!object.hasOwnProperty(QStringLiteral("foo")));

    eng.globalObject().setProperty(QStringLiteral("object3"), object.toJSValue());
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("object3.hasOwnProperty('foo')")), &eng)
             .strictlyEquals(eng.toManagedValue(false)));
    object.setProperty(QStringLiteral("foo"), num.toJSValue());
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("object3.hasOwnProperty('foo')")), &eng)
             .strictlyEquals(eng.toManagedValue(true)));
    QVERIFY(QJSManagedValue(eng.globalObject(), &eng).deleteProperty(QStringLiteral("object3")));
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("this.hasOwnProperty('object3')")), &eng)
             .strictlyEquals(eng.toManagedValue(false)));
}

void tst_QJSManagedValue::getSetProperty_resolveMode()
{
    // test ResolveMode
    QJSEngine eng;
    QJSManagedValue object(eng.newObject(), &eng);
    QJSManagedValue prototype(eng.newObject(), &eng);
    object.setPrototype(prototype);
    QJSManagedValue num2(eng.toManagedValue(456.0));
    prototype.setProperty(QStringLiteral("propertyInPrototype"), num2.toJSValue());
    // default is ResolvePrototype
    QVERIFY(QJSManagedValue(object.property(QStringLiteral("propertyInPrototype")), &eng)
             .strictlyEquals(num2));
}

void tst_QJSManagedValue::getSetProperty_twoEngines()
{
    QJSEngine engine;
    QJSManagedValue object(engine.newObject(), &engine);

    QJSEngine otherEngine;
    QJSManagedValue otherNum(otherEngine.toManagedValue(123));
    object.setProperty(QStringLiteral("oof"), QJSValue(std::move(otherNum)));
    QVERIFY(object.hasOwnProperty(QStringLiteral("oof"))); // primitive values don't have an engine
    QVERIFY(object.property(QStringLiteral("oof")).isNumber());

    QJSManagedValue otherString(otherEngine.toManagedValue(QStringLiteral("nope")));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setProperty() failed: Value was created in different engine.");
    object.setProperty(QStringLiteral("eek"), QJSValue(std::move(otherString)));
    QVERIFY(!object.hasOwnProperty(QStringLiteral("eek"))); // strings are managed
    QVERIFY(object.property(QStringLiteral("eek")).isUndefined());
}

void tst_QJSManagedValue::getSetProperty_gettersAndSettersThrowErrorJS()
{
    // getter/setter that throws an error (from js function)
    QJSEngine eng;
    QJSManagedValue str(eng.toManagedValue(QStringLiteral("bar")));

    eng.evaluate(QStringLiteral(
                     "o = new Object; "
                     "o.__defineGetter__('foo', function() { throw new Error('get foo') }); "
                     "o.__defineSetter__('foo', function() { throw new Error('set foo') }); "));
    QJSManagedValue object(eng.evaluate(QStringLiteral("o")), &eng);
    QCOMPARE(object.type(), QJSManagedValue::Object);
    QVERIFY(!eng.hasError());
    QJSManagedValue ret(object.property(QStringLiteral("foo")), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Undefined);
    QVERIFY(eng.hasError());
    QJSManagedValue error(eng.catchError(), &eng);
    QCOMPARE(error.toString(), QStringLiteral("Error: get foo"));
    QVERIFY(!eng.hasError());
    object.setProperty(QStringLiteral("foo"), QJSValue(std::move(str)));
    QVERIFY(eng.hasError());
    QCOMPARE(eng.catchError().toString(), QStringLiteral("Error: set foo"));
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::getSetProperty_array()
{
    QJSEngine eng;
    QJSManagedValue str(eng.toManagedValue(QStringLiteral("bar")));
    QJSManagedValue num(eng.toManagedValue(123.0));
    QJSManagedValue array(eng.newArray(), &eng);

    QVERIFY(array.isArray());
    array.setProperty(0, num.toJSValue());
    QCOMPARE(array.property(0).toNumber(), num.toNumber());
    QCOMPARE(array.property(QStringLiteral("0")).toNumber(), num.toNumber());
    QCOMPARE(array.property(QStringLiteral("length")).toUInt(), quint32(1));
    array.setProperty(1, str.toJSValue());
    QCOMPARE(array.property(1).toString(), str.toString());
    QCOMPARE(array.property(QStringLiteral("1")).toString(), str.toString());
    QCOMPARE(array.property(QStringLiteral("length")).toUInt(), quint32(2));
    array.setProperty(QStringLiteral("length"), eng.toScriptValue(1));
    QCOMPARE(array.property(QStringLiteral("length")).toUInt(), quint32(1));
    QVERIFY(array.property(1).isUndefined());
}

void tst_QJSManagedValue::getSetProperty()
{
    QJSEngine eng;

    QJSManagedValue object(eng.newObject(), &eng);

    QJSManagedValue str(eng.toManagedValue(QStringLiteral("bar")));
    object.setProperty(QStringLiteral("foo"), str.toJSValue());
    QCOMPARE(object.property(QStringLiteral("foo")).toString(), str.toString());

    QJSManagedValue num(eng.toManagedValue(123.0));
    object.setProperty(QStringLiteral("baz"), num.toJSValue());
    QCOMPARE(object.property(QStringLiteral("baz")).toNumber(), num.toNumber());

    QJSManagedValue strstr(QJSPrimitiveValue("bar"), &eng);
    object.setProperty(QStringLiteral("foo"), strstr.toJSValue());
    QCOMPARE(object.property(QStringLiteral("foo")).toString(), strstr.toString());

    QJSManagedValue numnum(QJSPrimitiveValue(123.0), &eng);
    object.setProperty(QStringLiteral("baz"), numnum.toJSValue());
    QCOMPARE(object.property(QStringLiteral("baz")).toNumber(), numnum.toNumber());

    QJSManagedValue inv;
    inv.setProperty(QStringLiteral("foo"), num.toJSValue());
    QCOMPARE(inv.property(QStringLiteral("foo")).isUndefined(), true);

    eng.globalObject().setProperty(QStringLiteral("object"), object.toJSValue());

    // Setting index property on non-Array
    object.setProperty(13, num.toJSValue());
    QVERIFY(QJSManagedValue(object.property(13), &eng).equals(num));
}

void tst_QJSManagedValue::getSetPrototype_cyclicPrototype()
{
    QJSEngine eng;
    QJSManagedValue prototype(eng.newObject(), &eng);
    QJSManagedValue object(eng.newObject(), &eng);
    object.setPrototype(prototype);

    QJSManagedValue previousPrototype = prototype.prototype();
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Prototype cycle detected.");
    prototype.setPrototype(prototype);
    QCOMPARE(prototype.prototype().strictlyEquals(previousPrototype), true);

    object.setPrototype(prototype);
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Prototype cycle detected.");
    prototype.setPrototype(object);
    QCOMPARE(prototype.prototype().strictlyEquals(previousPrototype), true);
}

void tst_QJSManagedValue::getSetPrototype_evalCyclicPrototype()
{
    QJSEngine eng;
    QJSManagedValue ret(eng.evaluate(QStringLiteral("o = { }; p = { }; o.__proto__ = p; p.__proto__ = o")), &eng);
    QEXPECT_FAIL("", "QJSEngine::evaluate() catches exceptions.", Abort);
    QCOMPARE(ret.type(), QJSManagedValue::Undefined);
    QVERIFY(eng.hasError());
    QJSManagedValue error(eng.catchError(), &eng);
    QCOMPARE(error.toString(), QStringLiteral("TypeError: Could not change prototype."));
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::getSetPrototype_eval()
{
    QJSEngine eng;
    QJSManagedValue ret(eng.evaluate(QStringLiteral("p = { }; p.__proto__ = { }")), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Object);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::getSetPrototype_invalidPrototype()
{
    QJSEngine eng;
    QJSManagedValue inv;
    QJSManagedValue object(eng.newObject(), &eng);
    QJSManagedValue proto = object.prototype();
    QVERIFY(object.prototype().strictlyEquals(proto));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set a prototype on an object (excluding null).");
    inv.setPrototype(object);
    QCOMPARE(inv.prototype().type(), QJSManagedValue::Undefined);
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(inv);
    QVERIFY(object.prototype().strictlyEquals(proto));
}

void tst_QJSManagedValue::getSetPrototype_twoEngines()
{
    QJSEngine eng;
    QJSManagedValue prototype(eng.newObject(), &eng);
    QJSManagedValue object(eng.newObject(), &eng);
    object.setPrototype(prototype);
    QJSEngine otherEngine;
    QJSManagedValue newPrototype(otherEngine.newObject(), &otherEngine);
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Prototype was created in differen engine.");
    object.setPrototype(newPrototype);
    QCOMPARE(object.prototype().strictlyEquals(prototype), true);

}

void tst_QJSManagedValue::getSetPrototype_null()
{
    QJSEngine eng;
    QJSManagedValue object(eng.newObject(), &eng);
    object.setPrototype(QJSManagedValue(QJSPrimitiveNull(), &eng));
    QVERIFY(object.prototype().isNull());

    QJSManagedValue newProto(eng.newObject(), &eng);
    object.setPrototype(newProto);
    QVERIFY(object.prototype().equals(newProto));

    object.setPrototype(QJSManagedValue(eng.evaluate(QStringLiteral("null")), &eng));
    QVERIFY(object.prototype().isNull());
}

void tst_QJSManagedValue::getSetPrototype_notObjectOrNull()
{
    QJSEngine eng;
    QJSManagedValue object(eng.newObject(), &eng);
    QJSManagedValue originalProto = object.prototype();

    // bool
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype({QJSPrimitiveValue(true), &eng});
    QVERIFY(object.prototype().equals(originalProto));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(eng.toManagedValue(true));
    QVERIFY(object.prototype().equals(originalProto));

    // number
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype({QJSPrimitiveValue(123), &eng});
    QVERIFY(object.prototype().equals(originalProto));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(eng.toManagedValue(123));
    QVERIFY(object.prototype().equals(originalProto));

    // string
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype({QStringLiteral("foo"), &eng});
    QVERIFY(object.prototype().equals(originalProto));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(eng.toManagedValue(QStringLiteral("foo")));
    QVERIFY(object.prototype().equals(originalProto));

    // undefined
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(QJSManagedValue(QJSPrimitiveUndefined(), &eng));
    QVERIFY(object.prototype().equals(originalProto));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::setPrototype() failed: Can only set objects (including null) as prototypes.");
    object.setPrototype(eng.toManagedValue(QVariant()));
    QVERIFY(object.prototype().equals(originalProto));
}

void tst_QJSManagedValue::getSetPrototype()
{
    QJSEngine eng;
    QJSManagedValue prototype(eng.newObject(), &eng);
    QJSManagedValue object(eng.newObject(), &eng);
    object.setPrototype(prototype);
    QVERIFY(object.prototype().strictlyEquals(prototype));
}

void tst_QJSManagedValue::call_function()
{
    QJSEngine eng;
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return 1; })")), &eng);
    QVERIFY(fun.isFunction());
    QJSManagedValue result(fun.call(), &eng);
    QCOMPARE(result.type(), QJSManagedValue::Number);
    QCOMPARE(result.toInteger(), 1);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::call_object()
{
    QJSEngine eng;
    QJSManagedValue object(eng.evaluate(QStringLiteral("Object")), &eng);
    QCOMPARE(object.isFunction(), true);
    QJSManagedValue result(object.callWithInstance(object.toJSValue()), &eng);
    QCOMPARE(result.type(), QJSManagedValue::Object);
    QVERIFY(!result.isNull());
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::call_newObjects()
{
    QJSEngine eng;
    // test that call() doesn't construct new objects
    QJSManagedValue number(eng.evaluate(QStringLiteral("Number")), &eng);
    QJSManagedValue object(eng.evaluate(QStringLiteral("Object")), &eng);
    QCOMPARE(object.isFunction(), true);
    QJSValueList args;
    args << eng.toScriptValue(123);
    QJSManagedValue result(number.callWithInstance(object.toJSValue(), args), &eng);
    QVERIFY(result.strictlyEquals(QJSManagedValue(args.at(0), &eng)));
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::call_this()
{
    QJSEngine eng;
    // test that correct "this" object is used
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return this; })")), &eng);
    QCOMPARE(fun.isFunction(), true);

    QJSManagedValue numberObject(eng.evaluate(QStringLiteral("new Number(123)")), &eng);
    QJSManagedValue result(fun.callWithInstance(QJSValue(std::move(numberObject))), &eng);
    QCOMPARE(result.type(), QJSManagedValue::Object);
    QCOMPARE(result.toNumber(), 123.0);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::call_arguments()
{
    QJSEngine eng;
    // test that correct arguments are passed

    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return arguments[0]; })")), &eng);
    QCOMPARE(fun.isFunction(), true);
    {
        QJSManagedValue result(fun.callWithInstance(eng.toScriptValue(QVariant())), &eng);
        QCOMPARE(result.type(), QJSManagedValue::Undefined);
        QVERIFY(!eng.hasError());
    }
    {
        QJSValueList args;
        args << eng.toScriptValue(123.0);
        QJSManagedValue result(fun.callWithInstance(eng.toScriptValue(QVariant()), args), &eng);
        QCOMPARE(result.type(), QJSManagedValue::Number);
        QCOMPARE(result.toNumber(), 123.0);
        QVERIFY(!eng.hasError());
    }
    // V2 constructors
    {
        QJSValueList args;
        args << QJSValue(123.0);
        QJSManagedValue result(fun.callWithInstance(eng.toScriptValue(QVariant()), args), &eng);
        QCOMPARE(result.type(), QJSManagedValue::Number);
        QCOMPARE(result.toNumber(), 123.0);
        QVERIFY(!eng.hasError());
    }
}

void tst_QJSManagedValue::call()
{
    QJSEngine eng;
    {
        QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return arguments[1]; })")), &eng);
        QCOMPARE(fun.isFunction(), true);

        QJSValueList args;
        args << eng.toScriptValue(123.0) << eng.toScriptValue(456.0);
        QJSManagedValue result(fun.callWithInstance(eng.toScriptValue(QVariant()), args), &eng);
        QCOMPARE(result.type(), QJSManagedValue::Number);
        QCOMPARE(result.toNumber(), 456.0);
        QVERIFY(!eng.hasError());
    }
    {
        QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { throw new Error('foo'); })")), &eng);
        QCOMPARE(fun.isFunction(), true);
        QVERIFY(!eng.hasError());

        QJSManagedValue result(fun.call(), &eng);
        QCOMPARE(result.type(), QJSManagedValue::Undefined);
        QVERIFY(eng.hasError());
        QJSManagedValue error(eng.catchError(), &eng);
        QVERIFY(error.toString().contains("foo"));
    }
}

void tst_QJSManagedValue::call_twoEngines()
{
    QJSEngine eng;
    QJSManagedValue object(eng.evaluate(QStringLiteral("Object")), &eng);
    QJSEngine otherEngine;
    QJSManagedValue fun(otherEngine.evaluate(QStringLiteral("(function() { return 1; })")), &otherEngine);
    QVERIFY(fun.isFunction());
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::callWithInstance() failed: Instance was created in different engine.");
    QVERIFY(fun.callWithInstance(QJSValue(std::move(object))).isUndefined());

    // Primitive value doesn't need an engine
    QVERIFY(!fun.call(QJSValueList() << eng.toScriptValue(123)).isUndefined());

    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::call() failed: Argument was created in different engine.");
    QVERIFY(fun.call(QJSValueList() << eng.toScriptValue(QStringLiteral("string")))
            .isUndefined());
    {
        QJSManagedValue fun(eng.evaluate(QStringLiteral("Object")), &eng);
        QVERIFY(fun.isFunction());
        QJSEngine eng2;
        QJSManagedValue objectInDifferentEngine(eng2.newObject(), &eng2);
        QJSValueList args;
        args << QJSValue(std::move(objectInDifferentEngine));
        QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::call() failed: Argument was created in different engine.");
        fun.call(args);
    }
}

void tst_QJSManagedValue::call_nonFunction_data()
{
    newEngine();
    QTest::addColumn<QJSValue>("value");

    QTest::newRow("invalid")   << QJSValue();
    QTest::newRow("bool")      << QJSValue(false);
    QTest::newRow("int")       << QJSValue(123);
    QTest::newRow("string")    << QJSValue(QStringLiteral("ciao"));
    QTest::newRow("undefined") << QJSValue(QJSValue::UndefinedValue);
    QTest::newRow("null")      << QJSValue(QJSValue::NullValue);

    QTest::newRow("bool bound")      << engine->toScriptValue(false);
    QTest::newRow("int bound")       << engine->toScriptValue(123);
    QTest::newRow("string bound")    << engine->toScriptValue(QStringLiteral("ciao"));
    QTest::newRow("undefined bound") << engine->toScriptValue(QVariant());
    QTest::newRow("null bound")      << engine->evaluate(QStringLiteral("null"));
}

void tst_QJSManagedValue::call_nonFunction()
{
    // calling things that are not functions
    QFETCH(QJSValue, value);
    QVERIFY(QJSManagedValue(std::move(value), engine.data()).call().isUndefined());
    QVERIFY(engine->hasError());
    QJSManagedValue error(engine->catchError(), engine.data());
    QVERIFY(error.toString().contains("TypeError"));
}

void tst_QJSManagedValue::construct_nonFunction_data()
{
    call_nonFunction_data();
}

void tst_QJSManagedValue::construct_nonFunction()
{
    QFETCH(QJSValue, value);
    QVERIFY(QJSManagedValue(std::move(value), engine.data()).callAsConstructor().isUndefined());
    QVERIFY(engine->hasError());
    QJSManagedValue error(engine->catchError(), engine.data());
    QVERIFY(error.toString().contains("TypeError"));
}

void tst_QJSManagedValue::construct_simple()
{
    QJSEngine eng;
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function () { this.foo = 123; })")), &eng);
    QVERIFY(fun.isFunction());
    QJSManagedValue ret(fun.callAsConstructor(), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Object);
    QVERIFY(!ret.isNull());
    QVERIFY(ret.prototype().strictlyEquals(
                QJSManagedValue(fun.property(QStringLiteral("prototype")), &eng)));
    QCOMPARE(ret.property(QStringLiteral("foo")).toInt(), 123);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_newObjectJS()
{
    QJSEngine eng;
    // returning a different object overrides the default-constructed one
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function () { return { bar: 456 }; })")), &eng);
    QVERIFY(fun.isFunction());
    QJSManagedValue ret(fun.callAsConstructor(), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Object);
    QVERIFY(!ret.isNull());
    QVERIFY(!ret.prototype().strictlyEquals(
                QJSManagedValue(fun.property(QStringLiteral("prototype")), &eng)));
    QCOMPARE(ret.property(QStringLiteral("bar")).toInt(), 456);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_arg()
{
    QJSEngine eng;
    QJSManagedValue Number(eng.evaluate(QStringLiteral("Number")), &eng);
    QCOMPARE(Number.isFunction(), true);
    QJSValueList args;
    args << eng.toScriptValue(123);
    QJSManagedValue ret(Number.callAsConstructor(args), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Object);
    QVERIFY(!ret.isNull());
    QCOMPARE(ret.toNumber(), args.at(0).toNumber());
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_proto()
{
    QJSEngine eng;
    // test that internal prototype is set correctly
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return this.__proto__; })")), &eng);
    QCOMPARE(fun.isFunction(), true);
    QCOMPARE(fun.property(QStringLiteral("prototype")).isObject(), true);
    QJSManagedValue ret(fun.callAsConstructor(), &eng);
    QVERIFY(QJSManagedValue(fun.property(QStringLiteral("prototype")), &eng).strictlyEquals(ret));
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_returnInt()
{
    QJSEngine eng;
    // test that we return the new object even if a non-object value is returned from the function
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { return 123; })")), &eng);
    QCOMPARE(fun.isFunction(), true);
    QJSManagedValue ret(fun.callAsConstructor(), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Object);
    QVERIFY(!ret.isNull());
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_throw()
{
    QJSEngine eng;
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { throw new Error('foo'); })")), &eng);
    QCOMPARE(fun.isFunction(), true);
    QVERIFY(!eng.hasError());
    QJSManagedValue ret(fun.callAsConstructor(), &eng);
    QCOMPARE(ret.type(), QJSManagedValue::Undefined);
    QVERIFY(eng.hasError());
    QJSManagedValue error(eng.catchError(), &eng);
    QVERIFY(!eng.hasError());
}

void tst_QJSManagedValue::construct_twoEngines()
{
    QJSEngine engine;
    QJSEngine otherEngine;
    QJSManagedValue ctor(engine.evaluate(QStringLiteral("(function (a, b) { this.foo = 123; })")), &engine);

    QJSManagedValue arg(otherEngine.toManagedValue(124567));
    QVERIFY(!ctor.callAsConstructor(QJSValueList() << arg.toJSValue()).isUndefined());

    QJSManagedValue arg2(otherEngine.toManagedValue(QStringLiteral("string")));
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::callAsConstructor() failed: Argument was created in different engine.");
    QVERIFY(ctor.callAsConstructor(QJSValueList() << arg2.toJSValue()).isUndefined());
    QTest::ignoreMessage(QtWarningMsg, "QJSManagedValue::callAsConstructor() failed: Argument was created in different engine.");

    QVERIFY(ctor.callAsConstructor(QJSValueList() << arg.toJSValue() << otherEngine.newObject()).isUndefined());
}

void tst_QJSManagedValue::construct_constructorThrowsPrimitive()
{
    QJSEngine eng;
    QJSManagedValue fun(eng.evaluate(QStringLiteral("(function() { throw 123; })")), &eng);
    QVERIFY(fun.isFunction());
    // construct(QJSValueList)
    {
        QJSManagedValue ret(fun.callAsConstructor(), &eng);
        QCOMPARE(ret.type(), QJSManagedValue::Undefined);
        QVERIFY(eng.hasError());
        QJSManagedValue error(eng.catchError(), &eng);
        QCOMPARE(error.toNumber(), 123.0);
        QVERIFY(!eng.hasError());
    }
}

void tst_QJSManagedValue::equals()
{
    QJSEngine eng;
    QObject temp;

    QVERIFY(QJSManagedValue().equals(QJSManagedValue()));

    QJSManagedValue num(eng.toManagedValue(123));
    QVERIFY(num.equals(eng.toManagedValue(123)));
    QVERIFY(!num.equals(eng.toManagedValue(321)));
    QVERIFY(num.equals(eng.toManagedValue(QStringLiteral("123"))));
    QVERIFY(!num.equals(eng.toManagedValue(QStringLiteral("321"))));
    QVERIFY(num.equals({eng.evaluate(QStringLiteral("new Number(123)")), &eng}));
    QVERIFY(!num.equals({eng.evaluate(QStringLiteral("new Number(321)")), &eng}));
    QVERIFY(num.equals({eng.evaluate(QStringLiteral("new String('123')")), &eng}));
    QVERIFY(!num.equals({eng.evaluate(QStringLiteral("new String('321')")), &eng}));
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("new Number(123)")), &eng).equals(num));
    QVERIFY(!num.equals(QJSManagedValue()));

    QJSManagedValue str(eng.toManagedValue(QStringLiteral("123")));
    QVERIFY(str.equals(eng.toManagedValue(QStringLiteral("123"))));
    QVERIFY(!str.equals(eng.toManagedValue(QStringLiteral("321"))));
    QVERIFY(str.equals(eng.toManagedValue(123)));
    QVERIFY(!str.equals(eng.toManagedValue(321)));
    QVERIFY(str.equals({eng.evaluate(QStringLiteral("new String('123')")), &eng}));
    QVERIFY(!str.equals({eng.evaluate(QStringLiteral("new String('321')")), &eng}));
    QVERIFY(str.equals({eng.evaluate(QStringLiteral("new Number(123)")), &eng}));
    QVERIFY(!str.equals({eng.evaluate(QStringLiteral("new Number(321)")), &eng}));
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("new String('123')")), &eng).equals(str));
    QCOMPARE(str.equals(QJSManagedValue()), false);

    QJSManagedValue num2(QJSPrimitiveValue(123), &eng);
    QVERIFY(num2.equals(QJSManagedValue(QJSPrimitiveValue(123), &eng)));
    QVERIFY(!num2.equals(QJSManagedValue(QJSPrimitiveValue(321), &eng)));
    QVERIFY(num2.equals(QJSManagedValue(QStringLiteral("123"), &eng)));
    QVERIFY(!num2.equals(QJSManagedValue(QStringLiteral("321"), &eng)));
    QVERIFY(!num2.equals(QJSManagedValue()));

    QJSManagedValue str2(QStringLiteral("123"), &eng);
    QVERIFY(str2.equals(QJSManagedValue(QStringLiteral("123"), &eng)));
    QVERIFY(!str2.equals(QJSManagedValue(QStringLiteral("321"), &eng)));
    QVERIFY(str2.equals(QJSManagedValue(QJSPrimitiveValue(123), &eng)));
    QVERIFY(!str2.equals(QJSManagedValue(QJSPrimitiveValue(321), &eng)));
    QVERIFY(!str2.equals(QJSManagedValue()));

    QJSManagedValue date1(eng.toManagedValue(QDate(2000, 1, 1).startOfDay()));
    QJSManagedValue date2(eng.toManagedValue(QDate(1999, 1, 1).startOfDay()));
    QCOMPARE(date1.equals(date2), false);
    QCOMPARE(date1.equals(date1), true);
    QCOMPARE(date2.equals(date2), true);

    QJSManagedValue undefined(eng.toManagedValue(QVariant()));
    QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
    QCOMPARE(undefined.equals(undefined), true);
    QCOMPARE(null.equals(null), true);
    QCOMPARE(undefined.equals(null), true);
    QCOMPARE(null.equals(undefined), true);
    QVERIFY(undefined.equals(QJSManagedValue()));
    QVERIFY(null.equals(QJSManagedValue()));
    QVERIFY(!null.equals(num));
    QVERIFY(!undefined.equals(num));

    QJSManagedValue sant(eng.toManagedValue(true));
    QVERIFY(sant.equals(eng.toManagedValue(1)));
    QVERIFY(sant.equals(eng.toManagedValue(QStringLiteral("1"))));
    QVERIFY(sant.equals(sant));
    QVERIFY(sant.equals({eng.evaluate(QStringLiteral("new Number(1)")), &eng}));
    QVERIFY(sant.equals({eng.evaluate(QStringLiteral("new String('1')")), &eng}));
    QVERIFY(sant.equals({eng.evaluate(QStringLiteral("new Boolean(true)")), &eng}));
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("new Boolean(true)")), &eng).equals(sant));
    QVERIFY(!sant.equals(eng.toManagedValue(0)));
    QVERIFY(!sant.equals(undefined));
    QVERIFY(!sant.equals(null));

    QJSManagedValue falskt(eng.toManagedValue(false));
    QVERIFY(falskt.equals(eng.toManagedValue(0)));
    QVERIFY(falskt.equals(eng.toManagedValue(QStringLiteral("0"))));
    QVERIFY(falskt.equals(falskt));
    QVERIFY(falskt.equals({eng.evaluate(QStringLiteral("new Number(0)")), &eng}));
    QVERIFY(falskt.equals({eng.evaluate(QStringLiteral("new String('0')")), &eng}));
    QVERIFY(falskt.equals({eng.evaluate(QStringLiteral("new Boolean(false)")), &eng}));
    QVERIFY(QJSManagedValue(eng.evaluate(QStringLiteral("new Boolean(false)")), &eng).equals(falskt));
    QVERIFY(!falskt.equals(sant));
    QVERIFY(!falskt.equals(undefined));
    QVERIFY(!falskt.equals(null));

    QJSManagedValue obj1(eng.newObject(), &eng);
    QJSManagedValue obj2(eng.newObject(), &eng);
    QCOMPARE(obj1.equals(obj2), false);
    QCOMPARE(obj2.equals(obj1), false);
    QCOMPARE(obj1.equals(obj1), true);
    QCOMPARE(obj2.equals(obj2), true);

    QJSManagedValue qobj1(eng.newQObject(&temp), &eng);
    QJSManagedValue qobj2(eng.newQObject(&temp), &eng);
    QJSManagedValue qobj3(eng.newQObject(nullptr), &eng);

    // FIXME: No ScriptOwnership: QJSManagedValue qobj4(eng.newQObject(new QObject(), QScriptEngine::ScriptOwnership), &eng);
    QJSManagedValue qobj4(eng.newQObject(new QObject()), &eng);

    QVERIFY(qobj1.equals(qobj2)); // compares the QObject pointers
    QVERIFY(!qobj2.equals(qobj4)); // compares the QObject pointers
    QVERIFY(!qobj2.equals(obj2)); // compares the QObject pointers

    QJSManagedValue compareFun(eng.evaluate(QStringLiteral("(function(a, b) { return a == b; })")), &eng);
    QVERIFY(compareFun.isFunction());
    {
        QJSManagedValue ret(compareFun.call(QJSValueList() << qobj1.toJSValue() << qobj2.toJSValue()), &eng);
        QCOMPARE(ret.type(), QJSManagedValue::Boolean);
        ret = QJSManagedValue(compareFun.call(QJSValueList() << qobj1.toJSValue() << qobj3.toJSValue()), &eng);
        QCOMPARE(ret.type(), QJSManagedValue::Boolean);
        QVERIFY(!ret.toBoolean());
        ret = QJSManagedValue(compareFun.call(QJSValueList() << qobj1.toJSValue() << qobj4.toJSValue()), &eng);
        QCOMPARE(ret.type(), QJSManagedValue::Boolean);
        QVERIFY(!ret.toBoolean());
        ret = QJSManagedValue(compareFun.call(QJSValueList() << qobj1.toJSValue() << obj1.toJSValue()), &eng);
        QCOMPARE(ret.type(), QJSManagedValue::Boolean);
        QVERIFY(!ret.toBoolean());
    }

    {
        QJSManagedValue var1(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QJSManagedValue var2(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QVERIFY(var1.equals(var2));
    }
    {
        QJSManagedValue var1(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QJSManagedValue var2(eng.toManagedValue(QVariant::fromValue(QPoint(3, 4))));
        QVERIFY(!var1.equals(var2));
    }
}

void tst_QJSManagedValue::strictlyEquals()
{
    QJSEngine eng;
    QObject temp;

    QVERIFY(QJSManagedValue().strictlyEquals(QJSManagedValue()));

    QJSManagedValue num(eng.toManagedValue(123));
    QVERIFY(num.strictlyEquals(eng.toManagedValue(123)));
    QVERIFY(!num.strictlyEquals(eng.toManagedValue(321)));
    QVERIFY(!num.strictlyEquals(eng.toManagedValue(QStringLiteral("123"))));
    QVERIFY(!num.strictlyEquals(eng.toManagedValue(QStringLiteral("321"))));
    QVERIFY(!num.strictlyEquals({eng.evaluate(QStringLiteral("new Number(123)")), &eng}));
    QVERIFY(!num.strictlyEquals({eng.evaluate(QStringLiteral("new Number(321)")), &eng}));
    QVERIFY(!num.strictlyEquals({eng.evaluate(QStringLiteral("new String('123')")), &eng}));
    QVERIFY(!num.strictlyEquals({eng.evaluate(QStringLiteral("new String('321')")), &eng}));
    QVERIFY(!QJSManagedValue(eng.evaluate(QStringLiteral("new Number(123)")), &eng).strictlyEquals(num));
    QVERIFY(!num.strictlyEquals(QJSManagedValue()));
    QVERIFY(!QJSManagedValue().strictlyEquals(num));

    QJSManagedValue str(eng.toManagedValue(QStringLiteral("123")));
    QVERIFY(str.strictlyEquals(eng.toManagedValue(QStringLiteral("123"))));
    QVERIFY(!str.strictlyEquals(eng.toManagedValue(QStringLiteral("321"))));
    QVERIFY(!str.strictlyEquals(eng.toManagedValue(123)));
    QVERIFY(!str.strictlyEquals(eng.toManagedValue(321)));
    QVERIFY(!str.strictlyEquals({eng.evaluate(QStringLiteral("new String('123')")), &eng}));
    QVERIFY(!str.strictlyEquals({eng.evaluate(QStringLiteral("new String('321')")), &eng}));
    QVERIFY(!str.strictlyEquals({eng.evaluate(QStringLiteral("new Number(123)")), &eng}));
    QVERIFY(!str.strictlyEquals({eng.evaluate(QStringLiteral("new Number(321)")), &eng}));
    QVERIFY(!QJSManagedValue(eng.evaluate(QStringLiteral("new String('123')")), &eng).strictlyEquals(str));
    QVERIFY(!str.strictlyEquals(QJSManagedValue()));

    QJSManagedValue num2(QJSPrimitiveValue(123), &eng);
    QVERIFY(num2.strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!num2.strictlyEquals({QJSPrimitiveValue(321), &eng}));
    QVERIFY(!num2.strictlyEquals({QStringLiteral("123"), &eng}));
    QVERIFY(!num2.strictlyEquals({QStringLiteral("321"), &eng}));
    QVERIFY(!num2.strictlyEquals(QJSManagedValue()));

    QJSManagedValue str2(QStringLiteral("123"), &eng);
    QVERIFY(str2.strictlyEquals({QStringLiteral("123"), &eng}));
    QVERIFY(!str2.strictlyEquals({QStringLiteral("321"), &eng}));
    QVERIFY(!str2.strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!str2.strictlyEquals({QJSPrimitiveValue(321), &eng}));
    QVERIFY(!str2.strictlyEquals(QJSManagedValue()));

    QJSManagedValue date1(eng.toManagedValue(QDate(2000, 1, 1).startOfDay()));
    QJSManagedValue date2(eng.toManagedValue(QDate(1999, 1, 1).startOfDay()));
    QCOMPARE(date1.strictlyEquals(date2), false);
    QCOMPARE(date1.strictlyEquals(date1), true);
    QCOMPARE(date2.strictlyEquals(date2), true);
    QVERIFY(!date1.strictlyEquals(QJSManagedValue()));

    QJSManagedValue undefined(eng.toManagedValue(QVariant()));
    QJSManagedValue null(eng.evaluate(QStringLiteral("null")), &eng);
    QCOMPARE(undefined.strictlyEquals(undefined), true);
    QCOMPARE(null.strictlyEquals(null), true);
    QCOMPARE(undefined.strictlyEquals(null), false);
    QCOMPARE(null.strictlyEquals(undefined), false);
    QVERIFY(!null.strictlyEquals(QJSManagedValue()));

    QJSManagedValue sant(eng.toManagedValue(true));
    QVERIFY(!sant.strictlyEquals(eng.toManagedValue(1)));
    QVERIFY(!sant.strictlyEquals(eng.toManagedValue(QStringLiteral("1"))));
    QVERIFY(sant.strictlyEquals(sant));
    QVERIFY(!sant.strictlyEquals({eng.evaluate(QStringLiteral("new Number(1)")), &eng}));
    QVERIFY(!sant.strictlyEquals({eng.evaluate(QStringLiteral("new String('1')")), &eng}));
    QVERIFY(!sant.strictlyEquals({eng.evaluate(QStringLiteral("new Boolean(true)")), &eng}));
    QVERIFY(!QJSManagedValue(eng.evaluate(QStringLiteral("new Boolean(true)")), &eng).strictlyEquals(sant));
    QVERIFY(!sant.strictlyEquals(eng.toManagedValue(0)));
    QVERIFY(!sant.strictlyEquals(undefined));
    QVERIFY(!sant.strictlyEquals(null));
    QVERIFY(!sant.strictlyEquals(QJSManagedValue()));

    QJSManagedValue falskt(eng.toManagedValue(false));
    QVERIFY(!falskt.strictlyEquals(eng.toManagedValue(0)));
    QVERIFY(!falskt.strictlyEquals(eng.toManagedValue(QStringLiteral("0"))));
    QVERIFY(falskt.strictlyEquals(falskt));
    QVERIFY(!falskt.strictlyEquals({eng.evaluate(QStringLiteral("new Number(0)")), &eng}));
    QVERIFY(!falskt.strictlyEquals({eng.evaluate(QStringLiteral("new String('0')")), &eng}));
    QVERIFY(!falskt.strictlyEquals({eng.evaluate(QStringLiteral("new Boolean(false)")), &eng}));
    QVERIFY(!QJSManagedValue(eng.evaluate(QStringLiteral("new Boolean(false)")), &eng).strictlyEquals(falskt));
    QVERIFY(!falskt.strictlyEquals(sant));
    QVERIFY(!falskt.strictlyEquals(undefined));
    QVERIFY(!falskt.strictlyEquals(null));
    QVERIFY(!falskt.strictlyEquals(QJSManagedValue()));

    QVERIFY(!QJSManagedValue(QJSPrimitiveValue(false), &eng).strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveUndefined(), &eng).strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveNull(), &eng).strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveValue(false), &eng).strictlyEquals({QStringLiteral("ciao"), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveUndefined(), &eng).strictlyEquals({QStringLiteral("ciao"), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveNull(), &eng).strictlyEquals({QStringLiteral("ciao"), &eng}));
    QVERIFY(eng.toManagedValue(QStringLiteral("ciao")).strictlyEquals(QJSManagedValue(QStringLiteral("ciao"), &eng)));
    QVERIFY(QJSManagedValue(QStringLiteral("ciao"), &eng).strictlyEquals(eng.toManagedValue(QStringLiteral("ciao"))));
    QVERIFY(!QJSManagedValue(QStringLiteral("ciao"), &eng).strictlyEquals({QJSPrimitiveValue(123), &eng}));
    QVERIFY(!QJSManagedValue(QStringLiteral("ciao"), &eng).strictlyEquals(eng.toManagedValue(123)));
    QVERIFY(!QJSManagedValue(QJSPrimitiveValue(123), &eng).strictlyEquals({QStringLiteral("ciao"), &eng}));
    QVERIFY(!QJSManagedValue(QJSPrimitiveValue(123), &eng).strictlyEquals(eng.toManagedValue(QStringLiteral("ciao"))));
    QVERIFY(!eng.toManagedValue(123).strictlyEquals(QJSManagedValue(QStringLiteral("ciao"), &eng)));

    QJSManagedValue obj1(eng.newObject(), &eng);
    QJSManagedValue obj2(eng.newObject(), &eng);
    QCOMPARE(obj1.strictlyEquals(obj2), false);
    QCOMPARE(obj2.strictlyEquals(obj1), false);
    QCOMPARE(obj1.strictlyEquals(obj1), true);
    QCOMPARE(obj2.strictlyEquals(obj2), true);
    QVERIFY(!obj1.strictlyEquals(QJSManagedValue()));

    QJSManagedValue qobj1(eng.newQObject(&temp), &eng);
    QJSManagedValue qobj2(eng.newQObject(&temp), &eng);
    QVERIFY(qobj1.strictlyEquals(qobj2));

    {
        QJSManagedValue var1(eng.toManagedValue(QVariant(QStringList() << QStringLiteral("a"))));
        QJSManagedValue var2(eng.toManagedValue(QVariant(QStringList() << QStringLiteral("a"))));
        QVERIFY(var1.isArray());
        QVERIFY(var2.isArray());
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSManagedValue var1(eng.toManagedValue(QVariant(QStringList() << QStringLiteral("a"))));
        QJSManagedValue var2(eng.toManagedValue(QVariant(QStringList() << QStringLiteral("b"))));
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSManagedValue var1(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QJSManagedValue var2(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QVERIFY(var1.strictlyEquals(var2));
    }
    {
        QJSManagedValue var1(eng.toManagedValue(QVariant::fromValue(QPoint(1, 2))));
        QJSManagedValue var2(eng.toManagedValue(QVariant::fromValue(QPoint(3, 4))));
        QVERIFY(!var1.strictlyEquals(var2));
    }

    {
        // Import QtQml to trigger the registration of QStringList, which makes it a sequence
        // type, rather than a generic JS array.
        QQmlEngine qmlEngine;
        QQmlComponent c(&qmlEngine);
        c.setData("import QtQml\nQtObject {}", QUrl());
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(!obj.isNull());

        QJSManagedValue var1(qmlEngine.toManagedValue(QVariant(QStringList() << QStringLiteral("a"))));
        QJSManagedValue var2(qmlEngine.toManagedValue(QVariant(QStringList() << QStringLiteral("a"))));
        QVERIFY(!var1.isArray());
        QVERIFY(!var2.isArray());
        QVERIFY(!var1.strictlyEquals(var2));
    }
}

void tst_QJSManagedValue::castToPointer()
{
    QJSEngine eng;
    {
        QRect c(123, 210, 231, 10);
        QJSManagedValue v(eng.toManagedValue(&c));
        QRect *cp = qjsvalue_cast<QRect*>(v);
        QVERIFY(cp != nullptr);
        QCOMPARE(*cp, c);

        QPoint *bp = qjsvalue_cast<QPoint*>(v);
        QVERIFY(!bp);

        QJSManagedValue v2(eng.toManagedValue(QVariant::fromValue(cp)));
        QCOMPARE(qjsvalue_cast<QRect*>(v2), cp);
    }
}

void tst_QJSManagedValue::engineDeleted()
{
    QJSEngine *eng = new QJSEngine;
    QObject *temp = new QObject(); // Owned by JS engine, as newQObject() sets JS ownership explicitly
    QJSManagedValue v1(eng->toManagedValue(123));
    QCOMPARE(v1.type(), QJSManagedValue::Number);
    QJSManagedValue v2(eng->toManagedValue(QStringLiteral("ciao")));
    QCOMPARE(v2.type(), QJSManagedValue::String);
    QJSManagedValue v3(eng->newObject(), eng);
    QCOMPARE(v3.type(), QJSManagedValue::Object);
    QVERIFY(!v3.isNull());
    QJSManagedValue v4(eng->newQObject(temp), eng);
    QCOMPARE(v4.type(), QJSManagedValue::Object);
    QVERIFY(!v4.isNull());
    QJSManagedValue v5(QStringLiteral("Hello"), eng);
    QCOMPARE(v2.type(), QJSManagedValue::String);

    delete eng;

    // You can still check the type, but anything involving the engine is obviously prohibited.
    QCOMPARE(v1.type(), QJSManagedValue::Undefined);
    QCOMPARE(v2.type(), QJSManagedValue::Undefined);
    QCOMPARE(v3.type(), QJSManagedValue::Undefined);
    QCOMPARE(v4.type(), QJSManagedValue::Undefined);
    QCOMPARE(v5.type(), QJSManagedValue::Undefined);
}

void tst_QJSManagedValue::valueOfWithClosure()
{
    QJSEngine eng;
    // valueOf()
    {
        QJSManagedValue obj(eng.evaluate(QStringLiteral("o = {}; (function(foo) { o.valueOf = function() { return foo; } })(123); o")), &eng);
        QCOMPARE(obj.type(), QJSManagedValue::Object);
        QVERIFY(!obj.isNull());
        QCOMPARE(obj.toNumber(), 123);
    }
    // toString()
    {
        QJSManagedValue obj(eng.evaluate(QStringLiteral("o = {}; (function(foo) { o.toString = function() { return foo; } })('ciao'); o")), &eng);
        QCOMPARE(obj.type(), QJSManagedValue::Object);
        QVERIFY(!obj.isNull());
        QCOMPARE(obj.toString(), QStringLiteral("ciao"));
    }
}

static int instanceCount = 0;

struct MyType
{
    MyType(int n = 0, const char *t=nullptr): number(n), text(t)
    {
        ++instanceCount;
    }
    MyType(const MyType &other)
        : number(other.number), text(other.text)
    {
        ++instanceCount;
    }
    ~MyType()
    {
        --instanceCount;
    }
    int number;
    const char *text;
};

Q_DECLARE_METATYPE(MyType)
Q_DECLARE_METATYPE(MyType*)

void tst_QJSManagedValue::jsvalueArrayToSequenceType()
{
    QCOMPARE(instanceCount, 0);
    {
        QJSEngine eng {};
        QJSManagedValue testObject(eng.newObject(), &eng);
        testObject.setProperty(QStringLiteral("test"), 42);
        testObject.setProperty(QStringLiteral("mytypeobject"), eng.toScriptValue(QVariant::fromValue(MyType {42, "hello"})));
        auto array = eng.newArray(4);
        array.setProperty(0, QStringLiteral("Hello World"));
        array.setProperty(1, 42);
        array.setProperty(2, QJSValue());
        array.setProperty(3, QJSValue(std::move(testObject)));
        auto asVariant = QVariant::fromValue(array);
        QVERIFY(asVariant.canConvert<QVariantList>());
        auto asIterable = asVariant.value<QSequentialIterable>();
        for (auto it = asIterable.begin(); it != asIterable.end(); ++it) {
            Q_UNUSED(*it);
        }
        int i = 0;
        for (QVariant myVariant: asIterable) {
            QCOMPARE(myVariant.isValid(), i != 2);
            ++i;
        }
        QVERIFY(asIterable.at(2).value<QVariant>().isNull());
        QCOMPARE(asIterable.at(3).value<QVariantMap>().find(QStringLiteral("mytypeobject"))->value<MyType>().number, 42);
        QCOMPARE(asIterable.at(0).value<QVariant>().toString(), QStringLiteral("Hello World"));
        auto it1 = asIterable.begin();
        auto it2 = asIterable.begin();
        QCOMPARE((*it1).value<QVariant>().toString(), (*it2).value<QVariant>().toString());
        QCOMPARE((*it1).value<QVariant>().toString(), QStringLiteral("Hello World"));
        ++it2;
        QCOMPARE((*it1).value<QVariant>().toString(), QStringLiteral("Hello World"));
        QCOMPARE((*it2).value<QVariant>().toInt(), 42);
    }
    // tests need to be done after engine has been destroyed, else it will hold a reference until
    // the gc decides to collect it
    QCOMPARE(instanceCount, 0);
}

static QLoggingCategory::CategoryFilter oldFilter;
void logFilter(QLoggingCategory *category)
{
    if (qstrcmp(category->categoryName(), "qt.qml.managedvalue") == 0)
        category->setEnabled(QtDebugMsg, true);
    else if (oldFilter)
        oldFilter(category);
}

void tst_QJSManagedValue::stringAndUrl()
{
    QJSEngine engine;
    const QString string = QStringLiteral("http://example.com/something.html");
    const QUrl url(string);

    const QJSManagedValue urlValue(engine.toManagedValue(url));
    QVERIFY(urlValue.isUrl());
    QCOMPARE(urlValue.toString(), string);
    QCOMPARE(engine.fromManagedValue<QUrl>(urlValue), url);

    const QJSManagedValue stringValue(engine.toManagedValue(string));
    QCOMPARE(stringValue.toString(), string);
    QCOMPARE(engine.fromManagedValue<QUrl>(stringValue), url);

    const QJSManagedValue immediateStringValue(string, &engine);
    QCOMPARE(immediateStringValue.toString(), string);
    QCOMPARE(engine.fromManagedValue<QUrl>(immediateStringValue), url);
}

void tst_QJSManagedValue::jsFunctionInVariant()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);
    QJSManagedValue console(engine.globalObject().property(QStringLiteral("console")), &engine);
    QCOMPARE(console.type(), QJSManagedValue::Object);
    QVERIFY(!console.isNull());
    QJSManagedValue log(console.property(QStringLiteral("log")), &engine);
    QVERIFY(log.isFunction());

    {
        QTest::ignoreMessage(QtDebugMsg, "direct call");
        log.callWithInstance(QJSValue(std::move(console)), {"direct call"});
    }
}

void tst_QJSManagedValue::stringByIndex()
{
    QJSEngine engine;

    const QString testString = QStringLiteral("foobar");
    QJSManagedValue str(testString, &engine);

    for (uint i = 0; i < testString.size(); ++i) {
        QVERIFY(str.hasOwnProperty(i));
        QVERIFY(str.hasProperty(i));

        QVERIFY(str.property(i).strictlyEquals(QJSValue(testString.mid(i, 1))));
        str.setProperty(i, QStringLiteral("u")); // ignored
        QCOMPARE(str.toString(), testString);
    }

    QVERIFY(!str.hasOwnProperty(6));
    QVERIFY(!str.hasProperty(6));
    QVERIFY(!str.hasOwnProperty(16));
    QVERIFY(!str.hasProperty(26));

    QVERIFY(str.property(6).isUndefined());
    QVERIFY(str.property(506).isUndefined());
}

void tst_QJSManagedValue::jsMetaTypes()
{
    QJSEngine engine;
    QJSManagedValue obj(engine.newObject(), &engine);

    QJSManagedValue emptyMetaType = obj.jsMetaType();
    QVERIFY(emptyMetaType.jsMetaMembers().isEmpty());

    QJSManagedValue emptyObj = emptyMetaType.jsMetaInstantiate();
    QVERIFY(emptyObj.isObject());

    obj.setProperty("a", 1);
    obj.setProperty("b", "foo");
    obj.setProperty("llala", true);
    obj.setProperty("ccc", QJSValue(std::move(emptyObj)));

    const QStringList expectedMembers = { "a", "b", "llala", "ccc" };

    QJSManagedValue populatedMetaType = obj.jsMetaType();
    QCOMPARE(populatedMetaType.jsMetaMembers(), expectedMembers);

    QJSManagedValue populatedObj = populatedMetaType.jsMetaInstantiate(
                {"bar", 11, QJSValue(QJSValue::NullValue), 17, "ignored"});
    QVERIFY(populatedObj.isObject());
    QCOMPARE(populatedObj.property("a").toString(), QStringLiteral("bar"));
    QCOMPARE(populatedObj.property("b").toInt(), 11);
    QVERIFY(populatedObj.property("llala").isNull());
    QCOMPARE(populatedObj.property("ccc").toInt(), 17);

    QJSManagedValue halfPopulated = populatedMetaType.jsMetaInstantiate({"one", 111});
    QVERIFY(halfPopulated.isObject());
    QCOMPARE(halfPopulated.property("a").toString(), QStringLiteral("one"));
    QCOMPARE(halfPopulated.property("b").toInt(), 111);
    QVERIFY(halfPopulated.property("llala").isUndefined());
    QVERIFY(halfPopulated.property("ccc").isUndefined());
}

void tst_QJSManagedValue::exceptionsOnNullAccess()
{
    QJSEngine engine;
    QJSManagedValue null(QJSValue(QJSValue::NullValue), &engine);
    QJSManagedValue undef(QJSValue(QJSValue::UndefinedValue), &engine);

    const QString nullReadError = engine.evaluate(
                QStringLiteral("var n = null; n.prop")).toString();
    const QString nullWriteError = engine.evaluate(
                QStringLiteral("var n = null; n.prop = 5")).toString();
    const QString undefReadError = engine.evaluate(
                QStringLiteral("var n; n.prop")).toString();
    const QString undefWriteError = engine.evaluate(
                QStringLiteral("var n; n.prop = 5")).toString();

    QVERIFY(null.property(QStringLiteral("prop")).isUndefined());
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), nullReadError);

    null.setProperty(QStringLiteral("prop"), 5);
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), nullWriteError);

    QVERIFY(undef.property(QStringLiteral("prop")).isUndefined());
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), undefReadError);

    undef.setProperty(QStringLiteral("prop"), 5);
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), undefWriteError);
}

QTEST_MAIN(tst_QJSManagedValue)
