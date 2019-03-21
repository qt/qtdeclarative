/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QJSEngine>
#include <private/qjsvalue_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4script_p.h>

class EnvVarSaver
{
public:
    EnvVarSaver(const char *name, const QByteArray &newValue)
        : _name(name)
    {
        _wasSet = qEnvironmentVariableIsSet(name);
        if (_wasSet)
            _oldValue = qgetenv(name);
        qputenv(name, newValue);
    }

    ~EnvVarSaver()
    {
        if (_wasSet)
            qputenv(_name, _oldValue);
        else
            qunsetenv(_name);
    }

private:
    const char *_name;
    bool _wasSet;
    QByteArray _oldValue;
};

class tst_v4traced : public QObject
{
    Q_OBJECT

private slots:
    void collectTraces_data();
    void collectTraces();

    void binopI32deopt_data();
    void binopI32deopt();

    void calls_data();
    void calls();

    void setLookup();
    void construct();
};

void tst_v4traced::collectTraces_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("tracePointCount");
    QTest::addColumn<int>("interestingTracePoint");
    QTest::addColumn<quint8>("expectedBits");

    QTest::newRow("int+")    << "var a = 4; a + 2"     << 2 << 1 << quint8(QV4::ObservedTraceValues::Integer);
    QTest::newRow("double+") << "var a = 4.1; a + 1.9" << 2 << 1 << quint8(QV4::ObservedTraceValues::Double);
    QTest::newRow("object+") << "var a = '4'; a + '2'" << 2 << 1 << quint8(QV4::ObservedTraceValues::Other);
}

void tst_v4traced::collectTraces()
{
    QFETCH(QString, code);
    QFETCH(int, tracePointCount);
    QFETCH(int, interestingTracePoint);
    QFETCH(quint8, expectedBits);

    EnvVarSaver forceInterpreter("QV4_FORCE_INTERPRETER", "1");
    EnvVarSaver forceTracing("QV4_FORCE_TRACING", "1");

    QV4::ExecutionEngine vm;
    QV4::Scope scope(&vm);
    QV4::ScopedContext ctx(scope, vm.rootContext());
    QV4::ScopedValue result(scope);
    QScopedPointer<QV4::Script> script;
    script.reset(new QV4::Script(ctx, QV4::Compiler::ContextType::Global, code, "collectTraces"));
    script->parseAsBinding = false;

    QVERIFY(!scope.engine->hasException);
    script->parse();
    QVERIFY(!scope.engine->hasException);

    QVERIFY(script->function()->tracingEnabled());
    result = script->run();
    QVERIFY(!scope.engine->hasException);

    QCOMPARE(int(script->function()->compiledFunction->nTraceInfos), tracePointCount);
    QCOMPARE(*script->function()->traceInfo(interestingTracePoint), expectedBits);
}

void tst_v4traced::binopI32deopt_data()
{
    QTest::addColumn<QString>("operand");
    QTest::addColumn<int>("int_arg1");
    QTest::addColumn<int>("int_arg2");
    QTest::addColumn<int>("int_result");
    QTest::addColumn<QString>("other_arg1");
    QTest::addColumn<QString>("other_arg2");
    QTest::addColumn<double>("other_result");

    QTest::newRow("+") << "+" << 1 << 2 << 3 << "1.1" << "1.9" << 3.0;
    QTest::newRow("-") << "-" << 3 << 2 << 1 << "3.1" << "2.1" << 1.0;
    QTest::newRow("*") << "*" << 2 << 3 << 6 << "2.1" << "1.9" << 3.99;
    QTest::newRow("/") << "/" << 6 << 3 << 2 << "6.6" << "3.3" << 2.0;

    QTest::newRow("&") << "&" << 6 << 3 << 2 << "'6'" << "'3'" << 2.0;
    QTest::newRow("|") << "|" << 6 << 3 << 7 << "'6'" << "'3'" << 7.0;
    QTest::newRow("^") << "^" << 6 << 3 << 5 << "'6'" << "'3'" << 5.0;

    QTest::newRow("<<") << "<<" << 5 << 1 << 10 << "'5'" << "'1'" << 10.0;
    QTest::newRow(">>") << ">>" << -1 << 1 << -1 << "'-1'" << "'1'" << -1.0;
    QTest::newRow(">>>") << ">>>" << -1 << 1 << 0x7FFFFFFF << "'-1'" << "'1'" << 2147483647.0;

    QTest::newRow("==") << "==" << 2 << 1 << 0 << "'2'" << "'1'" << 0.0;
    QTest::newRow("!=") << "!=" << 2 << 1 << 1 << "'2'" << "'1'" << 1.0;
    QTest::newRow("<" ) << "<"  << 2 << 1 << 0 << "'2'" << "'1'" << 0.0;
    QTest::newRow("<=") << "<=" << 2 << 1 << 0 << "'2'" << "'1'" << 0.0;
    QTest::newRow(">" ) << ">"  << 2 << 1 << 1 << "'2'" << "'1'" << 1.0;
    QTest::newRow(">=") << ">=" << 2 << 1 << 1 << "'2'" << "'1'" << 1.0;
}

void tst_v4traced::binopI32deopt()
{
    QFETCH(QString, operand);
    QFETCH(int, int_arg1);
    QFETCH(int, int_arg2);
    QFETCH(int, int_result);
    QFETCH(QString, other_arg1);
    QFETCH(QString, other_arg2);
    QFETCH(double, other_result);

    QString func = QStringLiteral("function binopI32(a, b) { return a %1 b }").arg(operand);
    QString intCall = QStringLiteral("binopI32(%1, %2)").arg(int_arg1).arg(int_arg2);
    QString otherCall = QStringLiteral("binopI32(%1, %2)").arg(other_arg1).arg(other_arg2);

    QJSEngine engine;
    engine.evaluate(func);

    QCOMPARE(engine.evaluate(intCall).toInt(), int_result); // interpret + trace
    QCOMPARE(engine.evaluate(intCall).toInt(), int_result); // jit
    QCOMPARE(engine.evaluate(otherCall).toNumber(), other_result); // deopt
    QCOMPARE(engine.evaluate(otherCall).toNumber(), other_result); // retrace
    QCOMPARE(engine.evaluate(otherCall).toNumber(), other_result); // rejit
}

void tst_v4traced::calls_data()
{
    QTest::addColumn<QString>("call");

    QTest::newRow("callGlobalLookup") << "globalLookup";
    QTest::newRow("callPropertyLookup") << "obj.propertyLookup";
}

class Calls
{
public:
    static int callCount;

    static QV4::ReturnedValue doSomething(const QV4::FunctionObject */*o*/,
                                          const QV4::Value */*thiz*/,
                                          const QV4::Value *argv, int argc)
    {
        ++callCount;

        if (argc == 0)
            return QV4::Encode(42);

        int prod = 1;
        for (int i = 0; i < argc; ++i) {
            Q_ASSERT(argv[i].isInteger());
            prod *= argv[i].int_32();
        }
        return QV4::Encode(prod);
    }
};

int Calls::callCount = 0;

void tst_v4traced::calls()
{
    QFETCH(QString, call);

    EnvVarSaver forceTracing("QV4_FORCE_TRACING", "1");
    EnvVarSaver jitCallThreshold("QV4_JIT_CALL_THRESHOLD", "1");

    QV4::ExecutionEngine vm;
    QV4::Scope scope(&vm);
    QV4::ScopedContext ctx(scope, vm.rootContext());
    vm.globalObject->defineDefaultProperty(QLatin1String("globalLookup"),
                                           Calls::doSomething);
    QV4::ScopedObject obj(scope, vm.newObject());
    vm.globalObject->defineDefaultProperty(QLatin1String("obj"), obj);
    obj->defineDefaultProperty("propertyLookup", Calls::doSomething);

    QString code = QStringLiteral(
            "function doCalls() {\n"
            "  if (%1() != 42) return false\n"
            "  if (%1(21, 2) != 42) return false\n"
            "  if (%1(2, 3, 7) != 42) return false\n"
            "  return true\n"
            "}\n"
            "var result = true\n"
            "for (var i = 0; i < 10; ++i) {\n"
            "  if (!doCalls()) { result = false; break }"
            "}\n"
            "result\n").arg(call);
    QScopedPointer<QV4::Script> script;
    script.reset(new QV4::Script(ctx, QV4::Compiler::ContextType::Global, code, "call"));
    script->parseAsBinding = false;

    QVERIFY(!scope.engine->hasException);
    script->parse();
    QVERIFY(!scope.engine->hasException);

    Calls::callCount = 0;
    QV4::ScopedValue result(scope, script->run());
    QVERIFY(!scope.engine->hasException);

    QVERIFY(result->isBoolean());
    QVERIFY(result->booleanValue());
    QCOMPARE(Calls::callCount, 30);
}

void tst_v4traced::setLookup()
{
    EnvVarSaver forceTracing("QV4_FORCE_TRACING", "1");
    EnvVarSaver jitCallThreshold("QV4_JIT_CALL_THRESHOLD", "1");

    QV4::ExecutionEngine vm;
    QV4::Scope scope(&vm);
    QV4::ScopedContext ctx(scope, vm.rootContext());
    QV4::ScopedObject obj(scope, vm.newObject());
    vm.globalObject->defineDefaultProperty(QLatin1String("oracle"), obj);
    obj->defineDefaultProperty("answer", QV4::Primitive::fromInt32(32));

    QString code = QStringLiteral(
                "function doit() {\n"
                "  ++oracle.answer\n"
                "}\n"
                "for (var i = 0; i < 10; ++i) doit()\n"
                "oracle.answer\n");
    QScopedPointer<QV4::Script> script;
    script.reset(new QV4::Script(ctx, QV4::Compiler::ContextType::Global, code, "setLookup"));
    script->parseAsBinding = false;

    QVERIFY(!scope.engine->hasException);
    script->parse();
    QVERIFY(!scope.engine->hasException);

    QV4::ScopedValue result(scope, script->run());
    QVERIFY(!scope.engine->hasException);

    QVERIFY(result->isInteger());
    QCOMPARE(result->int_32(), 42);
}

void tst_v4traced::construct()
{
    EnvVarSaver forceTracing("QV4_FORCE_TRACING", "1");
    EnvVarSaver jitCallThreshold("QV4_JIT_CALL_THRESHOLD", "1");

    QV4::ExecutionEngine vm;
    QV4::Scope scope(&vm);
    QV4::ScopedContext ctx(scope, vm.rootContext());
    QV4::ScopedObject obj(scope, vm.newObject());
    vm.globalObject->defineDefaultProperty(QLatin1String("oracle"), obj);
    obj->defineDefaultProperty("answer", QV4::Primitive::fromInt32(32));

    QString code = QStringLiteral(
                "function doit() {\n"
                "  this.arr = new Array()\n"
                "  this.arr[0] = 0\n"
                "  this.arr[1] = 1\n"
                "  this.arr[2] = 2\n"
                "}\n"
                "var o\n"
                "for (var i = 0; i < 10; ++i) o = new doit()\n"
                "o.arr\n");
    QScopedPointer<QV4::Script> script;
    script.reset(new QV4::Script(ctx, QV4::Compiler::ContextType::Global, code, "setLookup"));
    script->parseAsBinding = false;

    QVERIFY(!scope.engine->hasException);
    script->parse();
    QVERIFY(!scope.engine->hasException);

    QV4::ScopedValue result(scope, script->run());
    QVERIFY(!scope.engine->hasException);

    QVERIFY(result->as<QV4::ArrayObject>());
}

QTEST_MAIN(tst_v4traced)

#include "tst_v4traced.moc"
