// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qjsprimitivevalue.h>

#include <QtTest/qtest.h>

class tst_QJSPrimitiveValue : public QObject
{
    Q_OBJECT

private slots:
    void binaryOperators_data();
    void binaryOperators();

    void unaryOperators_data();
    void unaryOperators();

    void toFromVariant();
    void coercion();

    void ctor_invalid();
    void ctor_undefinedWithEngine();
    void ctor_boolWithEngine();
    void ctor_intWithEngine();
    void ctor_stringWithEngine();
    void ctor_copyAndAssignWithEngine();
    void toString();
    void toNumber();
    void toBoolean();
    void toVariant();
    void equals();
    void strictlyEquals();
    void stringAndUrl();

    void negativeNullMult();

private:
    QJSEngine engine;

    const QList<QJSPrimitiveValue> operands = {
        QJSPrimitiveNull(), QJSPrimitiveUndefined(), true, false,
        std::numeric_limits<int>::min(), -10, -1, 0, 1, 10, std::numeric_limits<int>::max(),
        -std::numeric_limits<double>::infinity(), -100.1, -1.2, -0.0, 0.0, 1.2, 100.1,
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()
    };
};

template<typename T>
QString toScriptString(T value)
{
    if constexpr (std::is_same_v<T, QString>) {
        return u'"' + value + u'"';
    } else {
        if (value.type() == QJSPrimitiveValue::Double) {
            // -0 is rendered as "0" in QJSValue's "toString()".
            // However, we need the sign when constructing an expression to be evaluated.
            const double result = value.toDouble();
            if (qIsNull(result)) {
                if (std::signbit(result))
                    return QStringLiteral("-0.0");
                else
                    return QStringLiteral("0.0");
            }
        } else if (value.type() == QJSPrimitiveValue::String) {
            return u'"' + value.toString() + u'"';
        }
        return value.toString();
    }
}

void tst_QJSPrimitiveValue::binaryOperators_data()
{
    QTest::addColumn<QJSPrimitiveValue>("lhs");
    QTest::addColumn<QJSPrimitiveValue>("rhs");

    for (QJSPrimitiveValue l : operands) {
        for (QJSPrimitiveValue r : operands)
            QTest::newRow(qPrintable(toScriptString(l) + " x " + toScriptString(r))) << l << r ;
    }
}

#define VERBOSECOMPARE(actual, expected) \
do {\
    if (!QTest::qCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) { \
        qDebug() << "In" << Q_FUNC_INFO; \
        return;\
    } \
} while (false)

#define VERBOSEVERIFY(condition, expression, js) \
    QVERIFY2(condition, qPrintable(expression + " -> " + js.toString()))

enum BinaryOperator {
    Add, Sub, Mul, Div, Eq, SEq, NEq, SNEq, GT, LT, GEq, LEq, Mod
};

QString toString(BinaryOperator op) {
    switch (op) {
    case Add:  return "+";
    case Sub:  return "-";
    case Mul:  return "*";
    case Div:  return "/";
    case Eq:   return "==";
    case SEq:  return "===";
    case NEq:  return "!=";
    case SNEq: return "!==";
    case GT:   return ">";
    case LT:   return "<";
    case GEq:  return ">=";
    case LEq:  return "<=";
    case Mod:  return "%";
    }

    Q_UNREACHABLE_RETURN(QString());
}

template<typename Result>
void verifyResult(Result result, QJSEngine *engine, const QString &expression)
{
    const QJSValue js = engine->evaluate(expression);

    if constexpr (std::is_same_v<Result, bool>) {
        VERBOSEVERIFY(js.isBool(), expression, js);
        VERBOSECOMPARE(js.toBool(), result);
    } else if constexpr (std::is_same_v<Result, QString>) {
        VERBOSEVERIFY(js.isString(), expression, js);
        VERBOSECOMPARE(result, js.toString());
    } else {
        switch (result.type()) {
        case QJSPrimitiveValue::Undefined:
            VERBOSEVERIFY(js.isUndefined(), expression, js);
            break;
        case QJSPrimitiveValue::Null:
            VERBOSEVERIFY(js.isNull(), expression, js);
            break;
        case QJSPrimitiveValue::Boolean:
            VERBOSEVERIFY(js.isBool(), expression, js);
            break;
        case QJSPrimitiveValue::Integer:
            VERBOSEVERIFY(js.isNumber(), expression, js);
            break;
        case QJSPrimitiveValue::Double:
            VERBOSEVERIFY(js.isNumber(), expression, js);
            break;
        case QJSPrimitiveValue::String:
            VERBOSEVERIFY(js.isString(), expression, js);
            break;
        default:
            QFAIL("unexpected type");
        }

        VERBOSECOMPARE(result.toBoolean(), js.toBool());
        VERBOSECOMPARE(result.toInteger(), js.toInt());
        VERBOSECOMPARE(result.toDouble(), js.toNumber());
        VERBOSECOMPARE(result.toString(), js.toString());
    }
}

template<typename LHS, typename RHS, typename Result, BinaryOperator op>
void doTestBinaryOperator(QJSEngine *engine, LHS lhs, RHS rhs)
{
    Result result;

    if constexpr (op == Add)
        result = lhs + rhs;
    else if constexpr (op == Sub)
        result = lhs - rhs;
    else if constexpr (op == Mul)
        result = lhs * rhs;
    else if constexpr (op == Div)
        result = lhs / rhs;
    else if constexpr (op == Eq)
        result = QJSPrimitiveValue(lhs).equals(rhs);
    else if constexpr (op == SEq)
        result = lhs == rhs;
    else if constexpr (op == NEq)
        result = !QJSPrimitiveValue(lhs).equals(rhs);
    else if constexpr (op == SNEq)
        result = lhs != rhs;
    else if constexpr (op == GT)
        result = lhs > rhs;
    else if constexpr (op == LT)
        result = lhs < rhs;
    else if constexpr (op == GEq)
        result = lhs >= rhs;
    else if constexpr (op == LEq)
        result = lhs <= rhs;
    else if constexpr (op == Mod)
        result = lhs % rhs;
    else
        QFAIL("Unkonwn operator");

    const QString expression = toScriptString(lhs) + " " + toString(op) + " " + toScriptString(rhs);
    verifyResult(result, engine, expression);
}

template<typename LHS, typename RHS>
void doTestForAllBinaryOperators(QJSEngine *engine, LHS lhs, RHS rhs)
{
    doTestBinaryOperator<LHS, RHS, QJSPrimitiveValue, Add>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, QJSPrimitiveValue, Sub>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, QJSPrimitiveValue, Mul>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, QJSPrimitiveValue, Div>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, QJSPrimitiveValue, Mod>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, Eq>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, SEq>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, NEq>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, SNEq>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, GT>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, LT>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, GEq>(engine, lhs, rhs);
    doTestBinaryOperator<LHS, RHS, bool, LEq>(engine, lhs, rhs);
}

void tst_QJSPrimitiveValue::binaryOperators()
{
    QFETCH(QJSPrimitiveValue, lhs);
    QFETCH(QJSPrimitiveValue, rhs);

    doTestForAllBinaryOperators(&engine, lhs,  rhs);
    doTestForAllBinaryOperators(&engine, lhs.toString(), rhs);
    doTestForAllBinaryOperators(&engine, lhs, rhs.toString());
    doTestForAllBinaryOperators(&engine, lhs.toString() + " bar", rhs);
    doTestForAllBinaryOperators(&engine, lhs, rhs.toString() + " bar");
    doTestForAllBinaryOperators(&engine, "foo" + lhs.toString(), rhs);
    doTestForAllBinaryOperators(&engine, lhs, "foo" + rhs.toString());
}

void tst_QJSPrimitiveValue::unaryOperators_data()
{
    QTest::addColumn<QJSPrimitiveValue>("operand");

    for (const QJSPrimitiveValue &o : operands)
        QTest::newRow(qPrintable(toScriptString(o))) << o;
}

enum UnaryOperator {
    Plus, Minus, PreIncrement, PostIncrement, PreDecrement, PostDecrement
};

QString toString(UnaryOperator op) {
    switch (op) {
    case Plus:
        return "+";
    case Minus:
        return "-";
    case PreIncrement:
    case PostIncrement:
        return "++";
    case PreDecrement:
    case PostDecrement:
        return "--";
    }

    Q_UNREACHABLE_RETURN(QString());
}

template<typename Result, UnaryOperator Operator>
void doTestUnaryOperator(QJSEngine *engine, QJSPrimitiveValue operand)
{
    Result result;

    // Need to get the string here as the operators change the original value
    const QString asString = toScriptString(operand);

    if constexpr (Operator == Plus)
        result = +operand;
    else if constexpr (Operator == Minus)
        result = -operand;
    else if constexpr (Operator == PreIncrement)
        result = ++operand;
    else if constexpr (Operator == PostIncrement)
        result = operand++;
    else if constexpr (Operator == PreDecrement)
        result = --operand;
    else if constexpr (Operator == PostDecrement)
        result = operand--;
    else
        QFAIL("Unkonwn operator");

    const QString decl = "var a = " + asString + "; ";
    const QString expression = decl + ((Operator == PostIncrement || Operator == PostDecrement)
            ? (u'a' + toString(Operator))
            : (toString(Operator) + u'a'));
    verifyResult(result, engine, expression);
}

void doTestForAllUnaryOperators(QJSEngine *engine, QJSPrimitiveValue operand)
{
    doTestUnaryOperator<QJSPrimitiveValue, Plus>(engine, operand);
    doTestUnaryOperator<QJSPrimitiveValue, Minus>(engine, operand);
    doTestUnaryOperator<QJSPrimitiveValue, PreIncrement>(engine, operand);
    doTestUnaryOperator<QJSPrimitiveValue, PostIncrement>(engine, operand);
    doTestUnaryOperator<QJSPrimitiveValue, PostDecrement>(engine, operand);
}

void tst_QJSPrimitiveValue::unaryOperators()
{
    QFETCH(QJSPrimitiveValue, operand);

    doTestForAllUnaryOperators(&engine, operand);
    doTestForAllUnaryOperators(&engine, operand.toString());
    doTestForAllUnaryOperators(&engine, operand.toString() + " bar");
    doTestForAllUnaryOperators(&engine, "foo" + operand.toString());
}

void tst_QJSPrimitiveValue::toFromVariant()
{
    for (const auto &operand : operands) {
        const QVariant var = operand.toVariant();
        switch (operand.type()) {
        case QJSPrimitiveValue::Undefined:
            QVERIFY(!var.isValid());
            QCOMPARE(operand.metaType(), QMetaType());
            QCOMPARE(operand.data(), nullptr);
            break;
        case QJSPrimitiveValue::Null:
            QCOMPARE(var.typeId(), QMetaType::Nullptr);
            QCOMPARE(operand.metaType(), QMetaType::fromType<std::nullptr_t>());
            QCOMPARE(operand.data(), nullptr);
            break;
        case QJSPrimitiveValue::Boolean:
            QCOMPARE(var.typeId(), QMetaType::Bool);
            QCOMPARE(var.toBool(), operand.toBoolean());
            QCOMPARE(operand.metaType(), QMetaType::fromType<bool>());
            QCOMPARE(*static_cast<const bool *>(operand.data()), operand.toBoolean());
            break;
        case QJSPrimitiveValue::Integer:
            QCOMPARE(var.typeId(), QMetaType::Int);
            QCOMPARE(var.toInt(), operand.toInteger());
            QCOMPARE(operand.metaType(), QMetaType::fromType<int>());
            QCOMPARE(*static_cast<const int *>(operand.data()), operand.toInteger());
            break;
        case QJSPrimitiveValue::Double:
            QCOMPARE(var.typeId(), QMetaType::Double);
            QCOMPARE(var.toDouble(), operand.toDouble());
            QCOMPARE(operand.metaType(), QMetaType::fromType<double>());
            QCOMPARE(*static_cast<const double *>(operand.data()), operand.toDouble());
            break;
        case QJSPrimitiveValue::String:
            QCOMPARE(var.typeId(), QMetaType::QString);
            QCOMPARE(var.toString(), operand.toString());
            QCOMPARE(operand.metaType(), QMetaType::fromType<QString>());
            QCOMPARE(*static_cast<const QString *>(operand.data()), operand.toString());
            break;
        }

        QJSPrimitiveValue fromVar(var);
        QCOMPARE(fromVar.type(), operand.type());
        if (operand.type() == QJSPrimitiveValue::Double && std::isnan(operand.toDouble()))
            QVERIFY(fromVar != operand);
        else
            QCOMPARE(fromVar, operand);
    }
}

void tst_QJSPrimitiveValue::coercion()
{
    for (const QJSPrimitiveValue &operand : operands) {
        QCOMPARE(operand.to<QJSPrimitiveValue::Undefined>(), QJSPrimitiveUndefined());
        QCOMPARE(operand.to<QJSPrimitiveValue::Null>(), QJSPrimitiveNull());
        QCOMPARE(operand.to<QJSPrimitiveValue::Boolean>(), operand.toBoolean());
        QCOMPARE(operand.to<QJSPrimitiveValue::Integer>(), operand.toInteger());
        QCOMPARE(operand.to<QJSPrimitiveValue::String>(), operand.toString());

        const QJSPrimitiveValue lhs = operand.to<QJSPrimitiveValue::Double>();
        QCOMPARE(lhs.type(), QJSPrimitiveValue::Double);
        const double rhs = operand.toDouble();
        if (std::isnan(rhs))
            QVERIFY(std::isnan(lhs.toDouble()));
        else
            QCOMPARE(lhs.toDouble(), rhs);
    }
}

void tst_QJSPrimitiveValue::ctor_invalid()
{
    QJSPrimitiveValue v;
    QCOMPARE(v.type(), QJSPrimitiveValue::Undefined);
}

void tst_QJSPrimitiveValue::ctor_undefinedWithEngine()
{
    QJSEngine eng;
    QJSPrimitiveValue v(eng.toPrimitiveValue(QVariant()));
    QCOMPARE(v.type(), QJSPrimitiveValue::Undefined);
}

void tst_QJSPrimitiveValue::ctor_boolWithEngine()
{
    QJSEngine eng;
    QJSPrimitiveValue v(eng.toPrimitiveValue(false));
    QCOMPARE(v.type(), QJSPrimitiveValue::Boolean);
    QCOMPARE(v.toBoolean(), false);
}

void tst_QJSPrimitiveValue::ctor_intWithEngine()
{
    QJSEngine eng;
    QJSPrimitiveValue v(eng.toPrimitiveValue(int(1)));
    QCOMPARE(v.type(), QJSPrimitiveValue::Integer);
    QCOMPARE(v.toInteger(), 1);
}

void tst_QJSPrimitiveValue::ctor_stringWithEngine()
{
    QJSEngine eng;
    QJSPrimitiveValue v(eng.toPrimitiveValue(QStringLiteral("ciao")));
    QCOMPARE(v.type(), QJSPrimitiveValue::String);
    QCOMPARE(v.toString(), QStringLiteral("ciao"));
}

void tst_QJSPrimitiveValue::ctor_copyAndAssignWithEngine()
{
    QJSEngine eng;
    // copy constructor, operator=

    QJSPrimitiveValue v(eng.toPrimitiveValue(1.0));
    QJSPrimitiveValue v2(v);
    QCOMPARE(v2.strictlyEquals(v), true);

    QJSPrimitiveValue v3(v);
    QCOMPARE(v3.strictlyEquals(v), true);
    QCOMPARE(v3.strictlyEquals(v2), true);

    QJSPrimitiveValue v4(eng.toPrimitiveValue(2.0));
    QCOMPARE(v4.strictlyEquals(v), false);
    v3 = QJSPrimitiveValue(v4);
    QCOMPARE(v3.strictlyEquals(v), false);
    QCOMPARE(v3.strictlyEquals(v4), true);

    v2 = QJSPrimitiveValue();
    QCOMPARE(v2.strictlyEquals(v), false);
    QCOMPARE(v.toDouble(), 1.0);

    QJSPrimitiveValue v5(v);
    QCOMPARE(v5.strictlyEquals(v), true);
    v = QJSPrimitiveValue();
    QCOMPARE(v5.strictlyEquals(v), false);
    QCOMPARE(v5.toDouble(), 1.0);
}

void tst_QJSPrimitiveValue::toString()
{
    QJSEngine eng;

    {
        QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
        QCOMPARE(undefined.toString(), QStringLiteral("undefined"));
        QCOMPARE(qjsvalue_cast<QString>(undefined), QStringLiteral("undefined"));
    }

    {
        QJSPrimitiveValue null((QJSPrimitiveNull()));
        QCOMPARE(null.toString(), QStringLiteral("null"));
        QCOMPARE(qjsvalue_cast<QString>(null), QStringLiteral("null"));
    }

    {
        QJSPrimitiveValue falskt(eng.toPrimitiveValue(false));
        QCOMPARE(falskt.toString(), QStringLiteral("false"));
        QCOMPARE(qjsvalue_cast<QString>(falskt), QStringLiteral("false"));

        QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
        QCOMPARE(sant.toString(), QStringLiteral("true"));
        QCOMPARE(qjsvalue_cast<QString>(sant), QStringLiteral("true"));
    }
    {
        QJSPrimitiveValue number(eng.toPrimitiveValue(123));
        QCOMPARE(number.toString(), QStringLiteral("123"));
        QCOMPARE(qjsvalue_cast<QString>(number), QStringLiteral("123"));
    }
    {
        QJSPrimitiveValue number(eng.toPrimitiveValue(6.37e-8));
        QCOMPARE(number.toString(), QStringLiteral("6.37e-8"));
    }
    {
        QJSPrimitiveValue number(eng.toPrimitiveValue(-6.37e-8));
        QCOMPARE(number.toString(), QStringLiteral("-6.37e-8"));

        QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("ciao")));
        QCOMPARE(str.toString(), QStringLiteral("ciao"));
        QCOMPARE(qjsvalue_cast<QString>(str), QStringLiteral("ciao"));
    }

    QJSPrimitiveValue inv((QJSPrimitiveUndefined()));
    QCOMPARE(inv.toString(), QStringLiteral("undefined"));

    // Type cannot be represented in QJSPrimitiveValue, and is converted to string.
    QJSPrimitiveValue variant(eng.toPrimitiveValue(QPoint(10, 20)));
    QCOMPARE(variant.type(), QJSPrimitiveValue::String);
    QCOMPARE(variant.toString(), QStringLiteral("QPoint(10, 20)"));
    variant = eng.toPrimitiveValue(QUrl());
    QCOMPARE(variant.type(), QJSPrimitiveValue::String);
    QVERIFY(variant.toString().isEmpty());

    {
        QByteArray hello = QByteArrayLiteral("Hello World");
        QJSPrimitiveValue jsValue(eng.toPrimitiveValue(hello));
        QCOMPARE(jsValue.toString(), QString::fromUtf8(hello));
    }
}

void tst_QJSPrimitiveValue::toNumber()
{
    QJSEngine eng;

    QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
    QCOMPARE(qIsNaN(undefined.toDouble()), true);
    QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(undefined)), true);

    QJSPrimitiveValue null((QJSPrimitiveNull()));
    QCOMPARE(null.toDouble(), 0.0);
    QCOMPARE(qjsvalue_cast<qreal>(null), 0.0);

    {
        QJSPrimitiveValue falskt(eng.toPrimitiveValue(false));
        QCOMPARE(falskt.toDouble(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
        QCOMPARE(sant.toDouble(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSPrimitiveValue number(eng.toPrimitiveValue(123.0));
        QCOMPARE(number.toDouble(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("ciao")));
        QCOMPARE(qIsNaN(str.toDouble()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSPrimitiveValue str2(eng.toPrimitiveValue(QStringLiteral("123")));
        QCOMPARE(str2.toDouble(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }

    QJSPrimitiveValue inv((QJSPrimitiveUndefined()));
    QVERIFY(qIsNaN(inv.toDouble()));
    QVERIFY(qIsNaN(qjsvalue_cast<qreal>(inv)));

    // V2 constructors
    {
        QJSPrimitiveValue falskt(false);
        QCOMPARE(falskt.toDouble(), 0.0);
        QCOMPARE(qjsvalue_cast<qreal>(falskt), 0.0);

        QJSPrimitiveValue sant(true);
        QCOMPARE(sant.toDouble(), 1.0);
        QCOMPARE(qjsvalue_cast<qreal>(sant), 1.0);

        QJSPrimitiveValue number(123.0);
        QCOMPARE(number.toDouble(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(number), 123.0);

        QJSPrimitiveValue number2(int(0x43211234));
        QCOMPARE(number2.toDouble(), 1126240820.0);

        QJSPrimitiveValue str(QStringLiteral("ciao"));
        QCOMPARE(qIsNaN(str.toDouble()), true);
        QCOMPARE(qIsNaN(qjsvalue_cast<qreal>(str)), true);

        QJSPrimitiveValue str2(QStringLiteral("123"));
        QCOMPARE(str2.toDouble(), 123.0);
        QCOMPARE(qjsvalue_cast<qreal>(str2), 123.0);
    }
}

void tst_QJSPrimitiveValue::toBoolean()
{
    QJSEngine eng;

    QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
    QCOMPARE(undefined.type(), QJSPrimitiveValue::Undefined);
    QCOMPARE(undefined.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(undefined), false);

    QJSPrimitiveValue null((QJSPrimitiveNull()));
    QCOMPARE(null.type(), QJSPrimitiveValue::Null);
    QCOMPARE(null.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(null), false);

    {
        QJSPrimitiveValue falskt = eng.toPrimitiveValue(false);
        QCOMPARE(falskt.type(), QJSPrimitiveValue::Boolean);
        QCOMPARE(falskt.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
        QCOMPARE(sant.type(), QJSPrimitiveValue::Boolean);
        QCOMPARE(sant.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSPrimitiveValue number(eng.toPrimitiveValue(0.0));
        QCOMPARE(number.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSPrimitiveValue number2(eng.toPrimitiveValue(qQNaN()));
        QCOMPARE(number2.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSPrimitiveValue number3(eng.toPrimitiveValue(123.0));
        QCOMPARE(number3.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSPrimitiveValue number4(eng.toPrimitiveValue(-456.0));
        QCOMPARE(number4.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("")));
        QCOMPARE(str.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSPrimitiveValue str2(eng.toPrimitiveValue(QStringLiteral("123")));
        QCOMPARE(str2.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }

    QJSPrimitiveValue inv((QJSPrimitiveUndefined()));
    QCOMPARE(inv.toBoolean(), false);
    QCOMPARE(qjsvalue_cast<bool>(inv), false);

    // V2 constructors
    {
        QJSPrimitiveValue falskt(false);
        QCOMPARE(falskt.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(falskt), false);

        QJSPrimitiveValue sant(true);
        QCOMPARE(sant.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(sant), true);

        QJSPrimitiveValue number(0.0);
        QCOMPARE(number.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number), false);

        QJSPrimitiveValue number2(qQNaN());
        QCOMPARE(number2.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(number2), false);

        QJSPrimitiveValue number3(123.0);
        QCOMPARE(number3.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number3), true);

        QJSPrimitiveValue number4(-456.0);
        QCOMPARE(number4.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(number4), true);

        QJSPrimitiveValue number5(0x43211234);
        QCOMPARE(number5.toBoolean(), true);

        QJSPrimitiveValue str(QStringLiteral(""));
        QCOMPARE(str.toBoolean(), false);
        QCOMPARE(qjsvalue_cast<bool>(str), false);

        QJSPrimitiveValue str2(QStringLiteral("123"));
        QCOMPARE(str2.toBoolean(), true);
        QCOMPARE(qjsvalue_cast<bool>(str2), true);
    }
}

void tst_QJSPrimitiveValue::toVariant()
{
    QJSEngine eng;

    {
        QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
        QCOMPARE(undefined.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(undefined), QVariant());
    }

    {
        QJSPrimitiveValue null((QJSPrimitiveNull()));
        QCOMPARE(null.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(qjsvalue_cast<QVariant>(null), QVariant::fromValue(nullptr));
    }

    {
        QJSPrimitiveValue number(eng.toPrimitiveValue(123.0));
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSPrimitiveValue intNumber(eng.toPrimitiveValue(qint32(123)));
        QCOMPARE(intNumber.toVariant().typeId(), QVariant((qint32)123).typeId());
        QCOMPARE((qjsvalue_cast<QVariant>(intNumber)).typeId(),
                 QVariant(qint32(123)).typeId());

        QJSPrimitiveValue falskt(eng.toPrimitiveValue(false));
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("ciao")));
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QStringLiteral("ciao")));
    }

    {
        QDateTime dateTime = QDate(1980, 10, 4).startOfDay();
        QJSPrimitiveValue dateObject(eng.toPrimitiveValue(dateTime));
        QVariant var = dateObject.toVariant();
        QCOMPARE(var, (eng.coerceValue<QDateTime, QString>(dateTime)));
        QCOMPARE(dateObject.toVariant(), var);
    }

    {
        QRegularExpression rx = QRegularExpression(QStringLiteral("[0-9a-z]+"));
        QJSPrimitiveValue rxObject(eng.toPrimitiveValue(rx));
        QCOMPARE(rxObject.type(), QJSPrimitiveValue::String);
        QVariant var = rxObject.toVariant();
        QCOMPARE(var, u'/' + rx.pattern() + u'/');
    }

    {
        QJSPrimitiveValue inv;
        QCOMPARE(inv.toVariant(), QVariant());
        QCOMPARE(inv.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(inv), QVariant());
    }

    // V2 constructors
    {
        QJSPrimitiveValue number(123.0);
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(number.toVariant(), QVariant(123.0));
        QCOMPARE(qjsvalue_cast<QVariant>(number), QVariant(123.0));

        QJSPrimitiveValue falskt(false);
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(falskt.toVariant(), QVariant(false));
        QCOMPARE(qjsvalue_cast<QVariant>(falskt), QVariant(false));

        QJSPrimitiveValue sant(true);
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(sant.toVariant(), QVariant(true));
        QCOMPARE(qjsvalue_cast<QVariant>(sant), QVariant(true));

        QJSPrimitiveValue str(QStringLiteral("ciao"));
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(str.toVariant(), QVariant(QStringLiteral("ciao")));
        QCOMPARE(qjsvalue_cast<QVariant>(str), QVariant(QStringLiteral("ciao")));

        QJSPrimitiveValue undef((QJSPrimitiveUndefined()));
        QCOMPARE(undef.toVariant(), QVariant());
        QCOMPARE(undef.toVariant(), QVariant());
        QCOMPARE(qjsvalue_cast<QVariant>(undef), QVariant());

        QJSPrimitiveValue nil((QJSPrimitiveNull()));
        QCOMPARE(nil.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(nil.toVariant(), QVariant::fromValue(nullptr));
        QCOMPARE(qjsvalue_cast<QVariant>(nil), QVariant::fromValue(nullptr));
    }
}

void tst_QJSPrimitiveValue::equals()
{
    QJSEngine eng;
    QObject temp;

    QVERIFY(QJSPrimitiveValue().equals(QJSPrimitiveValue()));

    QJSPrimitiveValue num(eng.toPrimitiveValue(123));
    QVERIFY(num.equals(eng.toPrimitiveValue(123)));
    QVERIFY(!num.equals(eng.toPrimitiveValue(321)));
    QVERIFY(num.equals(eng.toPrimitiveValue(QStringLiteral("123"))));
    QVERIFY(!num.equals(eng.toPrimitiveValue(QStringLiteral("321"))));
    QVERIFY(!num.equals(QJSPrimitiveValue()));

    QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("123")));
    QVERIFY(str.equals(eng.toPrimitiveValue(QStringLiteral("123"))));
    QVERIFY(!str.equals(eng.toPrimitiveValue(QStringLiteral("321"))));
    QVERIFY(str.equals(eng.toPrimitiveValue(123)));
    QVERIFY(!str.equals(eng.toPrimitiveValue(321)));
    QCOMPARE(str.equals(QJSPrimitiveValue()), false);

    QJSPrimitiveValue num2(123);
    QVERIFY(num2.equals(QJSPrimitiveValue(123)));
    QVERIFY(!num2.equals(QJSPrimitiveValue(321)));
    QVERIFY(num2.equals(QStringLiteral("123")));
    QVERIFY(!num2.equals(QStringLiteral("321")));
    QVERIFY(!num2.equals(QJSPrimitiveValue()));

    QJSPrimitiveValue str2(QStringLiteral("123"));
    QVERIFY(str2.equals(QStringLiteral("123")));
    QVERIFY(!str2.equals(QStringLiteral("321")));
    QVERIFY(str2.equals(QJSPrimitiveValue(123)));
    QVERIFY(!str2.equals(QJSPrimitiveValue(321)));
    QVERIFY(!str2.equals(QJSPrimitiveValue()));

    QJSPrimitiveValue date1(eng.toPrimitiveValue(QDate(2000, 1, 1).startOfDay()));
    QJSPrimitiveValue date2(eng.toPrimitiveValue(QDate(1999, 1, 1).startOfDay()));
    QCOMPARE(date1.equals(date2), false);
    QCOMPARE(date1.equals(date1), true);
    QCOMPARE(date2.equals(date2), true);

    QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
    QJSPrimitiveValue null((QJSPrimitiveNull()));
    QCOMPARE(undefined.equals(undefined), true);
    QCOMPARE(null.equals(null), true);
    QCOMPARE(undefined.equals(null), true);
    QCOMPARE(null.equals(undefined), true);
    QVERIFY(undefined.equals(QJSPrimitiveValue()));
    QVERIFY(null.equals(QJSPrimitiveValue()));
    QVERIFY(!null.equals(num));
    QVERIFY(!undefined.equals(num));

    QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
    QVERIFY(sant.equals(eng.toPrimitiveValue(1)));
    QVERIFY(sant.equals(eng.toPrimitiveValue(QStringLiteral("1"))));
    QVERIFY(sant.equals(sant));
    QVERIFY(!sant.equals(eng.toPrimitiveValue(0)));
    QVERIFY(!sant.equals(undefined));
    QVERIFY(!sant.equals(null));

    QJSPrimitiveValue falskt(eng.toPrimitiveValue(false));
    QVERIFY(falskt.equals(eng.toPrimitiveValue(0)));
    QVERIFY(falskt.equals(eng.toPrimitiveValue(QStringLiteral("0"))));
    QVERIFY(falskt.equals(falskt));
    QVERIFY(!falskt.equals(sant));
    QVERIFY(!falskt.equals(undefined));
    QVERIFY(!falskt.equals(null));

    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QVERIFY(var1.equals(var2));
    }
    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant::fromValue(QPoint(3, 4))));
        QVERIFY(!var1.equals(var2));
    }
}

void tst_QJSPrimitiveValue::strictlyEquals()
{
    QJSEngine eng;
    QObject temp;

    QVERIFY(QJSPrimitiveValue().strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue num(eng.toPrimitiveValue(123));
    QVERIFY(num.strictlyEquals(eng.toPrimitiveValue(123)));
    QVERIFY(!num.strictlyEquals(eng.toPrimitiveValue(321)));
    QVERIFY(!num.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("123"))));
    QVERIFY(!num.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("321"))));
    QVERIFY(!num.strictlyEquals(QJSPrimitiveValue()));
    QVERIFY(!QJSPrimitiveValue().strictlyEquals(num));

    QJSPrimitiveValue str(eng.toPrimitiveValue(QStringLiteral("123")));
    QVERIFY(str.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("123"))));
    QVERIFY(!str.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("321"))));
    QVERIFY(!str.strictlyEquals(eng.toPrimitiveValue(123)));
    QVERIFY(!str.strictlyEquals(eng.toPrimitiveValue(321)));
    QVERIFY(!str.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue num2(123);
    QVERIFY(num2.strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!num2.strictlyEquals(QJSPrimitiveValue(321)));
    QVERIFY(!num2.strictlyEquals(QStringLiteral("123")));
    QVERIFY(!num2.strictlyEquals(QStringLiteral("321")));
    QVERIFY(!num2.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue str2(QStringLiteral("123"));
    QVERIFY(str2.strictlyEquals(QStringLiteral("123")));
    QVERIFY(!str2.strictlyEquals(QStringLiteral("321")));
    QVERIFY(!str2.strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!str2.strictlyEquals(QJSPrimitiveValue(321)));
    QVERIFY(!str2.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue date1(eng.toPrimitiveValue(QDate(2000, 1, 1).startOfDay()));
    QJSPrimitiveValue date2(eng.toPrimitiveValue(QDate(1999, 1, 1).startOfDay()));
    QCOMPARE(date1.strictlyEquals(date2), false);
    QCOMPARE(date1.strictlyEquals(date1), true);
    QCOMPARE(date2.strictlyEquals(date2), true);
    QVERIFY(!date1.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue undefined(eng.toPrimitiveValue(QVariant()));
    QJSPrimitiveValue null((QJSPrimitiveNull()));
    QCOMPARE(undefined.strictlyEquals(undefined), true);
    QCOMPARE(null.strictlyEquals(null), true);
    QCOMPARE(undefined.strictlyEquals(null), false);
    QCOMPARE(null.strictlyEquals(undefined), false);
    QVERIFY(!null.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue sant(eng.toPrimitiveValue(true));
    QVERIFY(!sant.strictlyEquals(eng.toPrimitiveValue(1)));
    QVERIFY(!sant.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("1"))));
    QVERIFY(sant.strictlyEquals(sant));
    QVERIFY(!sant.strictlyEquals(eng.toPrimitiveValue(0)));
    QVERIFY(!sant.strictlyEquals(undefined));
    QVERIFY(!sant.strictlyEquals(null));
    QVERIFY(!sant.strictlyEquals(QJSPrimitiveValue()));

    QJSPrimitiveValue falskt(eng.toPrimitiveValue(false));
    QVERIFY(!falskt.strictlyEquals(eng.toPrimitiveValue(0)));
    QVERIFY(!falskt.strictlyEquals(eng.toPrimitiveValue(QStringLiteral("0"))));
    QVERIFY(falskt.strictlyEquals(falskt));
    QVERIFY(!falskt.strictlyEquals(sant));
    QVERIFY(!falskt.strictlyEquals(undefined));
    QVERIFY(!falskt.strictlyEquals(null));
    QVERIFY(!falskt.strictlyEquals(QJSPrimitiveValue()));

    QVERIFY(!QJSPrimitiveValue(false).strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!QJSPrimitiveValue(QJSPrimitiveUndefined()).strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!QJSPrimitiveValue(QJSPrimitiveNull()).strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!QJSPrimitiveValue(false).strictlyEquals({QStringLiteral("ciao")}));
    QVERIFY(!QJSPrimitiveValue(QJSPrimitiveUndefined()).strictlyEquals({QStringLiteral("ciao")}));
    QVERIFY(!QJSPrimitiveValue(QJSPrimitiveNull()).strictlyEquals({QStringLiteral("ciao")}));
    QVERIFY(eng.toPrimitiveValue(QStringLiteral("ciao")).strictlyEquals(QJSPrimitiveValue(QStringLiteral("ciao"))));
    QVERIFY(QJSPrimitiveValue(QStringLiteral("ciao")).strictlyEquals(eng.toPrimitiveValue(QStringLiteral("ciao"))));
    QVERIFY(!QJSPrimitiveValue(QStringLiteral("ciao")).strictlyEquals(QJSPrimitiveValue(123)));
    QVERIFY(!QJSPrimitiveValue(QStringLiteral("ciao")).strictlyEquals(eng.toPrimitiveValue(123)));
    QVERIFY(!QJSPrimitiveValue(123).strictlyEquals({QStringLiteral("ciao")}));
    QVERIFY(!QJSPrimitiveValue(123).strictlyEquals(eng.toPrimitiveValue(QStringLiteral("ciao"))));
    QVERIFY(!eng.toPrimitiveValue(123).strictlyEquals(QJSPrimitiveValue(QStringLiteral("ciao"))));

    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant(QStringList() << QStringLiteral("a"))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant(QStringList() << QStringLiteral("a"))));
        QVERIFY(var1.strictlyEquals(var2));
    }
    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant(QStringList() << QStringLiteral("a"))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant(QStringList() << QStringLiteral("b"))));
        QVERIFY(!var1.strictlyEquals(var2));
    }
    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QVERIFY(var1.strictlyEquals(var2));
    }
    {
        QJSPrimitiveValue var1(eng.toPrimitiveValue(QVariant::fromValue(QPoint(1, 2))));
        QJSPrimitiveValue var2(eng.toPrimitiveValue(QVariant::fromValue(QPoint(3, 4))));
        QVERIFY(!var1.strictlyEquals(var2));
    }
}

void tst_QJSPrimitiveValue::stringAndUrl()
{
    QJSEngine engine;
    const QString string = QStringLiteral("http://example.com/something.html");
    const QUrl url(string);

    const QJSPrimitiveValue urlValue(engine.toPrimitiveValue(url));
    QCOMPARE(urlValue.type(), QJSPrimitiveValue::String);
    QCOMPARE(urlValue.toString(), string);
    QCOMPARE(engine.fromPrimitiveValue<QUrl>(urlValue), url);

    const QJSPrimitiveValue stringValue(engine.toPrimitiveValue(string));
    QCOMPARE(stringValue.toString(), string);
    QCOMPARE(engine.fromPrimitiveValue<QUrl>(stringValue), url);

    const QJSPrimitiveValue immediateStringValue(string);
    QCOMPARE(immediateStringValue.toString(), string);
    QCOMPARE(engine.fromPrimitiveValue<QUrl>(immediateStringValue), url);
}

void tst_QJSPrimitiveValue::negativeNullMult()
{
    QJSPrimitiveValue zero(0);
    QJSPrimitiveValue negative(-1);
    QJSPrimitiveValue positive(1);

    QCOMPARE((zero * negative).type(), QJSPrimitiveValue::Double);
    QCOMPARE((negative * zero).type(), QJSPrimitiveValue::Double);

    QCOMPARE((zero * positive).type(), QJSPrimitiveValue::Integer);
    QCOMPARE((positive * zero).type(), QJSPrimitiveValue::Integer);
}

QTEST_MAIN(tst_QJSPrimitiveValue)

#include "tst_qjsprimitivevalue.moc"
