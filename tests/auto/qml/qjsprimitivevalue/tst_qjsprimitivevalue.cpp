// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    Q_UNREACHABLE();
    return QString();
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

    Q_UNREACHABLE();
    return QString();
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
            break;
        case QJSPrimitiveValue::Null:
            QCOMPARE(var.typeId(), QMetaType::Nullptr);
            break;
        case QJSPrimitiveValue::Boolean:
            QCOMPARE(var.typeId(), QMetaType::Bool);
            QCOMPARE(var.toBool(), operand.toBoolean());
            break;
        case QJSPrimitiveValue::Integer:
            QCOMPARE(var.typeId(), QMetaType::Int);
            QCOMPARE(var.toInt(), operand.toInteger());
            break;
        case QJSPrimitiveValue::Double:
            QCOMPARE(var.typeId(), QMetaType::Double);
            QCOMPARE(var.toDouble(), operand.toDouble());
            break;
        case QJSPrimitiveValue::String:
            QCOMPARE(var.typeId(), QMetaType::QString);
            QCOMPARE(var.toString(), operand.toString());
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

QTEST_MAIN(tst_QJSPrimitiveValue)

#include "tst_qjsprimitivevalue.moc"
