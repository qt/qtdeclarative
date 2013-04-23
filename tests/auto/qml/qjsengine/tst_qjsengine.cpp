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


#include <QtTest/QtTest>

#include <qjsengine.h>
#include <qjsvalueiterator.h>
#include <qgraphicsitem.h>
#include <qstandarditemmodel.h>
#include <QtCore/qnumeric.h>
#include <stdlib.h>

#include <private/v8.h>

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QObjectList)

// The JavaScriptCore GC marks the C stack. To try to ensure that there is
// no JSObject* left in stack memory by the compiler, we call this function
// to zap some bytes of memory before calling collectGarbage().
static void zapSomeStack()
{
    char buf[4096];
    memset(buf, 0, sizeof(buf));
}

static void collectGarbage_helper(QJSEngine &eng)
{
    zapSomeStack();
    eng.collectGarbage();
}

QT_BEGIN_NAMESPACE
extern Q_QML_EXPORT v8::Local<v8::Context> qt_QJSEngineV8Context(QJSEngine *);
extern Q_QML_EXPORT v8::Local<v8::Value> qt_QJSValueV8Value(const QJSValue &);
QT_END_NAMESPACE

class tst_QJSEngine : public QObject
{
    Q_OBJECT

public:
    tst_QJSEngine();
    virtual ~tst_QJSEngine();

private slots:
    void constructWithParent();
    void newObject();
    void newArray();
    void newArray_HooliganTask218092();
    void newArray_HooliganTask233836();
    void newVariant();
    void newVariant_valueOfToString();
    void newRegExp();
    void jsRegExp();
    void newDate();
    void jsParseDate();
    void newQObject();
    void newQObject_ownership();
    void newQObject_deletedEngine();
    void globalObjectProperties();
    void globalObjectEquals();
    void globalObjectProperties_enumerate();
    void createGlobalObjectProperty();
    void globalObjectWithCustomPrototype();
    void builtinFunctionNames_data();
    void builtinFunctionNames();
    void evaluate_data();
    void evaluate();
    void errorMessage_QT679();
    void valueConversion_basic();
    void valueConversion_QVariant();
    void valueConversion_basic2();
    void valueConversion_dateTime();
    void valueConversion_regExp();
    void castWithMultipleInheritance();
    void collectGarbage();
    void gcWithNestedDataStructure();
    void stacktrace();
    void numberParsing_data();
    void numberParsing();
    void automaticSemicolonInsertion();
    void errorConstructors();
    void argumentsProperty_globalContext();
    void argumentsProperty_JS();
    void jsNumberClass();
    void jsForInStatement_simple();
    void jsForInStatement_prototypeProperties();
    void jsForInStatement_mutateWhileIterating();
    void jsForInStatement_arrays();
    void jsForInStatement_nullAndUndefined();
    void jsFunctionDeclarationAsStatement();
    void stringObjects();
    void jsStringPrototypeReplaceBugs();
    void getterSetterThisObject_global();
    void getterSetterThisObject_plain();
    void getterSetterThisObject_prototypeChain();
    void jsContinueInSwitch();
    void jsShadowReadOnlyPrototypeProperty();
    void jsReservedWords_data();
    void jsReservedWords();
    void jsFutureReservedWords_data();
    void jsFutureReservedWords();
    void jsThrowInsideWithStatement();
    void reentrancy_globalObjectProperties();
    void reentrancy_Array();
    void reentrancy_objectCreation();
    void jsIncDecNonObjectProperty();

    void qRegExpInport_data();
    void qRegExpInport();
    void dateRoundtripJSQtJS();
    void dateRoundtripQtJSQt();
    void dateConversionJSQt();
    void dateConversionQtJS();
    void functionPrototypeExtensions();
    void threadedEngine();

    void v8Context_simple();
    void v8Context_exception();
    void v8Context_mixAPIs();
};

tst_QJSEngine::tst_QJSEngine()
{
}

tst_QJSEngine::~tst_QJSEngine()
{
}

void tst_QJSEngine::constructWithParent()
{
    QPointer<QJSEngine> ptr;
    {
        QObject obj;
        QJSEngine *engine = new QJSEngine(&obj);
        ptr = engine;
    }
    QVERIFY(ptr == 0);
}

void tst_QJSEngine::newObject()
{
    QJSEngine eng;
    QJSValue object = eng.newObject();
    QVERIFY(!object.isUndefined());
    QCOMPARE(object.isObject(), true);
    QCOMPARE(object.isCallable(), false);
    // prototype should be Object.prototype
    QVERIFY(!object.prototype().isUndefined());
    QCOMPARE(object.prototype().isObject(), true);
    QCOMPARE(object.prototype().strictlyEquals(eng.evaluate("Object.prototype")), true);
}

void tst_QJSEngine::newArray()
{
    QJSEngine eng;
    QJSValue array = eng.newArray();
    QVERIFY(!array.isUndefined());
    QCOMPARE(array.isArray(), true);
    QCOMPARE(array.isObject(), true);
    QVERIFY(!array.isCallable());
    // prototype should be Array.prototype
    QVERIFY(!array.prototype().isUndefined());
    QCOMPARE(array.prototype().isArray(), true);
    QCOMPARE(array.prototype().strictlyEquals(eng.evaluate("Array.prototype")), true);
}

void tst_QJSEngine::newArray_HooliganTask218092()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("[].splice(0, 0, 'a')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 0);
    }
    {
        QJSValue ret = eng.evaluate("['a'].splice(0, 1, 'b')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 1);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b'].splice(0, 1, 'c')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 1);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b', 'c'].splice(0, 2, 'd')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 2);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b', 'c'].splice(1, 2, 'd', 'e', 'f')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 2);
    }
}

void tst_QJSEngine::newArray_HooliganTask233836()
{
    QJSEngine eng;
    {
        // According to ECMA-262, this should cause a RangeError.
        QJSValue ret = eng.evaluate("a = new Array(4294967295); a.push('foo')");
        QVERIFY(ret.isError() && ret.toString().contains(QLatin1String("RangeError")));
    }
    {
        QJSValue ret = eng.newArray(0xFFFFFFFF);
        QEXPECT_FAIL("", "The maximum length of arrays is defined by v8 currently and differs from Qt Script", Abort);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        ret.setProperty(0xFFFFFFFF, 123);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        QVERIFY(ret.property(0xFFFFFFFF).isNumber());
        QCOMPARE(ret.property(0xFFFFFFFF).toInt(), 123);
        ret.setProperty(123, 456);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        QVERIFY(ret.property(123).isNumber());
        QCOMPARE(ret.property(123).toInt(), 456);
    }
}

void tst_QJSEngine::newVariant()
{
    QJSEngine eng;
    {
        QJSValue opaque = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QVERIFY(!opaque.isUndefined());
        QCOMPARE(opaque.isVariant(), true);
        QVERIFY(!opaque.isCallable());
        QCOMPARE(opaque.isObject(), true);
        QVERIFY(!opaque.prototype().isUndefined());
        QEXPECT_FAIL("", "FIXME: newly created QObject's prototype is an JS Object", Continue);
        QCOMPARE(opaque.prototype().isVariant(), true);
        QVERIFY(opaque.property("valueOf").callWithInstance(opaque).equals(opaque));
    }
}

void tst_QJSEngine::newVariant_valueOfToString()
{
    // valueOf() and toString()
    QJSEngine eng;
    {
        QJSValue object = eng.toScriptValue(QVariant(QPoint(10, 20)));
        QJSValue value = object.property("valueOf").callWithInstance(object);
        QVERIFY(value.isObject());
        QVERIFY(value.strictlyEquals(object));
        QCOMPARE(object.toString(), QString::fromLatin1("QVariant(QPoint)"));
    }
}

void tst_QJSEngine::newRegExp()
{
    QJSEngine eng;
    QJSValue rexp = eng.toScriptValue(QRegExp("foo"));
    QVERIFY(!rexp.isUndefined());
    QCOMPARE(rexp.isRegExp(), true);
    QCOMPARE(rexp.isObject(), true);
    QCOMPARE(rexp.isCallable(), false);
    // prototype should be RegExp.prototype
    QVERIFY(!rexp.prototype().isUndefined());
    QCOMPARE(rexp.prototype().isObject(), true);
    QCOMPARE(rexp.prototype().isRegExp(), true);
    // Get [[Class]] internal property of RegExp Prototype Object.
    // See ECMA-262 Section 8.6.2, "Object Internal Properties and Methods".
    // See ECMA-262 Section 15.10.6, "Properties of the RegExp Prototype Object".
    QJSValue r = eng.evaluate("Object.prototype.toString.call(RegExp.prototype)");
    QCOMPARE(r.toString(), QString::fromLatin1("[object RegExp]"));
    QCOMPARE(rexp.prototype().strictlyEquals(eng.evaluate("RegExp.prototype")), true);

    QCOMPARE(qjsvalue_cast<QRegExp>(rexp).pattern(), QRegExp("foo").pattern());
}

void tst_QJSEngine::jsRegExp()
{
    // See ECMA-262 Section 15.10, "RegExp Objects".
    // These should really be JS-only tests, as they test the implementation's
    // ECMA-compliance, not the C++ API. Compliance should already be covered
    // by the Mozilla tests (qscriptjstestsuite).
    // We can consider updating the expected results of this test if the
    // RegExp implementation changes.

    QJSEngine eng;
    QJSValue r = eng.evaluate("/foo/gim");
    QVERIFY(r.isRegExp());
    QCOMPARE(r.toString(), QString::fromLatin1("/foo/gim"));

    QJSValue rxCtor = eng.globalObject().property("RegExp");
    QJSValue r2 = rxCtor.call(QJSValueList() << r);
    QVERIFY(r2.isRegExp());
    QVERIFY(r2.strictlyEquals(r));

    QJSValue r3 = rxCtor.call(QJSValueList() << r << "gim");
    QVERIFY(r3.isError());
    QVERIFY(r3.toString().contains(QString::fromLatin1("TypeError"))); // Cannot supply flags when constructing one RegExp from another

    QJSValue r4 = rxCtor.call(QJSValueList() << "foo" << "gim");
    QVERIFY(r4.isRegExp());

    QJSValue r5 = rxCtor.callAsConstructor(QJSValueList() << r);
    QVERIFY(r5.isRegExp());
    QCOMPARE(r5.toString(), QString::fromLatin1("/foo/gim"));
    // In JSC, constructing a RegExp from another produces the same identical object.
    // This is different from SpiderMonkey and old back-end.
    QVERIFY(!r5.strictlyEquals(r));

    // See ECMA-262 Section 15.10.4.1, "new RegExp(pattern, flags)".
    QJSValue r6 = rxCtor.callAsConstructor(QJSValueList() << "foo" << "bar");
    QVERIFY(r6.isError());
    QVERIFY(r6.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r7 = eng.evaluate("/foo/gimp");
    QVERIFY(r7.isError());
    QVERIFY(r7.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r8 = eng.evaluate("/foo/migmigmig");
    QVERIFY(r8.isError());
    QVERIFY(r8.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r9 = rxCtor.callAsConstructor();
    QVERIFY(r9.isRegExp());
    QCOMPARE(r9.toString(), QString::fromLatin1("/(?:)/"));

    QJSValue r10 = rxCtor.callAsConstructor(QJSValueList() << "" << "gim");
    QVERIFY(r10.isRegExp());
    QCOMPARE(r10.toString(), QString::fromLatin1("/(?:)/gim"));

    QJSValue r11 = rxCtor.callAsConstructor(QJSValueList() << "{1.*}" << "g");
    QVERIFY(r11.isRegExp());
    QCOMPARE(r11.toString(), QString::fromLatin1("/{1.*}/g"));
}

void tst_QJSEngine::newDate()
{
    QJSEngine eng;

    {
        QJSValue date = eng.evaluate("new Date(0)");
        QVERIFY(!date.isUndefined());
        QCOMPARE(date.isDate(), true);
        QCOMPARE(date.isObject(), true);
        QVERIFY(!date.isCallable());
        // prototype should be Date.prototype
        QVERIFY(!date.prototype().isUndefined());
        QCOMPARE(date.prototype().isDate(), true);
        QCOMPARE(date.prototype().strictlyEquals(eng.evaluate("Date.prototype")), true);
    }

    {
        QDateTime dt = QDateTime(QDate(1, 2, 3), QTime(4, 5, 6, 7), Qt::LocalTime);
        QJSValue date = eng.toScriptValue(dt);
        QVERIFY(!date.isUndefined());
        QCOMPARE(date.isDate(), true);
        QCOMPARE(date.isObject(), true);
        // prototype should be Date.prototype
        QVERIFY(!date.prototype().isUndefined());
        QCOMPARE(date.prototype().isDate(), true);
        QCOMPARE(date.prototype().strictlyEquals(eng.evaluate("Date.prototype")), true);

        QCOMPARE(date.toDateTime(), dt);
    }

    {
        QDateTime dt = QDateTime(QDate(1, 2, 3), QTime(4, 5, 6, 7), Qt::UTC);
        QJSValue date = eng.toScriptValue(dt);
        // toDateTime() result should be in local time
        QCOMPARE(date.toDateTime(), dt.toLocalTime());
    }
}

void tst_QJSEngine::jsParseDate()
{
    QJSEngine eng;
    // Date.parse() should return NaN when it fails
    {
        QJSValue ret = eng.evaluate("Date.parse()");
        QVERIFY(ret.isNumber());
        QVERIFY(qIsNaN(ret.toNumber()));
    }

    // Date.parse() should be able to parse the output of Date().toString()
    {
        QJSValue ret = eng.evaluate("var x = new Date(); var s = x.toString(); s == new Date(Date.parse(s)).toString()");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), true);
    }
}

void tst_QJSEngine::newQObject()
{
    QJSEngine eng;
    QObject temp;

    {
        QJSValue qobject = eng.newQObject(0);
        QCOMPARE(qobject.isNull(), true);
        QCOMPARE(qobject.isObject(), false);
        QCOMPARE(qobject.toQObject(), (QObject *)0);
    }
    {
        QJSValue qobject = eng.newQObject(&temp);
        QVERIFY(!qobject.isUndefined());
        QCOMPARE(qobject.isQObject(), true);
        QCOMPARE(qobject.isObject(), true);
        QCOMPARE(qobject.toQObject(), (QObject *)&temp);
        QVERIFY(!qobject.isCallable());
        // prototype should be QObject.prototype
        QCOMPARE(qobject.prototype().isObject(), true);
        QEXPECT_FAIL("", "FIXME: newly created QObject's prototype is an JS Object", Continue);
        QCOMPARE(qobject.prototype().isQObject(), true);
    }
}

void tst_QJSEngine::newQObject_ownership()
{
    QJSEngine eng;
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr != 0);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        collectGarbage_helper(eng);
        if (ptr)
            QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
        QVERIFY(ptr == 0);
    }
    {
        QPointer<QObject> ptr = new QObject(this);
        QVERIFY(ptr != 0);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        QObject *before = ptr;
        collectGarbage_helper(eng);
        QVERIFY(ptr == before);
        delete ptr;
    }
    {
        QObject *parent = new QObject();
        QObject *child = new QObject(parent);
        QJSValue v = eng.newQObject(child);
        QCOMPARE(v.toQObject(), child);
        delete parent;
        QCOMPARE(v.toQObject(), (QObject *)0);
    }
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr != 0);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        collectGarbage_helper(eng);
        // no parent, so it should be like ScriptOwnership
        if (ptr)
            QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
        QVERIFY(ptr == 0);
    }
    {
        QObject *parent = new QObject();
        QPointer<QObject> child = new QObject(parent);
        QVERIFY(child != 0);
        {
            QJSValue v = eng.newQObject(child);
        }
        collectGarbage_helper(eng);
        // has parent, so it should be like QtOwnership
        QVERIFY(child != 0);
        delete parent;
    }
}

void tst_QJSEngine::newQObject_deletedEngine()
{
    QJSValue object;
    QObject *ptr = new QObject();
    QSignalSpy spy(ptr, SIGNAL(destroyed()));
    {
        QJSEngine engine;
        object = engine.newQObject(ptr);
        engine.globalObject().setProperty("obj", object);
    }
    QTRY_VERIFY(spy.count());
}

void tst_QJSEngine::globalObjectProperties()
{
    // See ECMA-262 Section 15.1, "The Global Object".

    QJSEngine eng;
    QJSValue global = eng.globalObject();

    QVERIFY(global.property("NaN").isNumber());
    QVERIFY(qIsNaN(global.property("NaN").toNumber()));

    QVERIFY(global.property("Infinity").isNumber());
    QVERIFY(qIsInf(global.property("Infinity").toNumber()));

    QVERIFY(global.property("undefined").isUndefined());

    QVERIFY(global.property("eval").isCallable());

    QVERIFY(global.property("parseInt").isCallable());

    QVERIFY(global.property("parseFloat").isCallable());

    QVERIFY(global.property("isNaN").isCallable());

    QVERIFY(global.property("isFinite").isCallable());

    QVERIFY(global.property("decodeURI").isCallable());

    QVERIFY(global.property("decodeURIComponent").isCallable());

    QVERIFY(global.property("encodeURI").isCallable());

    QVERIFY(global.property("encodeURIComponent").isCallable());

    QVERIFY(global.property("Object").isCallable());
    QVERIFY(global.property("Function").isCallable());
    QVERIFY(global.property("Array").isCallable());
    QVERIFY(global.property("String").isCallable());
    QVERIFY(global.property("Boolean").isCallable());
    QVERIFY(global.property("Number").isCallable());
    QVERIFY(global.property("Date").isCallable());
    QVERIFY(global.property("RegExp").isCallable());
    QVERIFY(global.property("Error").isCallable());
    QVERIFY(global.property("EvalError").isCallable());
    QVERIFY(global.property("RangeError").isCallable());
    QVERIFY(global.property("ReferenceError").isCallable());
    QVERIFY(global.property("SyntaxError").isCallable());
    QVERIFY(global.property("TypeError").isCallable());
    QVERIFY(global.property("URIError").isCallable());
    QVERIFY(global.property("Math").isObject());
    QVERIFY(!global.property("Math").isCallable());
}

void tst_QJSEngine::globalObjectEquals()
{
    QJSEngine eng;
    QJSValue o = eng.globalObject();
    QVERIFY(o.strictlyEquals(eng.globalObject()));
    QVERIFY(o.equals(eng.globalObject()));
}

void tst_QJSEngine::globalObjectProperties_enumerate()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();

    QSet<QString> expectedNames;
    expectedNames
        << "isNaN"
        << "parseFloat"
        << "String"
        << "EvalError"
        << "URIError"
        << "Math"
        << "encodeURIComponent"
        << "RangeError"
        << "eval"
        << "isFinite"
        << "ReferenceError"
        << "Infinity"
        << "Function"
        << "RegExp"
        << "Number"
        << "parseInt"
        << "Object"
        << "decodeURI"
        << "TypeError"
        << "Boolean"
        << "encodeURI"
        << "NaN"
        << "Error"
        << "decodeURIComponent"
        << "Date"
        << "Array"
        << "escape"
        << "unescape"
        << "SyntaxError"
        << "undefined"
        // JavaScriptCore
        << "JSON"
        ;
    QSet<QString> actualNames;
    {
        QJSValueIterator it(global);
        while (it.hasNext()) {
            it.next();
            actualNames.insert(it.name());
        }
    }

    QSet<QString> remainingNames = actualNames;
    {
        QSet<QString>::const_iterator it;
        for (it = expectedNames.constBegin(); it != expectedNames.constEnd(); ++it) {
            QString name = *it;
            QVERIFY(actualNames.contains(name));
            remainingNames.remove(name);
        }
    }
    QVERIFY(remainingNames.isEmpty());
}

void tst_QJSEngine::createGlobalObjectProperty()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();
    // create property with no attributes
    {
        QString name = QString::fromLatin1("foo");
        QVERIFY(global.property(name).isUndefined());
        QJSValue val(123);
        global.setProperty(name, val);
        QVERIFY(global.property(name).equals(val));
        global.deleteProperty(name);
        QVERIFY(global.property(name).isUndefined());
    }
}

void tst_QJSEngine::globalObjectWithCustomPrototype()
{
    QJSEngine engine;
    QJSValue proto = engine.newObject();
    proto.setProperty("protoProperty", 123);
    QJSValue global = engine.globalObject();
    QJSValue originalProto = global.prototype();
    global.setPrototype(proto);
    {
        QJSValue ret = engine.evaluate("protoProperty");
        QEXPECT_FAIL("", "Replacing the prototype of the global object is currently unsupported (see also v8 issue 1078)", Abort);
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("protoProperty")));
    }
    {
        QJSValue ret = engine.evaluate("this.protoProperty");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("protoProperty")));
    }
    {
        QJSValue ret = engine.evaluate("hasOwnProperty('protoProperty')");
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
    }
    {
        QJSValue ret = engine.evaluate("this.hasOwnProperty('protoProperty')");
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
    }

    // Custom prototype set from JS
    {
        QJSValue ret = engine.evaluate("this.__proto__ = { 'a': 123 }; a");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("a")));
    }
}

void tst_QJSEngine::builtinFunctionNames_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedName");

    // See ECMA-262 Chapter 15, "Standard Built-in ECMAScript Objects".

    QTest::newRow("parseInt") << QString("parseInt") << QString("parseInt");
    QTest::newRow("parseFloat") << QString("parseFloat") << QString("parseFloat");
    QTest::newRow("isNaN") << QString("isNaN") << QString("isNaN");
    QTest::newRow("isFinite") << QString("isFinite") << QString("isFinite");
    QTest::newRow("decodeURI") << QString("decodeURI") << QString("decodeURI");
    QTest::newRow("decodeURIComponent") << QString("decodeURIComponent") << QString("decodeURIComponent");
    QTest::newRow("encodeURI") << QString("encodeURI") << QString("encodeURI");
    QTest::newRow("encodeURIComponent") << QString("encodeURIComponent") << QString("encodeURIComponent");
    QTest::newRow("escape") << QString("escape") << QString("escape");
    QTest::newRow("unescape") << QString("unescape") << QString("unescape");

    QTest::newRow("Array") << QString("Array") << QString("Array");
    QTest::newRow("Array.prototype.toString") << QString("Array.prototype.toString") << QString("toString");
    QTest::newRow("Array.prototype.toLocaleString") << QString("Array.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Array.prototype.concat") << QString("Array.prototype.concat") << QString("concat");
    QTest::newRow("Array.prototype.join") << QString("Array.prototype.join") << QString("join");
    QTest::newRow("Array.prototype.pop") << QString("Array.prototype.pop") << QString("pop");
    QTest::newRow("Array.prototype.push") << QString("Array.prototype.push") << QString("push");
    QTest::newRow("Array.prototype.reverse") << QString("Array.prototype.reverse") << QString("reverse");
    QTest::newRow("Array.prototype.shift") << QString("Array.prototype.shift") << QString("shift");
    QTest::newRow("Array.prototype.slice") << QString("Array.prototype.slice") << QString("slice");
    QTest::newRow("Array.prototype.sort") << QString("Array.prototype.sort") << QString("sort");
    QTest::newRow("Array.prototype.splice") << QString("Array.prototype.splice") << QString("splice");
    QTest::newRow("Array.prototype.unshift") << QString("Array.prototype.unshift") << QString("unshift");

    QTest::newRow("Boolean") << QString("Boolean") << QString("Boolean");
    QTest::newRow("Boolean.prototype.toString") << QString("Boolean.prototype.toString") << QString("toString");

    QTest::newRow("Date") << QString("Date") << QString("Date");
    QTest::newRow("Date.prototype.toString") << QString("Date.prototype.toString") << QString("toString");
    QTest::newRow("Date.prototype.toDateString") << QString("Date.prototype.toDateString") << QString("toDateString");
    QTest::newRow("Date.prototype.toTimeString") << QString("Date.prototype.toTimeString") << QString("toTimeString");
    QTest::newRow("Date.prototype.toLocaleString") << QString("Date.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Date.prototype.toLocaleDateString") << QString("Date.prototype.toLocaleDateString") << QString("toLocaleDateString");
    QTest::newRow("Date.prototype.toLocaleTimeString") << QString("Date.prototype.toLocaleTimeString") << QString("toLocaleTimeString");
    QTest::newRow("Date.prototype.valueOf") << QString("Date.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Date.prototype.getTime") << QString("Date.prototype.getTime") << QString("getTime");
    QTest::newRow("Date.prototype.getYear") << QString("Date.prototype.getYear") << QString("getYear");
    QTest::newRow("Date.prototype.getFullYear") << QString("Date.prototype.getFullYear") << QString("getFullYear");
    QTest::newRow("Date.prototype.getUTCFullYear") << QString("Date.prototype.getUTCFullYear") << QString("getUTCFullYear");
    QTest::newRow("Date.prototype.getMonth") << QString("Date.prototype.getMonth") << QString("getMonth");
    QTest::newRow("Date.prototype.getUTCMonth") << QString("Date.prototype.getUTCMonth") << QString("getUTCMonth");
    QTest::newRow("Date.prototype.getDate") << QString("Date.prototype.getDate") << QString("getDate");
    QTest::newRow("Date.prototype.getUTCDate") << QString("Date.prototype.getUTCDate") << QString("getUTCDate");
    QTest::newRow("Date.prototype.getDay") << QString("Date.prototype.getDay") << QString("getDay");
    QTest::newRow("Date.prototype.getUTCDay") << QString("Date.prototype.getUTCDay") << QString("getUTCDay");
    QTest::newRow("Date.prototype.getHours") << QString("Date.prototype.getHours") << QString("getHours");
    QTest::newRow("Date.prototype.getUTCHours") << QString("Date.prototype.getUTCHours") << QString("getUTCHours");
    QTest::newRow("Date.prototype.getMinutes") << QString("Date.prototype.getMinutes") << QString("getMinutes");
    QTest::newRow("Date.prototype.getUTCMinutes") << QString("Date.prototype.getUTCMinutes") << QString("getUTCMinutes");
    QTest::newRow("Date.prototype.getSeconds") << QString("Date.prototype.getSeconds") << QString("getSeconds");
    QTest::newRow("Date.prototype.getUTCSeconds") << QString("Date.prototype.getUTCSeconds") << QString("getUTCSeconds");
    QTest::newRow("Date.prototype.getMilliseconds") << QString("Date.prototype.getMilliseconds") << QString("getMilliseconds");
    QTest::newRow("Date.prototype.getUTCMilliseconds") << QString("Date.prototype.getUTCMilliseconds") << QString("getUTCMilliseconds");
    QTest::newRow("Date.prototype.getTimezoneOffset") << QString("Date.prototype.getTimezoneOffset") << QString("getTimezoneOffset");
    QTest::newRow("Date.prototype.setTime") << QString("Date.prototype.setTime") << QString("setTime");
    QTest::newRow("Date.prototype.setMilliseconds") << QString("Date.prototype.setMilliseconds") << QString("setMilliseconds");
    QTest::newRow("Date.prototype.setUTCMilliseconds") << QString("Date.prototype.setUTCMilliseconds") << QString("setUTCMilliseconds");
    QTest::newRow("Date.prototype.setSeconds") << QString("Date.prototype.setSeconds") << QString("setSeconds");
    QTest::newRow("Date.prototype.setUTCSeconds") << QString("Date.prototype.setUTCSeconds") << QString("setUTCSeconds");
    QTest::newRow("Date.prototype.setMinutes") << QString("Date.prototype.setMinutes") << QString("setMinutes");
    QTest::newRow("Date.prototype.setUTCMinutes") << QString("Date.prototype.setUTCMinutes") << QString("setUTCMinutes");
    QTest::newRow("Date.prototype.setHours") << QString("Date.prototype.setHours") << QString("setHours");
    QTest::newRow("Date.prototype.setUTCHours") << QString("Date.prototype.setUTCHours") << QString("setUTCHours");
    QTest::newRow("Date.prototype.setDate") << QString("Date.prototype.setDate") << QString("setDate");
    QTest::newRow("Date.prototype.setUTCDate") << QString("Date.prototype.setUTCDate") << QString("setUTCDate");
    QTest::newRow("Date.prototype.setMonth") << QString("Date.prototype.setMonth") << QString("setMonth");
    QTest::newRow("Date.prototype.setUTCMonth") << QString("Date.prototype.setUTCMonth") << QString("setUTCMonth");
    QTest::newRow("Date.prototype.setYear") << QString("Date.prototype.setYear") << QString("setYear");
    QTest::newRow("Date.prototype.setFullYear") << QString("Date.prototype.setFullYear") << QString("setFullYear");
    QTest::newRow("Date.prototype.setUTCFullYear") << QString("Date.prototype.setUTCFullYear") << QString("setUTCFullYear");
    QTest::newRow("Date.prototype.toUTCString") << QString("Date.prototype.toUTCString") << QString("toUTCString");
    QTest::newRow("Date.prototype.toGMTString") << QString("Date.prototype.toGMTString") << QString("toGMTString");

    QTest::newRow("Error") << QString("Error") << QString("Error");
//    QTest::newRow("Error.prototype.backtrace") << QString("Error.prototype.backtrace") << QString("backtrace");
    QTest::newRow("Error.prototype.toString") << QString("Error.prototype.toString") << QString("toString");

    QTest::newRow("EvalError") << QString("EvalError") << QString("EvalError");
    QTest::newRow("RangeError") << QString("RangeError") << QString("RangeError");
    QTest::newRow("ReferenceError") << QString("ReferenceError") << QString("ReferenceError");
    QTest::newRow("SyntaxError") << QString("SyntaxError") << QString("SyntaxError");
    QTest::newRow("TypeError") << QString("TypeError") << QString("TypeError");
    QTest::newRow("URIError") << QString("URIError") << QString("URIError");

    QTest::newRow("Function") << QString("Function") << QString("Function");
    QTest::newRow("Function.prototype.toString") << QString("Function.prototype.toString") << QString("toString");
    QTest::newRow("Function.prototype.apply") << QString("Function.prototype.apply") << QString("apply");
    QTest::newRow("Function.prototype.call") << QString("Function.prototype.call") << QString("call");
/*  In V8, those function are only there for signals
    QTest::newRow("Function.prototype.connect") << QString("Function.prototype.connect") << QString("connect");
    QTest::newRow("Function.prototype.disconnect") << QString("Function.prototype.disconnect") << QString("disconnect");*/

    QTest::newRow("Math.abs") << QString("Math.abs") << QString("abs");
    QTest::newRow("Math.acos") << QString("Math.acos") << QString("acos");
    QTest::newRow("Math.asin") << QString("Math.asin") << QString("asin");
    QTest::newRow("Math.atan") << QString("Math.atan") << QString("atan");
    QTest::newRow("Math.atan2") << QString("Math.atan2") << QString("atan2");
    QTest::newRow("Math.ceil") << QString("Math.ceil") << QString("ceil");
    QTest::newRow("Math.cos") << QString("Math.cos") << QString("cos");
    QTest::newRow("Math.exp") << QString("Math.exp") << QString("exp");
    QTest::newRow("Math.floor") << QString("Math.floor") << QString("floor");
    QTest::newRow("Math.log") << QString("Math.log") << QString("log");
    QTest::newRow("Math.max") << QString("Math.max") << QString("max");
    QTest::newRow("Math.min") << QString("Math.min") << QString("min");
    QTest::newRow("Math.pow") << QString("Math.pow") << QString("pow");
    QTest::newRow("Math.random") << QString("Math.random") << QString("random");
    QTest::newRow("Math.round") << QString("Math.round") << QString("round");
    QTest::newRow("Math.sin") << QString("Math.sin") << QString("sin");
    QTest::newRow("Math.sqrt") << QString("Math.sqrt") << QString("sqrt");
    QTest::newRow("Math.tan") << QString("Math.tan") << QString("tan");

    QTest::newRow("Number") << QString("Number") << QString("Number");
    QTest::newRow("Number.prototype.toString") << QString("Number.prototype.toString") << QString("toString");
    QTest::newRow("Number.prototype.toLocaleString") << QString("Number.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Number.prototype.valueOf") << QString("Number.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Number.prototype.toFixed") << QString("Number.prototype.toFixed") << QString("toFixed");
    QTest::newRow("Number.prototype.toExponential") << QString("Number.prototype.toExponential") << QString("toExponential");
    QTest::newRow("Number.prototype.toPrecision") << QString("Number.prototype.toPrecision") << QString("toPrecision");

    QTest::newRow("Object") << QString("Object") << QString("Object");
    QTest::newRow("Object.prototype.toString") << QString("Object.prototype.toString") << QString("toString");
    QTest::newRow("Object.prototype.toLocaleString") << QString("Object.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Object.prototype.valueOf") << QString("Object.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Object.prototype.hasOwnProperty") << QString("Object.prototype.hasOwnProperty") << QString("hasOwnProperty");
    QTest::newRow("Object.prototype.isPrototypeOf") << QString("Object.prototype.isPrototypeOf") << QString("isPrototypeOf");
    QTest::newRow("Object.prototype.propertyIsEnumerable") << QString("Object.prototype.propertyIsEnumerable") << QString("propertyIsEnumerable");
    QTest::newRow("Object.prototype.__defineGetter__") << QString("Object.prototype.__defineGetter__") << QString("__defineGetter__");
    QTest::newRow("Object.prototype.__defineSetter__") << QString("Object.prototype.__defineSetter__") << QString("__defineSetter__");

    QTest::newRow("RegExp") << QString("RegExp") << QString("RegExp");
    QTest::newRow("RegExp.prototype.exec") << QString("RegExp.prototype.exec") << QString("exec");
    QTest::newRow("RegExp.prototype.test") << QString("RegExp.prototype.test") << QString("test");
    QTest::newRow("RegExp.prototype.toString") << QString("RegExp.prototype.toString") << QString("toString");

    QTest::newRow("String") << QString("String") << QString("String");
    QTest::newRow("String.prototype.toString") << QString("String.prototype.toString") << QString("toString");
    QTest::newRow("String.prototype.valueOf") << QString("String.prototype.valueOf") << QString("valueOf");
    QTest::newRow("String.prototype.charAt") << QString("String.prototype.charAt") << QString("charAt");
    QTest::newRow("String.prototype.charCodeAt") << QString("String.prototype.charCodeAt") << QString("charCodeAt");
    QTest::newRow("String.prototype.concat") << QString("String.prototype.concat") << QString("concat");
    QTest::newRow("String.prototype.indexOf") << QString("String.prototype.indexOf") << QString("indexOf");
    QTest::newRow("String.prototype.lastIndexOf") << QString("String.prototype.lastIndexOf") << QString("lastIndexOf");
    QTest::newRow("String.prototype.localeCompare") << QString("String.prototype.localeCompare") << QString("localeCompare");
    QTest::newRow("String.prototype.match") << QString("String.prototype.match") << QString("match");
    QTest::newRow("String.prototype.replace") << QString("String.prototype.replace") << QString("replace");
    QTest::newRow("String.prototype.search") << QString("String.prototype.search") << QString("search");
    QTest::newRow("String.prototype.slice") << QString("String.prototype.slice") << QString("slice");
    QTest::newRow("String.prototype.split") << QString("String.prototype.split") << QString("split");
    QTest::newRow("String.prototype.substring") << QString("String.prototype.substring") << QString("substring");
    QTest::newRow("String.prototype.toLowerCase") << QString("String.prototype.toLowerCase") << QString("toLowerCase");
    QTest::newRow("String.prototype.toLocaleLowerCase") << QString("String.prototype.toLocaleLowerCase") << QString("toLocaleLowerCase");
    QTest::newRow("String.prototype.toUpperCase") << QString("String.prototype.toUpperCase") << QString("toUpperCase");
    QTest::newRow("String.prototype.toLocaleUpperCase") << QString("String.prototype.toLocaleUpperCase") << QString("toLocaleUpperCase");
}

void tst_QJSEngine::builtinFunctionNames()
{
    QFETCH(QString, expression);
    QFETCH(QString, expectedName);
    QJSEngine eng;
    // The "name" property is actually non-standard, but JSC supports it.
    QJSValue ret = eng.evaluate(QString::fromLatin1("%0.name").arg(expression));
    QVERIFY(ret.isString());
    QCOMPARE(ret.toString(), expectedName);
}

void tst_QJSEngine::evaluate_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("lineNumber");
    QTest::addColumn<bool>("expectHadError");
    QTest::addColumn<int>("expectErrorLineNumber");

    QTest::newRow("(newline)") << QString("\n") << -1 << false << -1;
    QTest::newRow("0 //")   << QString("0 //") << -1 << false << -1;
    QTest::newRow("/* */")   << QString("/* */") << -1 << false << -1;
    QTest::newRow("//") << QString("//") << -1 << false << -1;
    QTest::newRow("(spaces)")  << QString("  ") << -1 << false << -1;
    QTest::newRow("(empty)")   << QString("") << -1 << false << -1;
    QTest::newRow("0")     << QString("0")       << -1 << false << -1;
    QTest::newRow("0=1")   << QString("\n0=1;\n") << -1 << true  << 2;
    QTest::newRow("a=1")   << QString("a=1\n")   << -1 << false << -1;
    QTest::newRow("a=1;K") << QString("a=1;\nK") << -1 << true  << 2;

    QTest::newRow("f()") << QString("function f()\n"
                                    "{\n"
                                    "  var a;\n"
                                    "  var b=\";\n" // here's the error
                                    "}\n"
                                    "f();\n")
                         << -1 << true << 4;

    QTest::newRow("0")     << QString("0")       << 10 << false << -1;
    QTest::newRow("0=1")   << QString("\n\n0=1\n") << 10 << true  << 12;
    QTest::newRow("a=1")   << QString("a=1\n")   << 10 << false << -1;
    QTest::newRow("a=1;K") << QString("a=1;\n\nK") << 10 << true  << 12;

    QTest::newRow("f()") << QString("function f()\n"
                                    "{\n"
                                    "  var a;\n"
                                    "\n\n"
                                    "  var b=\";\n" // here's the error
                                    "}\n"
                                    "f();\n")
                         << 10 << true << 15;
    QTest::newRow("functionThatDoesntExist()")
        << QString(";\n;\n;\nfunctionThatDoesntExist()")
        << -1 << true << 4;
    QTest::newRow("for (var p in this) { continue labelThatDoesntExist; }")
        << QString("for (var p in this) {\ncontinue labelThatDoesntExist; }")
        << 4 << true << 5;
    QTest::newRow("duplicateLabel: { duplicateLabel: ; }")
        << QString("duplicateLabel: { duplicateLabel: ; }")
        << 12 << true << 12;

    QTest::newRow("/=/") << QString("/=/") << -1 << false << -1;
    QTest::newRow("/=/g") << QString("/=/g") << -1 << false << -1;
    QTest::newRow("/a/") << QString("/a/") << -1 << false << -1;
    QTest::newRow("/a/g") << QString("/a/g") << -1 << false << -1;
    QTest::newRow("/a/gim") << QString("/a/gim") << -1 << false << -1;
    QTest::newRow("/a/gimp") << QString("/a/gimp") << 1 << true << 1;
}

void tst_QJSEngine::evaluate()
{
    QFETCH(QString, code);
    QFETCH(int, lineNumber);
    QFETCH(bool, expectHadError);
    QFETCH(int, expectErrorLineNumber);

    QJSEngine eng;
    QJSValue ret;
    if (lineNumber != -1)
        ret = eng.evaluate(code, /*fileName =*/QString(), lineNumber);
    else
        ret = eng.evaluate(code);
    QCOMPARE(ret.isError(), expectHadError);
    if (ret.isError()) {
        QEXPECT_FAIL("", "we have no more lineNumber property ", Continue);
        QVERIFY(ret.property("lineNumber").strictlyEquals(eng.toScriptValue(expectErrorLineNumber)));
    }
}

void tst_QJSEngine::errorMessage_QT679()
{
    QJSEngine engine;
    engine.globalObject().setProperty("foo", 15);
    QJSValue error = engine.evaluate("'hello world';\nfoo.bar.blah");
    QVERIFY(error.isError());
    QVERIFY(error.toString().startsWith(QString::fromLatin1("TypeError: ")));
}

struct Foo {
public:
    int x, y;
    Foo() : x(-1), y(-1) { }
};

Q_DECLARE_METATYPE(Foo)
Q_DECLARE_METATYPE(Foo*)

Q_DECLARE_METATYPE(QLinkedList<QString>)
Q_DECLARE_METATYPE(QList<Foo>)
Q_DECLARE_METATYPE(QVector<QChar>)
Q_DECLARE_METATYPE(QStack<int>)
Q_DECLARE_METATYPE(QQueue<char>)
Q_DECLARE_METATYPE(QLinkedList<QStack<int> >)

void tst_QJSEngine::valueConversion_basic()
{
    QJSEngine eng;
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(num.isNumber(), true);
        QCOMPARE(num.strictlyEquals(eng.toScriptValue(123)), true);

        int inum = eng.fromScriptValue<int>(num);
        QCOMPARE(inum, 123);

        QString snum = eng.fromScriptValue<QString>(num);
        QCOMPARE(snum, QLatin1String("123"));
    }
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(num.isNumber(), true);
        QCOMPARE(num.strictlyEquals(eng.toScriptValue(123)), true);

        int inum = eng.fromScriptValue<int>(num);
        QCOMPARE(inum, 123);

        QString snum = eng.fromScriptValue<QString>(num);
        QCOMPARE(snum, QLatin1String("123"));
    }
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(eng.fromScriptValue<char>(num), char(123));
        QCOMPARE(eng.fromScriptValue<unsigned char>(num), (unsigned char)(123));
        QCOMPARE(eng.fromScriptValue<short>(num), short(123));
        QCOMPARE(eng.fromScriptValue<unsigned short>(num), (unsigned short)(123));
        QCOMPARE(eng.fromScriptValue<float>(num), float(123));
        QCOMPARE(eng.fromScriptValue<double>(num), double(123));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), qlonglong(123));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), qulonglong(123));
    }
    {
        QJSValue num(123);
        QCOMPARE(eng.fromScriptValue<char>(num), char(123));
        QCOMPARE(eng.fromScriptValue<unsigned char>(num), (unsigned char)(123));
        QCOMPARE(eng.fromScriptValue<short>(num), short(123));
        QCOMPARE(eng.fromScriptValue<unsigned short>(num), (unsigned short)(123));
        QCOMPARE(eng.fromScriptValue<float>(num), float(123));
        QCOMPARE(eng.fromScriptValue<double>(num), double(123));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), qlonglong(123));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), qulonglong(123));
    }

    {
        QJSValue num = eng.toScriptValue(Q_INT64_C(0x100000000));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), Q_INT64_C(0x100000000));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), Q_UINT64_C(0x100000000));
    }

    {
        QChar c = QLatin1Char('c');
        QJSValue str = eng.toScriptValue(QString::fromLatin1("ciao"));
        QCOMPARE(eng.fromScriptValue<QChar>(str), c);
        QJSValue code = eng.toScriptValue(c.unicode());
        QCOMPARE(eng.fromScriptValue<QChar>(code), c);
        QCOMPARE(eng.fromScriptValue<QChar>(eng.toScriptValue(c)), c);
    }

    QVERIFY(eng.toScriptValue(static_cast<void *>(0)).isNull());
}

void tst_QJSEngine::valueConversion_QVariant()
{
    QJSEngine eng;
    // qScriptValueFromValue() should be "smart" when the argument is a QVariant
    {
        QJSValue val = eng.toScriptValue(QVariant());
        QVERIFY(!val.isVariant());
        QVERIFY(val.isUndefined());
    }
    // Checking nested QVariants
    {
        QVariant tmp1;
        QVariant tmp2(QMetaType::QVariant, &tmp1);
        QVERIFY(QMetaType::Type(tmp2.type()) == QMetaType::QVariant);

        QJSValue val1 = eng.toScriptValue(tmp1);
        QJSValue val2 = eng.toScriptValue(tmp2);
        QVERIFY(val1.isUndefined());
        QEXPECT_FAIL("", "Variant are unrwapped, maybe we should not...", Continue);
        QVERIFY(!val2.isUndefined());
        QVERIFY(!val1.isVariant());
        QEXPECT_FAIL("", "Variant are unrwapped, maybe we should not...", Continue);
        QVERIFY(val2.isVariant());
    }
    {
        QVariant tmp1(123);
        QVariant tmp2(QMetaType::QVariant, &tmp1);
        QVariant tmp3(QMetaType::QVariant, &tmp2);
        QVERIFY(QMetaType::Type(tmp1.type()) == QMetaType::Int);
        QVERIFY(QMetaType::Type(tmp2.type()) == QMetaType::QVariant);
        QVERIFY(QMetaType::Type(tmp3.type()) == QMetaType::QVariant);

        QJSValue val1 = eng.toScriptValue(tmp2);
        QJSValue val2 = eng.toScriptValue(tmp3);
        QVERIFY(!val1.isUndefined());
        QVERIFY(!val2.isUndefined());
        QEXPECT_FAIL("", "Variant are unrwapped, maybe we should not...", Continue);
        QVERIFY(val1.isVariant());
        QEXPECT_FAIL("", "Variant are unrwapped, maybe we should not...", Continue);
        QVERIFY(val2.isVariant());
        QVERIFY(val1.toVariant().toInt() == 123);
        QVERIFY(eng.toScriptValue(val2.toVariant()).toVariant().toInt() == 123);
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(true));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isBool());
        QCOMPARE(val.toBool(), true);
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(int(123)));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isNumber());
        QCOMPARE(val.toNumber(), qreal(123));
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(qreal(1.25)));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isNumber());
        QCOMPARE(val.toNumber(), qreal(1.25));
    }
    {
        QString str = QString::fromLatin1("ciao");
        QJSValue val = eng.toScriptValue(QVariant(str));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isString());
        QCOMPARE(val.toString(), str);
    }
    {
        QJSValue val = eng.toScriptValue(qVariantFromValue((QObject*)this));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isQObject());
        QCOMPARE(val.toQObject(), (QObject*)this);
    }
    {
        QVariant var = qVariantFromValue(QPoint(123, 456));
        QJSValue val = eng.toScriptValue(var);
        QVERIFY(val.isVariant());
        QCOMPARE(val.toVariant(), var);
    }

    QCOMPARE(qjsvalue_cast<QVariant>(QJSValue(123)), QVariant(123));

    QVERIFY(eng.toScriptValue(QVariant(QMetaType::VoidStar, 0)).isNull());
}

void tst_QJSEngine::valueConversion_basic2()
{
    QJSEngine eng;
    // more built-in types
    {
        QJSValue val = eng.toScriptValue(uint(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(qulonglong(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(float(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(short(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(ushort(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(char(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(uchar(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
}

void tst_QJSEngine::valueConversion_dateTime()
{
    QJSEngine eng;
    {
        QDateTime in = QDateTime::currentDateTime();
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isDate());
        QCOMPARE(val.toDateTime(), in);
    }
    {
        QDate in = QDate::currentDate();
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isDate());
        QCOMPARE(val.toDateTime().date(), in);
    }
}

void tst_QJSEngine::valueConversion_regExp()
{
    QJSEngine eng;
    {
        QRegExp in = QRegExp("foo");
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isRegExp());
        QRegExp out = qjsvalue_cast<QRegExp>(val);
        QEXPECT_FAIL("", "QTBUG-6136: JSC-based back-end doesn't preserve QRegExp::patternSyntax (always uses RegExp2)", Continue);
        QCOMPARE(out.patternSyntax(), in.patternSyntax());
        QCOMPARE(out.pattern(), in.pattern());
        QCOMPARE(out.caseSensitivity(), in.caseSensitivity());
        QCOMPARE(out.isMinimal(), in.isMinimal());
    }
    {
        QRegExp in = QRegExp("foo", Qt::CaseSensitive, QRegExp::RegExp2);
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isRegExp());
        QCOMPARE(qjsvalue_cast<QRegExp>(val), in);
    }
    {
        QRegExp in = QRegExp("foo");
        in.setMinimal(true);
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isRegExp());
        QEXPECT_FAIL("", "QTBUG-6136: JSC-based back-end doesn't preserve QRegExp::minimal (always false)", Continue);
        QCOMPARE(qjsvalue_cast<QRegExp>(val).isMinimal(), in.isMinimal());
    }
}

Q_DECLARE_METATYPE(QGradient)
Q_DECLARE_METATYPE(QGradient*)
Q_DECLARE_METATYPE(QLinearGradient)

class Klazz : public QWidget,
              public QStandardItem,
              public QGraphicsItem
{
    Q_INTERFACES(QGraphicsItem)
    Q_OBJECT
public:
    Klazz(QWidget *parent = 0) : QWidget(parent) { }
    virtual QRectF boundingRect() const { return QRectF(); }
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) { }
};

Q_DECLARE_METATYPE(Klazz*)
Q_DECLARE_METATYPE(QStandardItem*)

void tst_QJSEngine::castWithMultipleInheritance()
{
    QJSEngine eng;
    Klazz klz;
    QJSValue v = eng.newQObject(&klz);

    QCOMPARE(qjsvalue_cast<Klazz*>(v), &klz);
    QCOMPARE(qjsvalue_cast<QWidget*>(v), (QWidget *)&klz);
    QCOMPARE(qjsvalue_cast<QObject*>(v), (QObject *)&klz);
    QCOMPARE(qjsvalue_cast<QStandardItem*>(v), (QStandardItem *)&klz);
    QCOMPARE(qjsvalue_cast<QGraphicsItem*>(v), (QGraphicsItem *)&klz);
}

void tst_QJSEngine::collectGarbage()
{
    QJSEngine eng;
    eng.evaluate("a = new Object(); a = new Object(); a = new Object()");
    QJSValue a = eng.newObject();
    a = eng.newObject();
    a = eng.newObject();
    QPointer<QObject> ptr = new QObject();
    QVERIFY(ptr != 0);
    (void)eng.newQObject(ptr);
    collectGarbage_helper(eng);
    if (ptr)
        QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
    QVERIFY(ptr == 0);
}

void tst_QJSEngine::gcWithNestedDataStructure()
{
    // The GC must be able to traverse deeply nested objects, otherwise this
    // test would crash.
    QJSEngine eng;
    QJSValue ret = eng.evaluate(
        "function makeList(size)"
        "{"
        "  var head = { };"
        "  var l = head;"
        "  for (var i = 0; i < size; ++i) {"
        "    l.data = i + \"\";"
        "    l.next = { }; l = l.next;"
        "  }"
        "  l.next = null;"
        "  return head;"
        "}");
    QVERIFY(!ret.isError());
    const int size = 200;
    QJSValue head = eng.evaluate(QString::fromLatin1("makeList(%0)").arg(size));
    QVERIFY(!head.isError());
    for (int x = 0; x < 2; ++x) {
        if (x == 1)
            eng.evaluate("gc()");
        QJSValue l = head;
        // Make sure all the nodes are still alive.
        for (int i = 0; i < 200; ++i) {
            QCOMPARE(l.property("data").toString(), QString::number(i));
            l = l.property("next");
        }
    }
}

void tst_QJSEngine::stacktrace()
{
    QString script = QString::fromLatin1(
        "function foo(counter) {\n"
        "    switch (counter) {\n"
        "        case 0: foo(counter+1); break;\n"
        "        case 1: foo(counter+1); break;\n"
        "        case 2: foo(counter+1); break;\n"
        "        case 3: foo(counter+1); break;\n"
        "        case 4: foo(counter+1); break;\n"
        "        default:\n"
        "        throw new Error('blah');\n"
        "    }\n"
        "}\n"
        "foo(0);");

    const QString fileName("testfile");

    QStringList backtrace;
    backtrace << "foo(5)@testfile:9"
              << "foo(4)@testfile:7"
              << "foo(3)@testfile:6"
              << "foo(2)@testfile:5"
              << "foo(1)@testfile:4"
              << "foo(0)@testfile:3"
              << "<global>()@testfile:12";

    QJSEngine eng;
    QJSValue result = eng.evaluate(script, fileName);
    QVERIFY(result.isError());

    QJSValue stack = result.property("stack");

    QJSValueIterator it(stack);
    int counter = 5;
    while (it.hasNext()) {
        it.next();
        QJSValue obj = it.value();
        QJSValue frame = obj.property("frame");

        QCOMPARE(obj.property("fileName").toString(), fileName);
        if (counter >= 0) {
            QJSValue callee = frame.property("arguments").property("callee");
            QVERIFY(callee.strictlyEquals(eng.globalObject().property("foo")));
            QCOMPARE(obj.property("functionName").toString(), QString("foo"));
            int line = obj.property("lineNumber").toInt();
            if (counter == 5)
                QCOMPARE(line, 9);
            else
                QCOMPARE(line, 3 + counter);
        } else {
            QVERIFY(frame.strictlyEquals(eng.globalObject()));
            QVERIFY(obj.property("functionName").toString().isEmpty());
        }

        --counter;
    }

    // throw something that isn't an Error object
    // ###FIXME: No uncaughtExceptionBacktrace: QVERIFY(eng.uncaughtExceptionBacktrace().isEmpty());
    QString script2 = QString::fromLatin1(
        "function foo(counter) {\n"
        "    switch (counter) {\n"
        "        case 0: foo(counter+1); break;\n"
        "        case 1: foo(counter+1); break;\n"
        "        case 2: foo(counter+1); break;\n"
        "        case 3: foo(counter+1); break;\n"
        "        case 4: foo(counter+1); break;\n"
        "        default:\n"
        "        throw 'just a string';\n"
        "    }\n"
        "}\n"
        "foo(0);");

    QJSValue result2 = eng.evaluate(script2, fileName);
    QVERIFY(!result2.isError());
    QVERIFY(result2.isString());
}

void tst_QJSEngine::numberParsing_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<qreal>("expect");

    QTest::newRow("decimal 0") << QString("0") << qreal(0);
    QTest::newRow("octal 0") << QString("00") << qreal(00);
    QTest::newRow("hex 0") << QString("0x0") << qreal(0x0);
    QTest::newRow("decimal 100") << QString("100") << qreal(100);
    QTest::newRow("hex 100") << QString("0x100") << qreal(0x100);
    QTest::newRow("octal 100") << QString("0100") << qreal(0100);
    QTest::newRow("decimal 4G") << QString("4294967296") << qreal(Q_UINT64_C(4294967296));
    QTest::newRow("hex 4G") << QString("0x100000000") << qreal(Q_UINT64_C(0x100000000));
    QTest::newRow("octal 4G") << QString("040000000000") << qreal(Q_UINT64_C(040000000000));
    QTest::newRow("0.5") << QString("0.5") << qreal(0.5);
    QTest::newRow("1.5") << QString("1.5") << qreal(1.5);
    QTest::newRow("1e2") << QString("1e2") << qreal(100);
}

void tst_QJSEngine::numberParsing()
{
    QFETCH(QString, string);
    QFETCH(qreal, expect);

    QJSEngine eng;
    QJSValue ret = eng.evaluate(string);
    QVERIFY(ret.isNumber());
    qreal actual = ret.toNumber();
    QCOMPARE(actual, expect);
}

// see ECMA-262, section 7.9
// This is testing ECMA compliance, not our C++ API, but it's important that
// the back-end is conformant in this regard.
void tst_QJSEngine::automaticSemicolonInsertion()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("{ 1 2 } 3");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("{ 1\n2 } 3");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("for (a; b\n)");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("(function() { return\n1 + 2 })()");
        QVERIFY(ret.isUndefined());
    }
    {
        eng.evaluate("c = 2; b = 1");
        QJSValue ret = eng.evaluate("a = b\n++c");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("if (a > b)\nelse c = d");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        eng.evaluate("function c() { return { foo: function() { return 5; } } }");
        eng.evaluate("b = 1; d = 2; e = 3");
        QJSValue ret = eng.evaluate("a = b + c\n(d + e).foo()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("throw\n1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("a = Number(1)\n++a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 2);
    }

    // "a semicolon is never inserted automatically if the semicolon
    // would then be parsed as an empty statement"
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0)\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0)\n --a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if ((0))\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if ((0))\n --a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0\n)\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0\n ++a; a");
        QVERIFY(ret.isError());
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0))\n ++a; a");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("n = 0; for (i = 0; i < 10; ++i)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; for (i = 0; i < 10; ++i)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("n = 0; for (var i = 0; i < 10; ++i)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; for (var i = 0; i < 10; ++i)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("n = 0; i = 0; while (i++ < 10)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; i = 0; while (i++ < 10)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 0; for (i in o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 9; for (i in o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 0; for (var i in o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 9; for (var i in o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("o = { n: 3 }; n = 5; with (o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 5);
    }
    {
        QJSValue ret = eng.evaluate("o = { n: 3 }; n = 10; with (o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 5; i = 0; do\n ++n; while (++i < 10); n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 15);
    }
    {
        QJSValue ret = eng.evaluate("n = 20; i = 0; do\n --n; while (++i < 10); n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }

    {
        QJSValue ret = eng.evaluate("n = 1; i = 0; if (n) i\n++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 2);
    }
    {
        QJSValue ret = eng.evaluate("n = 1; i = 0; if (n) i\n--n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 0);
    }

    {
        QJSValue ret = eng.evaluate("if (0)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("while (0)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("for (;;)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("for (p in this)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("with (this)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("do");
        QVERIFY(ret.isError());
    }
}

void tst_QJSEngine::errorConstructors()
{
    QJSEngine eng;
    QStringList prefixes;
    prefixes << "" << "Eval" << "Range" << "Reference" << "Syntax" << "Type" << "URI";
    for (int x = 0; x < 3; ++x) {
        for (int i = 0; i < prefixes.size(); ++i) {
            QString name = prefixes.at(i) + QLatin1String("Error");
            QString code = QString(i+1, QLatin1Char('\n'));
            if (x == 0)
                code += QLatin1String("throw ");
            else if (x == 1)
                code += QLatin1String("new ");
            code += name + QLatin1String("()");
            QJSValue ret = eng.evaluate(code);
            QVERIFY(ret.isError());
            QVERIFY(ret.toString().startsWith(name));
            //QTBUG-6138: JSC doesn't assign lineNumber when errors are not thrown
            QEXPECT_FAIL("", "we have no more lineNumber property ", Continue);
            QCOMPARE(ret.property("lineNumber").toInt(), i+2);
        }
    }
}

void tst_QJSEngine::argumentsProperty_globalContext()
{
    QJSEngine eng;
    {
        // Unlike function calls, the global context doesn't have an
        // arguments property.
        QJSValue ret = eng.evaluate("arguments");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    eng.evaluate("arguments = 10");
    {
        QJSValue ret = eng.evaluate("arguments");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    QVERIFY(eng.evaluate("delete arguments").toBool());
    QVERIFY(eng.globalObject().property("arguments").isUndefined());
}

void tst_QJSEngine::argumentsProperty_JS()
{
    {
        QJSEngine eng;
        eng.evaluate("o = { arguments: 123 }");
        QJSValue ret = eng.evaluate("with (o) { arguments; }");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        QJSEngine eng;
        QVERIFY(eng.globalObject().property("arguments").isUndefined());
        // This is testing ECMA-262 compliance. In function calls, "arguments"
        // appears like a local variable, and it can be replaced.
        QJSValue ret = eng.evaluate("(function() { arguments = 456; return arguments; })()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 456);
        QVERIFY(eng.globalObject().property("arguments").isUndefined());
    }
}

void tst_QJSEngine::jsNumberClass()
{
    // See ECMA-262 Section 15.7, "Number Objects".

    QJSEngine eng;

    QJSValue ctor = eng.globalObject().property("Number");
    QVERIFY(ctor.property("length").isNumber());
    QCOMPARE(ctor.property("length").toNumber(), qreal(1));
    QJSValue proto = ctor.property("prototype");
    QVERIFY(proto.isObject());
    {
        QVERIFY(ctor.property("MAX_VALUE").isNumber());
        QVERIFY(ctor.property("MIN_VALUE").isNumber());
        QVERIFY(ctor.property("NaN").isNumber());
        QVERIFY(ctor.property("NEGATIVE_INFINITY").isNumber());
        QVERIFY(ctor.property("POSITIVE_INFINITY").isNumber());
    }
    QCOMPARE(proto.toNumber(), qreal(0));
    QVERIFY(proto.property("constructor").strictlyEquals(ctor));

    {
        QJSValue ret = eng.evaluate("Number()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(0));
    }
    {
        QJSValue ret = eng.evaluate("Number(123)");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    {
        QJSValue ret = eng.evaluate("Number('456')");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(456));
    }
    {
        QJSValue ret = eng.evaluate("new Number()");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(0));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123)");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    {
        QJSValue ret = eng.evaluate("new Number('456')");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(456));
    }

    QVERIFY(proto.property("toString").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toString()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123).toString(8)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("173"));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123).toString(16)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("7b"));
    }
    QVERIFY(proto.property("toLocaleString").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toLocaleString()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
    QVERIFY(proto.property("valueOf").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).valueOf()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    QVERIFY(proto.property("toExponential").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toExponential()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("1.23e+2"));
    }
    QVERIFY(proto.property("toFixed").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toFixed()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
    QVERIFY(proto.property("toPrecision").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toPrecision()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
}

void tst_QJSEngine::jsForInStatement_simple()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("o = { }; r = []; for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QVERIFY(lst.isEmpty());
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123, q: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
        QCOMPARE(lst.at(1), QString::fromLatin1("q"));
    }
}

void tst_QJSEngine::jsForInStatement_prototypeProperties()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("o = { }; o.__proto__ = { p: 123 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; o.__proto__ = { q: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
        QCOMPARE(lst.at(1), QString::fromLatin1("q"));
    }
    {
        // shadowed property
        QJSValue ret = eng.evaluate("o = { p: 123 }; o.__proto__ = { p: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

}

void tst_QJSEngine::jsForInStatement_mutateWhileIterating()
{
    QJSEngine eng;
    // deleting property during enumeration
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; delete r[p]; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123, q: 456 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; delete o.q; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

    // adding property during enumeration
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; o.q = 456; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

}

void tst_QJSEngine::jsForInStatement_arrays()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
    }
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; a.foo = 'bar'; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 3);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
        QCOMPARE(lst.at(2), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; a.foo = 'bar';"
                                        "b = [111, 222, 333]; b.bar = 'baz';"
                                        "a.__proto__ = b; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 5);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
        QCOMPARE(lst.at(2), QString::fromLatin1("foo"));
        QCOMPARE(lst.at(3), QString::fromLatin1("2"));
        QCOMPARE(lst.at(4), QString::fromLatin1("bar"));
    }
}

void tst_QJSEngine::jsForInStatement_nullAndUndefined()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("r = true; for (var p in undefined) r = false; r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
    {
        QJSValue ret = eng.evaluate("r = true; for (var p in null) r = false; r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
}

void tst_QJSEngine::jsFunctionDeclarationAsStatement()
{
    // ECMA-262 does not allow function declarations to be used as statements,
    // but several popular implementations (including JSC) do. See the NOTE
    // at the beginning of chapter 12 in ECMA-262 5th edition, where it's
    // recommended that implementations either disallow this usage or issue
    // a warning.
    // Since we had a bug report long ago about Qt Script not supporting this
    // "feature" (and thus deviating from other implementations), we still
    // check this behavior.

    QJSEngine eng;
    QVERIFY(eng.globalObject().property("bar").isUndefined());
    eng.evaluate("function foo(arg) {\n"
                 "  if (arg == 'bar')\n"
                 "    function bar() { return 'bar'; }\n"
                 "  else\n"
                 "    function baz() { return 'baz'; }\n"
                 "  return (arg == 'bar') ? bar : baz;\n"
                 "}");
    QVERIFY(eng.globalObject().property("bar").isUndefined());
    QVERIFY(eng.globalObject().property("baz").isUndefined());
    QVERIFY(eng.evaluate("foo").isCallable());
    {
        QJSValue ret = eng.evaluate("foo('bar')");
        QVERIFY(ret.isCallable());
        QJSValue ret2 = ret.call();
        QCOMPARE(ret2.toString(), QString::fromLatin1("bar"));
        QVERIFY(eng.globalObject().property("bar").isUndefined());
        QVERIFY(eng.globalObject().property("baz").isUndefined());
    }
    {
        QJSValue ret = eng.evaluate("foo('baz')");
        QVERIFY(ret.isCallable());
        QJSValue ret2 = ret.call();
        QCOMPARE(ret2.toString(), QString::fromLatin1("baz"));
        QVERIFY(eng.globalObject().property("bar").isUndefined());
        QVERIFY(eng.globalObject().property("baz").isUndefined());
    }
}

void tst_QJSEngine::stringObjects()
{
    // See ECMA-262 Section 15.5, "String Objects".

    QJSEngine eng;
    QString str("ciao");
    // in C++
    {
        QJSValue obj = eng.evaluate(QString::fromLatin1("new String('%0')").arg(str));
        QCOMPARE(obj.property("length").toInt(), str.length());
        for (int i = 0; i < str.length(); ++i) {
            QString pname = QString::number(i);
            QVERIFY(obj.property(pname).isString());
            QCOMPARE(obj.property(pname).toString(), QString(str.at(i)));
            QVERIFY(!obj.deleteProperty(pname));
            obj.setProperty(pname, 123);
            QVERIFY(obj.property(pname).isString());
            QCOMPARE(obj.property(pname).toString(), QString(str.at(i)));
        }
        QVERIFY(obj.property("-1").isUndefined());
        QVERIFY(obj.property(QString::number(str.length())).isUndefined());

        QJSValue val = eng.toScriptValue(123);
        obj.setProperty("-1", val);
        QVERIFY(obj.property("-1").strictlyEquals(val));
        obj.setProperty("100", val);
        QVERIFY(obj.property("100").strictlyEquals(val));
    }

    {
        QJSValue ret = eng.evaluate("s = new String('ciao'); r = []; for (var p in s) r.push(p); r");
        QVERIFY(ret.isArray());
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), str.length());
        for (int i = 0; i < str.length(); ++i)
            QCOMPARE(lst.at(i), QString::number(i));

        QJSValue ret2 = eng.evaluate("s[0] = 123; s[0]");
        QVERIFY(ret2.isString());
        QCOMPARE(ret2.toString().length(), 1);
        QCOMPARE(ret2.toString().at(0), str.at(0));

        QJSValue ret3 = eng.evaluate("s[-1] = 123; s[-1]");
        QVERIFY(ret3.isNumber());
        QCOMPARE(ret3.toInt(), 123);

        QJSValue ret4 = eng.evaluate("s[s.length] = 456; s[s.length]");
        QVERIFY(ret4.isNumber());
        QCOMPARE(ret4.toInt(), 456);

        QJSValue ret5 = eng.evaluate("delete s[0]");
        QVERIFY(ret5.isBool());
        QVERIFY(!ret5.toBool());

        QJSValue ret6 = eng.evaluate("delete s[-1]");
        QVERIFY(ret6.isBool());
        QVERIFY(ret6.toBool());

        QJSValue ret7 = eng.evaluate("delete s[s.length]");
        QVERIFY(ret7.isBool());
        QVERIFY(ret7.toBool());
    }
}

void tst_QJSEngine::jsStringPrototypeReplaceBugs()
{
    QJSEngine eng;
    // task 212440
    {
        QJSValue ret = eng.evaluate("replace_args = []; \"a a a\".replace(/(a)/g, function() { replace_args.push(arguments); }); replace_args");
        QVERIFY(ret.isArray());
        int len = ret.property("length").toInt();
        QCOMPARE(len, 3);
        for (int i = 0; i < len; ++i) {
            QJSValue args = ret.property(i);
            QCOMPARE(args.property("length").toInt(), 4);
            QCOMPARE(args.property(0).toString(), QString::fromLatin1("a")); // matched string
            QCOMPARE(args.property(1).toString(), QString::fromLatin1("a")); // capture
            QVERIFY(args.property(2).isNumber());
            QCOMPARE(args.property(2).toInt(), i*2); // index of match
            QCOMPARE(args.property(3).toString(), QString::fromLatin1("a a a"));
        }
    }
    // task 212501
    {
        QJSValue ret = eng.evaluate("\"foo\".replace(/a/g, function() {})");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
}

void tst_QJSEngine::getterSetterThisObject_global()
{
    {
        QJSEngine eng;
        // read
        eng.evaluate("__defineGetter__('x', function() { return this; });");
        {
            QJSValue ret = eng.evaluate("x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("(function() { return x; })()");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("with (this) x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("with ({}) x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("(function() { with ({}) return x; })()");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        // write
        eng.evaluate("__defineSetter__('x', function() { return this; });");
        {
            QJSValue ret = eng.evaluate("x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("(function() { return x = 'foo'; })()");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("with (this) x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("with ({}) x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("(function() { with ({}) return x = 'foo'; })()");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
    }
}

void tst_QJSEngine::getterSetterThisObject_plain()
{
    {
        QJSEngine eng;
        eng.evaluate("o = {}");
        // read
        eng.evaluate("o.__defineGetter__('x', function() { return this; })");
        QVERIFY(eng.evaluate("o.x === o").toBool());
        QVERIFY(eng.evaluate("with (o) x").equals(eng.evaluate("o")));
        QVERIFY(eng.evaluate("(function() { with (o) return x; })() === o").toBool());
        eng.evaluate("q = {}; with (o) with (q) x").equals(eng.evaluate("o"));
        // write
        eng.evaluate("o.__defineSetter__('x', function() { return this; });");
        // SpiderMonkey says setter return value, JSC says RHS.
        QVERIFY(eng.evaluate("(o.x = 'foo') === 'foo'").toBool());
        QVERIFY(eng.evaluate("with (o) x = 'foo'").equals("foo"));
        QVERIFY(eng.evaluate("with (o) with (q) x = 'foo'").equals("foo"));
    }
}

void tst_QJSEngine::getterSetterThisObject_prototypeChain()
{
    {
        QJSEngine eng;
        eng.evaluate("o = {}; p = {}; o.__proto__ = p");
        // read
        eng.evaluate("p.__defineGetter__('x', function() { return this; })");
        QVERIFY(eng.evaluate("o.x === o").toBool());
        QVERIFY(eng.evaluate("with (o) x").equals(eng.evaluate("o")));
        QVERIFY(eng.evaluate("(function() { with (o) return x; })() === o").toBool());
        eng.evaluate("q = {}; with (o) with (q) x").equals(eng.evaluate("o"));
        eng.evaluate("with (q) with (o) x").equals(eng.evaluate("o"));
        // write
        eng.evaluate("o.__defineSetter__('x', function() { return this; });");
        // SpiderMonkey says setter return value, JSC says RHS.
        QVERIFY(eng.evaluate("(o.x = 'foo') === 'foo'").toBool());
        QVERIFY(eng.evaluate("with (o) x = 'foo'").equals("foo"));
        QVERIFY(eng.evaluate("with (o) with (q) x = 'foo'").equals("foo"));
    }
}

void tst_QJSEngine::jsContinueInSwitch()
{
    // This is testing ECMA-262 compliance, not C++ API.

    QJSEngine eng;
    // switch - continue
    {
        QJSValue ret = eng.evaluate("switch (1) { default: continue; }");
        QVERIFY(ret.isError());
    }
    // for - switch - case - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1: ++j; continue;\n"
                                        "    case 100: ++j; continue;\n"
                                        "    case 1000: ++j; continue;\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    // for - switch - case - default - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1: ++j; continue;\n"
                                        "    case 100: ++j; continue;\n"
                                        "    case 1000: ++j; continue;\n"
                                        "    default: if (i < 50000) break; else continue;\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    // switch - for - continue
    {
        QJSValue ret = eng.evaluate("j = 123; switch (j) {\n"
                                        "  case 123:\n"
                                        "  for (i = 0; i < 100000; ++i) {\n"
                                        "    continue;\n"
                                        "  }\n"
                                        "}; i\n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 100000);
    }
    // switch - switch - continue
    {
        QJSValue ret = eng.evaluate("i = 1; switch (i) { default: switch (i) { case 1: continue; } }");
        QVERIFY(ret.isError());
    }
    // for - switch - switch - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1:\n"
                                        "    switch (i) {\n"
                                        "      case 1: ++j; continue;\n"
                                        "    }\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 1);
    }
    // switch - for - switch - continue
    {
        QJSValue ret = eng.evaluate("j = 123; switch (j) {\n"
                                        "  case 123:\n"
                                        "  for (i = 0; i < 100000; ++i) {\n"
                                        "    switch (i) {\n"
                                        "      case 1:\n"
                                        "      ++j; continue;\n"
                                        "    }\n"
                                        "  }\n"
                                        "}; i\n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 100000);
    }
}

void tst_QJSEngine::jsShadowReadOnlyPrototypeProperty()
{
    QJSEngine eng;
    QVERIFY(eng.evaluate("o = {}; o.__proto__ = parseInt; o.length").isNumber());
    QVERIFY(eng.evaluate("o.length = 123; o.length").toInt() != 123);
    QVERIFY(!eng.evaluate("o.hasOwnProperty('length')").toBool());
}

void tst_QJSEngine::jsReservedWords_data()
{
    QTest::addColumn<QString>("word");
    QTest::newRow("break") << QString("break");
    QTest::newRow("case") << QString("case");
    QTest::newRow("catch") << QString("catch");
    QTest::newRow("continue") << QString("continue");
    QTest::newRow("default") << QString("default");
    QTest::newRow("delete") << QString("delete");
    QTest::newRow("do") << QString("do");
    QTest::newRow("else") << QString("else");
    QTest::newRow("false") << QString("false");
    QTest::newRow("finally") << QString("finally");
    QTest::newRow("for") << QString("for");
    QTest::newRow("function") << QString("function");
    QTest::newRow("if") << QString("if");
    QTest::newRow("in") << QString("in");
    QTest::newRow("instanceof") << QString("instanceof");
    QTest::newRow("new") << QString("new");
    QTest::newRow("null") << QString("null");
    QTest::newRow("return") << QString("return");
    QTest::newRow("switch") << QString("switch");
    QTest::newRow("this") << QString("this");
    QTest::newRow("throw") << QString("throw");
    QTest::newRow("true") << QString("true");
    QTest::newRow("try") << QString("try");
    QTest::newRow("typeof") << QString("typeof");
    QTest::newRow("var") << QString("var");
    QTest::newRow("void") << QString("void");
    QTest::newRow("while") << QString("while");
    QTest::newRow("with") << QString("with");
}

void tst_QJSEngine::jsReservedWords()
{
    // See ECMA-262 Section 7.6.1, "Reserved Words".
    // We prefer that the implementation is less strict than the spec; e.g.
    // it's good to allow reserved words as identifiers in object literals,
    // and when accessing properties using dot notation.

    QFETCH(QString, word);
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate(word + " = 123");
        QVERIFY(ret.isError());
        QString str = ret.toString();
        QVERIFY(str.startsWith("SyntaxError") || str.startsWith("ReferenceError"));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("var " + word + " = 123");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().startsWith("SyntaxError"));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = {}; o." + word + " = 123");
        // in the old back-end, in SpiderMonkey and in v8, this is allowed, but not in JSC
        QVERIFY(!ret.isError());
        QVERIFY(ret.strictlyEquals(eng.evaluate("o." + word)));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = { " + word + ": 123 }");
        // in the old back-end, in SpiderMonkey and in v8, this is allowed, but not in JSC
        QVERIFY(!ret.isError());
        QVERIFY(ret.property(word).isNumber());
    }
    {
        // SpiderMonkey allows this, but we don't
        QJSEngine eng;
        QJSValue ret = eng.evaluate("function " + word + "() {}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().startsWith("SyntaxError"));
    }
}

void tst_QJSEngine::jsFutureReservedWords_data()
{
    QTest::addColumn<QString>("word");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("abstract") << QString("abstract") << true;
    QTest::newRow("boolean") << QString("boolean") << true;
    QTest::newRow("byte") << QString("byte") << true;
    QTest::newRow("char") << QString("char") << true;
    QTest::newRow("class") << QString("class") << false;
    QTest::newRow("const") << QString("const") << false;
    QTest::newRow("debugger") << QString("debugger") << false;
    QTest::newRow("double") << QString("double") << true;
    QTest::newRow("enum") << QString("enum") << false;
    QTest::newRow("export") << QString("export") << false;
    QTest::newRow("extends") << QString("extends") << false;
    QTest::newRow("final") << QString("final") << true;
    QTest::newRow("float") << QString("float") << true;
    QTest::newRow("goto") << QString("goto") << true;
    QTest::newRow("implements") << QString("implements") << true;
    QTest::newRow("import") << QString("import") << false;
    QTest::newRow("int") << QString("int") << true;
    QTest::newRow("interface") << QString("interface") << true;
    QTest::newRow("long") << QString("long") << true;
    QTest::newRow("native") << QString("native") << true;
    QTest::newRow("package") << QString("package") << true;
    QTest::newRow("private") << QString("private") << true;
    QTest::newRow("protected") << QString("protected") << true;
    QTest::newRow("public") << QString("public") << true;
    QTest::newRow("short") << QString("short") << true;
    QTest::newRow("static") << QString("static") << true;
    QTest::newRow("super") << QString("super") << false;
    QTest::newRow("synchronized") << QString("synchronized") << true;
    QTest::newRow("throws") << QString("throws") << true;
    QTest::newRow("transient") << QString("transient") << true;
    QTest::newRow("volatile") << QString("volatile") << true;
}

void tst_QJSEngine::jsFutureReservedWords()
{
    // See ECMA-262 Section 7.6.1.2, "Future Reserved Words".
    // In real-world implementations, most of these words are
    // actually allowed as normal identifiers.

    QFETCH(QString, word);
    QFETCH(bool, allowed);
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate(word + " = 123");
        QCOMPARE(!ret.isError(), allowed);
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("var " + word + " = 123");
        QCOMPARE(!ret.isError(), allowed);
    }
    {
        // this should probably be allowed (see task 162567)
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = {}; o." + word + " = 123");

        QEXPECT_FAIL("class", "QTBUG-27193", Abort);
        QEXPECT_FAIL("const", "QTBUG-27193", Abort);
        QEXPECT_FAIL("debugger", "QTBUG-27193", Abort);
        QEXPECT_FAIL("enum", "QTBUG-27193", Abort);
        QEXPECT_FAIL("export", "QTBUG-27193", Abort);
        QEXPECT_FAIL("extends", "QTBUG-27193", Abort);
        QEXPECT_FAIL("import", "QTBUG-27193", Abort);
        QEXPECT_FAIL("super", "QTBUG-27193", Abort);

        QCOMPARE(ret.isNumber(), allowed);
        QCOMPARE(!ret.isError(), allowed);
    }
    {
        // this should probably be allowed (see task 162567)
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = { " + word + ": 123 }");
        QCOMPARE(!ret.isError(), allowed);
    }
}

void tst_QJSEngine::jsThrowInsideWithStatement()
{
    // This is testing ECMA-262 compliance, not C++ API.

    // task 209988
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate(
            "try {"
            "  o = { bad : \"bug\" };"
            "  with (o) {"
            "    throw 123;"
            "  }"
            "} catch (e) {"
            "  bad;"
            "}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    {
        QJSValue ret = eng.evaluate(
            "try {"
            "  o = { bad : \"bug\" };"
            "  with (o) {"
            "    throw 123;"
            "  }"
            "} finally {"
            "  bad;"
            "}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    {
        QJSValue ret = eng.evaluate(
            "o = { bug : \"no bug\" };"
            "with (o) {"
            "  try {"
            "    throw 123;"
            "  } finally {"
            "    bug;"
            "  }"
            "}");
        QVERIFY(!ret.isError());
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        QJSValue ret = eng.evaluate(
            "o = { bug : \"no bug\" };"
            "with (o) {"
            "    throw 123;"
            "}");
        QVERIFY(ret.isNumber());
        QJSValue ret2 = eng.evaluate("bug");
        QVERIFY(ret2.isError());
        QVERIFY(ret2.toString().contains(QString::fromLatin1("ReferenceError")));
    }
}

void tst_QJSEngine::reentrancy_globalObjectProperties()
{
    QJSEngine eng1;
    QJSEngine eng2;
    QVERIFY(eng2.globalObject().property("a").isUndefined());
    eng1.evaluate("a = 10");
    QVERIFY(eng1.globalObject().property("a").isNumber());
    QVERIFY(eng2.globalObject().property("a").isUndefined());
    eng2.evaluate("a = 20");
    QVERIFY(eng2.globalObject().property("a").isNumber());
    QCOMPARE(eng1.globalObject().property("a").toInt(), 10);
}

void tst_QJSEngine::reentrancy_Array()
{
    // weird bug with JSC backend
    {
        QJSEngine eng;
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
        eng.evaluate("Array.prototype.toString");
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
    }
    {
        QJSEngine eng;
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
    }
}

void tst_QJSEngine::reentrancy_objectCreation()
{
    // Owned by JS engine, as newQObject() sets JS ownership explicitly
    QObject *temp = new QObject();

    QJSEngine eng1;
    QJSEngine eng2;
    {
        QDateTime dt = QDateTime::currentDateTime();
        QJSValue d1 = eng1.toScriptValue(dt);
        QJSValue d2 = eng2.toScriptValue(dt);
        QCOMPARE(d1.toDateTime(), d2.toDateTime());
        QCOMPARE(d2.toDateTime(), d1.toDateTime());
    }
    {
        QJSValue r1 = eng1.evaluate("new RegExp('foo', 'gim')");
        QJSValue r2 = eng2.evaluate("new RegExp('foo', 'gim')");
        QCOMPARE(qjsvalue_cast<QRegExp>(r1), qjsvalue_cast<QRegExp>(r2));
        QCOMPARE(qjsvalue_cast<QRegExp>(r2), qjsvalue_cast<QRegExp>(r1));
    }
    {
        QJSValue o1 = eng1.newQObject(temp);
        QJSValue o2 = eng2.newQObject(temp);
        QCOMPARE(o1.toQObject(), o2.toQObject());
        QCOMPARE(o2.toQObject(), o1.toQObject());
    }
}

void tst_QJSEngine::jsIncDecNonObjectProperty()
{
    // This is testing ECMA-262 compliance, not C++ API.

    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("var a; a.n++");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n--");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = null; a.n++");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = null; a.n--");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; ++a.n");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; --a.n");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n += 1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n -= 1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; a.length++");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 4);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; a.length--");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 4);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; ++a.length");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 5);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; --a.length");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
}

static QRegExp minimal(QRegExp r) { r.setMinimal(true); return r; }

void tst_QJSEngine::qRegExpInport_data()
{
    QTest::addColumn<QRegExp>("rx");
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("matched");

    QTest::newRow("normal")  << QRegExp("(test|foo)") << "test _ foo _ test _ Foo";
    QTest::newRow("normal2")  << QRegExp("(Test|Foo)") << "test _ foo _ test _ Foo";
    QTest::newRow("case insensitive)")  << QRegExp("(test|foo)", Qt::CaseInsensitive) << "test _ foo _ test _ Foo";
    QTest::newRow("case insensitive2)")  << QRegExp("(Test|Foo)", Qt::CaseInsensitive) << "test _ foo _ test _ Foo";
    QTest::newRow("b(a*)(b*)")  << QRegExp("b(a*)(b*)", Qt::CaseInsensitive) << "aaabbBbaAabaAaababaaabbaaab";
    QTest::newRow("greedy")  << QRegExp("a*(a*)", Qt::CaseInsensitive, QRegExp::RegExp2) << "aaaabaaba";
    QTest::newRow("willcard")  << QRegExp("*.txt", Qt::CaseSensitive, QRegExp::Wildcard) << "file.txt";
    QTest::newRow("willcard 2")  << QRegExp("a?b.txt", Qt::CaseSensitive, QRegExp::Wildcard) << "ab.txt abb.rtc acb.txt";
    QTest::newRow("slash")  << QRegExp("g/.*/s", Qt::CaseInsensitive, QRegExp::RegExp2) << "string/string/string";
    QTest::newRow("slash2")  << QRegExp("g / .* / s", Qt::CaseInsensitive, QRegExp::RegExp2) << "string / string / string";
    QTest::newRow("fixed")  << QRegExp("a*aa.a(ba)*a\\ba", Qt::CaseInsensitive, QRegExp::FixedString) << "aa*aa.a(ba)*a\\ba";
    QTest::newRow("fixed insensitive")  << QRegExp("A*A", Qt::CaseInsensitive, QRegExp::FixedString) << "a*A A*a A*A a*a";
    QTest::newRow("fixed sensitive")  << QRegExp("A*A", Qt::CaseSensitive, QRegExp::FixedString) << "a*A A*a A*A a*a";
    QTest::newRow("html")  << QRegExp("<b>(.*)</b>", Qt::CaseSensitive, QRegExp::RegExp2) << "<b>bold</b><i>italic</i><b>bold</b>";
    QTest::newRow("html minimal")  << minimal(QRegExp("<b>(.*)</b>", Qt::CaseSensitive, QRegExp::RegExp2)) << "<b>bold</b><i>italic</i><b>bold</b>";
    QTest::newRow("aaa")  << QRegExp("a{2,5}") << "aAaAaaaaaAa";
    QTest::newRow("aaa minimal")  << minimal(QRegExp("a{2,5}")) << "aAaAaaaaaAa";
    QTest::newRow("minimal")  << minimal(QRegExp(".*\\} [*8]")) << "}?} ?} *";
    QTest::newRow(".? minimal")  << minimal(QRegExp(".?")) << ".?";
    QTest::newRow(".+ minimal")  << minimal(QRegExp(".+")) << ".+";
    QTest::newRow("[.?] minimal")  << minimal(QRegExp("[.?]")) << ".?";
    QTest::newRow("[.+] minimal")  << minimal(QRegExp("[.+]")) << ".+";
}

void tst_QJSEngine::qRegExpInport()
{
    QFETCH(QRegExp, rx);
    QFETCH(QString, string);

    QJSEngine eng;
    QJSValue rexp;
    rexp = eng.toScriptValue(rx);

    QCOMPARE(rexp.isRegExp(), true);
    QCOMPARE(rexp.isCallable(), false);

    QJSValue func = eng.evaluate("(function(string, regexp) { return string.match(regexp); })");
    QJSValue result = func.call(QJSValueList() << string << rexp);

    rx.indexIn(string);
    for (int i = 0; i <= rx.captureCount(); i++)  {
        QCOMPARE(result.property(i).toString(), rx.cap(i));
    }
}

// QScriptValue::toDateTime() returns a local time, whereas JS dates
// are always stored as UTC. Qt Script must respect the current time
// zone, and correctly adjust for daylight saving time that may be in
// effect at a given date (QTBUG-9770).
void tst_QJSEngine::dateRoundtripJSQtJS()
{
    uint secs = QDateTime(QDate(2009, 1, 1)).toUTC().toTime_t();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.evaluate(QString::fromLatin1("new Date(%0)").arg(secs * 1000.0));
        QDateTime qtDate = jsDate.toDateTime();
        QJSValue jsDate2 = eng.toScriptValue(qtDate);
        if (jsDate2.toNumber() != jsDate.toNumber())
            QFAIL(qPrintable(jsDate.toString()));
        secs += 2*60*60;
    }
}

void tst_QJSEngine::dateRoundtripQtJSQt()
{
    QDateTime qtDate = QDateTime(QDate(2009, 1, 1));
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.toScriptValue(qtDate);
        QDateTime qtDate2 = jsDate.toDateTime();
        if (qtDate2 != qtDate)
            QFAIL(qPrintable(qtDate.toString()));
        qtDate = qtDate.addSecs(2*60*60);
    }
}

void tst_QJSEngine::dateConversionJSQt()
{
    uint secs = QDateTime(QDate(2009, 1, 1)).toUTC().toTime_t();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.evaluate(QString::fromLatin1("new Date(%0)").arg(secs * 1000.0));
        QDateTime qtDate = jsDate.toDateTime();
        QString qtUTCDateStr = qtDate.toUTC().toString(Qt::ISODate);
        QString jsUTCDateStr = jsDate.property("toISOString").callWithInstance(jsDate).toString();
        jsUTCDateStr.remove(jsUTCDateStr.length() - 5, 4); // get rid of milliseconds (".000")
        if (qtUTCDateStr != jsUTCDateStr)
            QFAIL(qPrintable(jsDate.toString()));
        secs += 2*60*60;
    }
}

void tst_QJSEngine::dateConversionQtJS()
{
    QDateTime qtDate = QDateTime(QDate(2009, 1, 1));
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.toScriptValue(qtDate);
        QString jsUTCDateStr = jsDate.property("toISOString").callWithInstance(jsDate).toString();
        jsUTCDateStr.remove(jsUTCDateStr.length() - 5, 4); // get rid of milliseconds (".000")
        QString qtUTCDateStr = qtDate.toUTC().toString(Qt::ISODate);
        if (jsUTCDateStr != qtUTCDateStr)
            QFAIL(qPrintable(qtDate.toString()));
        qtDate = qtDate.addSecs(2*60*60);
    }
}

void tst_QJSEngine::functionPrototypeExtensions()
{
    // QJS adds connect and disconnect properties to Function.prototype.
    QJSEngine eng;
    QJSValue funProto = eng.globalObject().property("Function").property("prototype");
    QVERIFY(funProto.isCallable());
    QVERIFY(funProto.property("connect").isCallable());
    QVERIFY(funProto.property("disconnect").isCallable());

    // No properties should appear in for-in statements.
    QJSValue props = eng.evaluate("props = []; for (var p in Function.prototype) props.push(p); props");
    QVERIFY(props.isArray());
    QCOMPARE(props.property("length").toInt(), 0);
}

class ThreadedTestEngine : public QThread {
    Q_OBJECT;

public:
    int result;

    ThreadedTestEngine()
        : result(0) {}

    void run() {
        QJSEngine firstEngine;
        QJSEngine secondEngine;
        QJSValue value = firstEngine.evaluate("1");
        result = secondEngine.evaluate("1 + " + QString::number(value.toInt())).toInt();
    }
};

void tst_QJSEngine::threadedEngine()
{
    ThreadedTestEngine thread1;
    ThreadedTestEngine thread2;
    thread1.start();
    thread2.start();
    thread1.wait();
    thread2.wait();
    QCOMPARE(thread1.result, 2);
    QCOMPARE(thread2.result, 2);
}

void tst_QJSEngine::v8Context_simple()
{
    QJSEngine eng;

    v8::HandleScope handleScope;
    v8::Local<v8::Context> context = QT_PREPEND_NAMESPACE(qt_QJSEngineV8Context(&eng));
    v8::Context::Scope contextScope(context);

    v8::Local<v8::Script> script = v8::Script::Compile(
                v8::String::New("({ foo: 123, bar: 'ciao', baz: true })"));

    v8::TryCatch tc;
    v8::Local<v8::Value> result = script->Run();

    QVERIFY(!tc.HasCaught());
    QVERIFY(result->IsObject());

    v8::Local<v8::Object> object = result.As<v8::Object>();
    QVERIFY(object->Get(v8::String::New("foo"))->Equals(v8::Number::New(123)));
    QVERIFY(object->Get(v8::String::New("bar"))->Equals(v8::String::New("ciao")));
    QVERIFY(object->Get(v8::String::New("baz"))->IsTrue());
}

void tst_QJSEngine::v8Context_exception()
{
    QJSEngine eng;

    v8::HandleScope handleScope;
    v8::Local<v8::Context> context = qt_QJSEngineV8Context(&eng);
    v8::Context::Scope contextScope(context);

    int startLineNumber = 42;
    v8::ScriptOrigin origin(v8::String::New("test.js"), v8::Integer::New(startLineNumber));
    v8::Local<v8::Script> script = v8::Script::Compile(
                v8::String::New(
                    "function foo(i) {\n"
                    "  if (i > 5)\n"
                    "    throw Error('Catch me if you can');\n"
                    "  foo(i + 1);\n"
                    "}\n"
                    "foo(0);"),
                &origin);

// QJS does this for us:
//    v8::V8::SetCaptureStackTraceForUncaughtExceptions(true);

    v8::TryCatch tc;
    v8::Local<v8::Value> result = script->Run();

    QVERIFY(tc.HasCaught());
    QVERIFY(result.IsEmpty());

    v8::Local<v8::Message> message = tc.Message();
    QVERIFY(!message.IsEmpty());
    QCOMPARE(*v8::String::AsciiValue(message->Get()), "Uncaught Error: Catch me if you can");
    QCOMPARE(*v8::String::AsciiValue(message->GetScriptResourceName()), "test.js");
    QCOMPARE(message->GetLineNumber(), startLineNumber + 3);
}

void tst_QJSEngine::v8Context_mixAPIs()
{
    QJSEngine eng;

    v8::HandleScope handleScope;
    v8::Local<v8::Context> context = qt_QJSEngineV8Context(&eng);
    v8::Context::Scope contextScope(context);

    QJSValue globalQJS = eng.globalObject();
    v8::Local<v8::Value> globalV8Value = qt_QJSValueV8Value(globalQJS);
    QVERIFY(!globalV8Value.IsEmpty());
    QVERIFY(globalV8Value->IsObject());
    v8::Local<v8::Object> globalV8 = globalV8Value.As<v8::Object>();

    QVERIFY(globalQJS.property("foo").isUndefined());
    QVERIFY(globalV8->Get(v8::String::New("foo"))->IsUndefined());

    globalQJS.setProperty("foo", 123);
    QVERIFY(globalV8->Get(v8::String::New("foo"))->Equals(v8::Number::New(123)));

    globalV8->Set(v8::String::New("bar"), v8::String::New("ciao"));
    QVERIFY(globalQJS.property("bar").equals("ciao"));

    QJSValue arrayQJS = eng.newArray(10);
    v8::Local<v8::Value> arrayV8Value = qt_QJSValueV8Value(arrayQJS);
    QVERIFY(!arrayV8Value.IsEmpty());
    QVERIFY(arrayV8Value->IsArray());
    v8::Local<v8::Array> arrayV8 = arrayV8Value.As<v8::Array>();

    QCOMPARE(int(arrayV8->Length()), 10);
    arrayV8->Set(5, v8::Null());
    QVERIFY(arrayQJS.property(5).isNull());
}

QTEST_MAIN(tst_QJSEngine)

#include "tst_qjsengine.moc"

