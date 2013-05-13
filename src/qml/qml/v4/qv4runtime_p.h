/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#include "qv4global_p.h"
#include "qv4value_p.h"
#include "qv4math_p.h"


#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>

#include <cmath>
#include <cassert>
#include <limits>

//#include <wtf/MathExtras.h>

#ifdef DO_TRACE_INSTR
#  define TRACE1(x) fprintf(stderr, "    %s\n", __FUNCTION__);
#  define TRACE2(x, y) fprintf(stderr, "    %s\n", __FUNCTION__);
#else
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // TRACE1

QT_BEGIN_NAMESPACE

namespace QV4 {

enum TypeHint {
    PREFERREDTYPE_HINT,
    NUMBER_HINT,
    STRING_HINT
};

struct Function;
struct Object;
struct String;
struct ExecutionContext;
struct FunctionObject;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct DateObject;
struct RegExpObject;
struct ArrayObject;
struct ErrorObject;
struct ExecutionEngine;
struct InternalClass;

struct Q_QML_EXPORT Exception {
    explicit Exception(ExecutionContext *throwingContext, const Value &exceptionValue);
    ~Exception();

    void accept(ExecutionContext *catchingContext);

    void partiallyUnwindContext(ExecutionContext *catchingContext);

    Value value() const { return exception; }

private:
    ExecutionContext *throwingContext;
    bool accepted;
    PersistentValue exception;
};

}

QT_END_NAMESPACE

extern "C" {

// context
void __qmljs_call_activation_property(QV4::ExecutionContext *, QV4::Value *result, QV4::String *name, QV4::Value *args, int argc);
void __qmljs_call_property(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &that, QV4::String *name, QV4::Value *args, int argc);
void __qmljs_call_property_lookup(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &thisObject, uint index, QV4::Value *args, int argc);
void __qmljs_call_element(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &that, const QV4::Value &index, QV4::Value *args, int argc);
void __qmljs_call_value(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value *thisObject, const QV4::Value &func, QV4::Value *args, int argc);

void __qmljs_construct_activation_property(QV4::ExecutionContext *, QV4::Value *result, QV4::String *name, QV4::Value *args, int argc);
void __qmljs_construct_property(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &base, QV4::String *name, QV4::Value *args, int argc);
void __qmljs_construct_value(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &func, QV4::Value *args, int argc);

void __qmljs_builtin_typeof(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &val);
void __qmljs_builtin_typeof_name(QV4::ExecutionContext *context, QV4::Value* result, QV4::String *name);
void __qmljs_builtin_typeof_member(QV4::ExecutionContext* context, QV4::Value* result, const QV4::Value &base, QV4::String *name);
void __qmljs_builtin_typeof_element(QV4::ExecutionContext* context, QV4::Value *result, const QV4::Value &base, const QV4::Value &index);

void __qmljs_builtin_post_increment(QV4::Value *result, QV4::Value *val);
void __qmljs_builtin_post_increment_name(QV4::ExecutionContext *context, QV4::Value *result, QV4::String *name);
void __qmljs_builtin_post_increment_member(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &base, QV4::String *name);
void __qmljs_builtin_post_increment_element(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &base, const QV4::Value *index);

void __qmljs_builtin_post_decrement(QV4::Value *result, QV4::Value *val);
void __qmljs_builtin_post_decrement_name(QV4::ExecutionContext *context, QV4::Value *result, QV4::String *name);
void __qmljs_builtin_post_decrement_member(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &base, QV4::String *name);
void __qmljs_builtin_post_decrement_element(QV4::ExecutionContext *context, QV4::Value *result, const QV4::Value &base, const QV4::Value &index);

void Q_NORETURN __qmljs_builtin_throw(QV4::ExecutionContext *context, const QV4::Value &val);
void Q_NORETURN __qmljs_builtin_rethrow(QV4::ExecutionContext *context);
QV4::ExecutionContext *__qmljs_builtin_push_with_scope(const QV4::Value &o, QV4::ExecutionContext *ctx);
QV4::ExecutionContext *__qmljs_builtin_push_catch_scope(QV4::String *exceptionVarName, const QV4::Value &exceptionValue, QV4::ExecutionContext *ctx);
QV4::ExecutionContext *__qmljs_builtin_pop_scope(QV4::ExecutionContext *ctx);
void __qmljs_builtin_declare_var(QV4::ExecutionContext *ctx, bool deletable, QV4::String *name);
void __qmljs_builtin_define_property(QV4::ExecutionContext *ctx, const QV4::Value &object, QV4::String *name, QV4::Value *val);
void __qmljs_builtin_define_array(QV4::ExecutionContext *ctx, QV4::Value *array, QV4::Value *values, uint length);
void __qmljs_builtin_define_getter_setter(QV4::ExecutionContext *ctx, const QV4::Value &object, QV4::String *name, const QV4::Value *getter, const QV4::Value *setter);
void __qmljs_builtin_define_object_literal(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value *args, QV4::InternalClass *klass);

// constructors
void __qmljs_init_closure(QV4::ExecutionContext *ctx, QV4::Value *result, QV4::Function *clos);
QV4::Function *__qmljs_register_function(QV4::ExecutionContext *ctx, QV4::String *name,
                                         bool hasDirectEval,
                                         bool usesArgumentsObject, bool isStrict,
                                         bool hasNestedFunctions,
                                         QV4::String **formals, unsigned formalCount,
                                         QV4::String **locals, unsigned localCount);

// strings
Q_QML_EXPORT double __qmljs_string_to_number(const QString &s);
QV4::Value __qmljs_string_from_number(QV4::ExecutionContext *ctx, double number);
QV4::String *__qmljs_string_concat(QV4::ExecutionContext *ctx, QV4::String *first, QV4::String *second);

// objects
Q_QML_EXPORT QV4::Value __qmljs_object_default_value(QV4::Object *object, int typeHint);
void __qmljs_set_activation_property(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value& value);
void __qmljs_set_property(QV4::ExecutionContext *ctx, const QV4::Value &object, QV4::String *name, const QV4::Value &value);
void __qmljs_get_property(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &object, QV4::String *name);
void __qmljs_get_activation_property(QV4::ExecutionContext *ctx, QV4::Value *result, QV4::String *name);

void __qmljs_call_global_lookup(QV4::ExecutionContext *context, QV4::Value *result, uint index, QV4::Value *args, int argc);
void __qmljs_construct_global_lookup(QV4::ExecutionContext *context, QV4::Value *result, uint index, QV4::Value *args, int argc);


void __qmljs_get_element(QV4::ExecutionContext *ctx, QV4::Value *retval, const QV4::Value &object, const QV4::Value &index);
void __qmljs_set_element(QV4::ExecutionContext *ctx, const QV4::Value &object, const QV4::Value &index, const QV4::Value &value);

// For each
void __qmljs_foreach_iterator_object(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &in);
void __qmljs_foreach_next_property_name(QV4::Value *result, const QV4::Value &foreach_iterator);

// type conversion and testing
QV4::Value __qmljs_to_primitive(const QV4::Value &value, int typeHint);
QV4::Bool __qmljs_to_boolean(const QV4::Value &value);
double __qmljs_to_number(const QV4::Value &value);
QV4::Value __qmljs_to_string(const QV4::Value &value, QV4::ExecutionContext *ctx);
Q_QML_EXPORT QV4::String *__qmljs_convert_to_string(QV4::ExecutionContext *ctx, const QV4::Value &value);
void __qmljs_numberToString(QString *result, double num, int radix = 10);
QV4::Value __qmljs_to_object(QV4::ExecutionContext *ctx, const QV4::Value &value);
QV4::Object *__qmljs_convert_to_object(QV4::ExecutionContext *ctx, const QV4::Value &value);

QV4::Bool __qmljs_equal(const QV4::Value &x, const QV4::Value &y);
QV4::Bool __qmljs_strict_equal(const QV4::Value &x, const QV4::Value &y);

// unary operators
typedef void (*UnaryOpName)(QV4::Value *, const QV4::Value &);
void __qmljs_uplus(QV4::Value *result, const QV4::Value &value);
void __qmljs_uminus(QV4::Value *result, const QV4::Value &value);
void __qmljs_compl(QV4::Value *result, const QV4::Value &value);
void __qmljs_not(QV4::Value *result, const QV4::Value &value);
void __qmljs_increment(QV4::Value *result, const QV4::Value &value);
void __qmljs_decrement(QV4::Value *result, const QV4::Value &value);

void __qmljs_delete_subscript(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &base, const QV4::Value &index);
void __qmljs_delete_member(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &base, QV4::String *name);
void __qmljs_delete_name(QV4::ExecutionContext *ctx, QV4::Value *result, QV4::String *name);

void Q_NORETURN __qmljs_throw(QV4::ExecutionContext*, const QV4::Value &value);

// binary operators
typedef void (*BinOp)(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);

void __qmljs_instanceof(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_in(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_bit_or(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_bit_xor(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_bit_and(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_add(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_sub(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_mul(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_div(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_mod(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_shl(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_shr(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_ushr(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_gt(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_lt(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_ge(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_le(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_eq(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_ne(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_se(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);
void __qmljs_sne(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);

void __qmljs_add_helper(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right);


typedef void (*InplaceBinOpName)(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_bit_and_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_bit_or_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_bit_xor_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_add_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_sub_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_mul_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_div_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_mod_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_shl_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_shr_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);
void __qmljs_inplace_ushr_name(QV4::ExecutionContext *ctx, QV4::String *name, const QV4::Value &value);

typedef void (*InplaceBinOpElement)(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_bit_and_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_bit_or_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_bit_xor_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_add_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_sub_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_mul_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_div_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_mod_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_shl_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_shr_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);
void __qmljs_inplace_ushr_element(QV4::ExecutionContext *ctx, const QV4::Value &base, const QV4::Value &index, const QV4::Value &rhs);

typedef void (*InplaceBinOpMember)(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_bit_and_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_bit_or_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_bit_xor_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_add_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_sub_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_mul_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_div_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_mod_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_shl_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_shr_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);
void __qmljs_inplace_ushr_member(QV4::ExecutionContext *ctx, const QV4::Value &base, QV4::String *name, const QV4::Value &rhs);

typedef QV4::Bool (*CmpOp)(QV4::ExecutionContext *ctx, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_gt(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_lt(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_ge(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_le(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_eq(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_ne(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_se(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_sne(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_instanceof(QV4::ExecutionContext *ctx, const QV4::Value &left, const QV4::Value &right);
QV4::Bool __qmljs_cmp_in(QV4::ExecutionContext *ctx, const QV4::Value &left, const QV4::Value &right);

// type conversion and testing
inline QV4::Value __qmljs_to_primitive(const QV4::Value &value, int typeHint)
{
    QV4::Object *o = value.asObject();
    if (!o)
        return value;
    return __qmljs_object_default_value(o, typeHint);
}

inline double __qmljs_to_number(const QV4::Value &value)
{
    switch (value.type()) {
    case QV4::Value::Undefined_Type:
        return std::numeric_limits<double>::quiet_NaN();
    case QV4::Value::Null_Type:
        return 0;
    case QV4::Value::Boolean_Type:
        return (value.booleanValue() ? 1. : 0.);
    case QV4::Value::Integer_Type:
        return value.int_32;
    case QV4::Value::String_Type:
        return __qmljs_string_to_number(value.stringValue()->toQString());
    case QV4::Value::Object_Type: {
        QV4::Value prim = __qmljs_to_primitive(value, QV4::NUMBER_HINT);
        return __qmljs_to_number(prim);
    }
    default: // double
        return value.doubleValue();
    }
}

inline QV4::Value __qmljs_to_string(const QV4::Value &value, QV4::ExecutionContext *ctx)
{
    if (value.isString())
        return value;
    return QV4::Value::fromString(__qmljs_convert_to_string(ctx, value));
}

inline QV4::Value __qmljs_to_object(QV4::ExecutionContext *ctx, const QV4::Value &value)
{
    if (value.isObject())
        return value;
    return QV4::Value::fromObject(__qmljs_convert_to_object(ctx, value));
}


inline void __qmljs_uplus(QV4::Value *result, const QV4::Value &value)
{
    TRACE1(value);

    *result = value;
    if (result->tryIntegerConversion())
        return;

    double n = __qmljs_to_number(value);
    *result = QV4::Value::fromDouble(n);
}

inline void __qmljs_uminus(QV4::Value *result, const QV4::Value &value)
{
    TRACE1(value);

    // +0 != -0, so we need to convert to double when negating 0
    if (value.isInteger() && value.integerValue())
        *result = QV4::Value::fromInt32(-value.integerValue());
    else {
        double n = __qmljs_to_number(value);
        *result = QV4::Value::fromDouble(-n);
    }
}

inline void __qmljs_compl(QV4::Value *result, const QV4::Value &value)
{
    TRACE1(value);

    int n;
    if (value.isConvertibleToInt())
        n = value.int_32;
    else
        n = QV4::Value::toInt32(__qmljs_to_number(value));

    *result = QV4::Value::fromInt32(~n);
}

inline void __qmljs_not(QV4::Value *result, const QV4::Value &value)
{
    TRACE1(value);

    bool b = value.toBoolean();
    *result = QV4::Value::fromBoolean(!b);
}

// binary operators
inline void __qmljs_bit_or(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right)) {
        *result = QV4::Value::fromInt32(left.integerValue() | right.integerValue());
        return;
    }

    int lval = QV4::Value::toInt32(__qmljs_to_number(left));
    int rval = QV4::Value::toInt32(__qmljs_to_number(right));
    *result = QV4::Value::fromInt32(lval | rval);
}

inline void __qmljs_bit_xor(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right)) {
        *result = QV4::Value::fromInt32(left.integerValue() ^ right.integerValue());
        return;
    }

    int lval = QV4::Value::toInt32(__qmljs_to_number(left));
    int rval = QV4::Value::toInt32(__qmljs_to_number(right));
    *result = QV4::Value::fromInt32(lval ^ rval);
}

inline void __qmljs_bit_and(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right)) {
        *result = QV4::Value::fromInt32(left.integerValue() & right.integerValue());
        return;
    }

    int lval = QV4::Value::toInt32(__qmljs_to_number(left));
    int rval = QV4::Value::toInt32(__qmljs_to_number(right));
    *result = QV4::Value::fromInt32(lval & rval);
}

inline void __qmljs_add(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (QV4::Value::integerCompatible(left, right)) {
        *result = add_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    if (QV4::Value::bothDouble(left, right)) {
        *result = QV4::Value::fromDouble(left.doubleValue() + right.doubleValue());
        return;
    }

    __qmljs_add_helper(ctx, result, left, right);
}

inline void __qmljs_sub(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (QV4::Value::integerCompatible(left, right)) {
        *result = sub_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    *result = QV4::Value::fromDouble(lval - rval);
}

inline void __qmljs_mul(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

#ifdef QMLJS_INLINE_MATH
    if (QV4::Value::integerCompatible(left, right)) {
        *result = mul_int32(left.integerValue(), right.integerValue());
        return;
    }
#endif

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    *result = QV4::Value::fromDouble(lval * rval);
}

inline void __qmljs_div(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    *result = QV4::Value::fromDouble(lval / rval);
}

inline void __qmljs_mod(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right) && right.integerValue() != 0) {
        int intRes = left.integerValue() % right.integerValue();
        if (intRes != 0 || left.integerValue() >= 0) {
            *result = QV4::Value::fromInt32(intRes);
            return;
        }
    }

    double lval = __qmljs_to_number(left);
    double rval = __qmljs_to_number(right);
    *result = QV4::Value::fromDouble(fmod(lval, rval));
}

inline void __qmljs_shl(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right)) {
        *result = QV4::Value::fromInt32(left.integerValue() << ((uint(right.integerValue()) & 0x1f)));
        return;
    }

    int lval = QV4::Value::toInt32(__qmljs_to_number(left));
    unsigned rval = QV4::Value::toUInt32(__qmljs_to_number(right)) & 0x1f;
    *result = QV4::Value::fromInt32(lval << rval);
}

inline void __qmljs_shr(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    if (QV4::Value::integerCompatible(left, right)) {
        *result = QV4::Value::fromInt32(left.integerValue() >> ((uint(right.integerValue()) & 0x1f)));
        return;
    }

    int lval = QV4::Value::toInt32(__qmljs_to_number(left));
    unsigned rval = QV4::Value::toUInt32(__qmljs_to_number(right)) & 0x1f;
    *result = QV4::Value::fromInt32(lval >> rval);
}

inline void __qmljs_ushr(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    uint res;
    if (QV4::Value::integerCompatible(left, right)) {
        res = uint(left.integerValue()) >> (uint(right.integerValue()) & 0x1f);
    } else {
        unsigned lval = QV4::Value::toUInt32(__qmljs_to_number(left));
        unsigned rval = QV4::Value::toUInt32(__qmljs_to_number(right)) & 0x1f;
        res = lval >> rval;
    }

    if (res > INT_MAX)
        *result = QV4::Value::fromDouble(res);
    else
        *result = QV4::Value::fromInt32(res);
}

inline void __qmljs_gt(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(__qmljs_cmp_gt(ctx, left, right));
}

inline void __qmljs_lt(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(__qmljs_cmp_lt(ctx, left, right));
}

inline void __qmljs_ge(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(__qmljs_cmp_ge(ctx, left, right));
}

inline void __qmljs_le(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(__qmljs_cmp_le(ctx, left, right));
}

inline void __qmljs_eq(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(__qmljs_cmp_eq(ctx, left, right));
}

inline void __qmljs_ne(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    *result = QV4::Value::fromBoolean(!__qmljs_cmp_eq(ctx, left, right));
}

inline void __qmljs_se(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    bool r = __qmljs_strict_equal(left, right);
    *result = QV4::Value::fromBoolean(r);
}

inline void __qmljs_sne(QV4::ExecutionContext *, QV4::Value *result, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    bool r = ! __qmljs_strict_equal(left, right);
    *result = QV4::Value::fromBoolean(r);
}

inline QV4::Bool __qmljs_cmp_gt(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);
    if (QV4::Value::integerCompatible(left, right))
        return left.integerValue() > right.integerValue();

    QV4::Value l = __qmljs_to_primitive(left, QV4::NUMBER_HINT);
    QV4::Value r = __qmljs_to_primitive(right, QV4::NUMBER_HINT);

    if (QV4::Value::bothDouble(l, r)) {
        return l.doubleValue() > r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return r.stringValue()->compare(l.stringValue());
    } else {
        double dl = __qmljs_to_number(l);
        double dr = __qmljs_to_number(r);
        return dl > dr;
    }
}

inline QV4::Bool __qmljs_cmp_lt(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);
    if (QV4::Value::integerCompatible(left, right))
        return left.integerValue() < right.integerValue();

    QV4::Value l = __qmljs_to_primitive(left, QV4::NUMBER_HINT);
    QV4::Value r = __qmljs_to_primitive(right, QV4::NUMBER_HINT);

    if (QV4::Value::bothDouble(l, r)) {
        return l.doubleValue() < r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return l.stringValue()->compare(r.stringValue());
    } else {
        double dl = __qmljs_to_number(l);
        double dr = __qmljs_to_number(r);
        return dl < dr;
    }
}

inline QV4::Bool __qmljs_cmp_ge(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);
    if (QV4::Value::integerCompatible(left, right))
        return left.integerValue() >= right.integerValue();

    QV4::Value l = __qmljs_to_primitive(left, QV4::NUMBER_HINT);
    QV4::Value r = __qmljs_to_primitive(right, QV4::NUMBER_HINT);

    if (QV4::Value::bothDouble(l, r)) {
        return l.doubleValue() >= r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return !l.stringValue()->compare(r.stringValue());
    } else {
        double dl = __qmljs_to_number(l);
        double dr = __qmljs_to_number(r);
        return dl >= dr;
    }
}

inline QV4::Bool __qmljs_cmp_le(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);
    if (QV4::Value::integerCompatible(left, right))
        return left.integerValue() <= right.integerValue();

    QV4::Value l = __qmljs_to_primitive(left, QV4::NUMBER_HINT);
    QV4::Value r = __qmljs_to_primitive(right, QV4::NUMBER_HINT);

    if (QV4::Value::bothDouble(l, r)) {
        return l.doubleValue() <= r.doubleValue();
    } else if (l.isString() && r.isString()) {
        return !r.stringValue()->compare(l.stringValue());
    } else {
        double dl = __qmljs_to_number(l);
        double dr = __qmljs_to_number(r);
        return dl <= dr;
    }
}

inline QV4::Bool __qmljs_cmp_eq(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    // need to test for doubles first as NaN != NaN
    if (QV4::Value::bothDouble(left, right))
        return left.doubleValue() == right.doubleValue();
    if (left.val == right.val)
        return true;
    if (left.isString() && right.isString())
        return left.stringValue()->isEqualTo(right.stringValue());

    return __qmljs_equal(left, right);
}

inline QV4::Bool __qmljs_cmp_ne(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    return !__qmljs_cmp_eq(0, left, right);
}

inline QV4::Bool __qmljs_cmp_se(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    return __qmljs_strict_equal(left, right);
}

inline QV4::Bool __qmljs_cmp_sne(QV4::ExecutionContext *, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    return ! __qmljs_strict_equal(left, right);
}

inline QV4::Bool __qmljs_cmp_instanceof(QV4::ExecutionContext *ctx, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    QV4::Value v;
    __qmljs_instanceof(ctx, &v, left, right);
    return v.booleanValue();
}

inline uint __qmljs_cmp_in(QV4::ExecutionContext *ctx, const QV4::Value &left, const QV4::Value &right)
{
    TRACE2(left, right);

    QV4::Value v;
    __qmljs_in(ctx, &v, left, right);
    return v.booleanValue();
}

} // extern "C"

#endif // QMLJS_RUNTIME_H
