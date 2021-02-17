/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QtCore/qobject.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qjsprimitivevalue.h>

#include <QtTest/qtest.h>

class tst_QJSPrimitiveValue : public QObject
{
    Q_OBJECT

private slots:
    void operators_data();
    void operators();

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
        }
        return value.toString();
    }
}

void tst_QJSPrimitiveValue::operators_data()
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

enum Operator {
    Add, Sub, Mul, Div, Eq, SEq, NEq, SNEq, GT, LT, GEq, LEq, Mod
};

QString toString(Operator op) {
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

template<typename LHS, typename RHS, typename Result, Operator op>
void doTestOperator(QJSEngine *engine, LHS lhs, RHS rhs)
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

template<typename LHS, typename RHS>
void doTestForAllOperators(QJSEngine *engine, LHS lhs, RHS rhs)
{
    doTestOperator<LHS, RHS, QJSPrimitiveValue, Add>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, QJSPrimitiveValue, Sub>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, QJSPrimitiveValue, Mul>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, QJSPrimitiveValue, Div>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, QJSPrimitiveValue, Mod>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, Eq>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, SEq>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, NEq>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, SNEq>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, GT>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, LT>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, GEq>(engine, lhs, rhs);
    doTestOperator<LHS, RHS, bool, LEq>(engine, lhs, rhs);
}

void tst_QJSPrimitiveValue::operators()
{
    QFETCH(QJSPrimitiveValue, lhs);
    QFETCH(QJSPrimitiveValue, rhs);

    doTestForAllOperators(&engine, lhs,  rhs);
    doTestForAllOperators(&engine, lhs.toString(), rhs);
    doTestForAllOperators(&engine, lhs, rhs.toString());
    doTestForAllOperators(&engine, lhs.toString() + " bar", rhs);
    doTestForAllOperators(&engine, lhs, rhs.toString() + " bar");
    doTestForAllOperators(&engine, "foo" + lhs.toString(), rhs);
    doTestForAllOperators(&engine, lhs, "foo" + rhs.toString());
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
