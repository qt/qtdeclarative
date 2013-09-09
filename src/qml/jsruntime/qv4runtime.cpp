/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4objectproto_p.h"
#include "qv4globalobject_p.h"
#include "qv4stringobject_p.h"
#include "qv4argumentsobject_p.h"
#include "qv4lookup_p.h"
#include "qv4function_p.h"
#include "qv4exception_p.h"
#include "private/qlocale_tools_p.h"
#include "qv4scopedvalue_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>
#include <typeinfo>
#include <stdlib.h>

#include "../../../3rdparty/double-conversion/double-conversion.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

void __qmljs_numberToString(QString *result, double num, int radix)
{
    Q_ASSERT(result);

    if (std::isnan(num)) {
        *result = QStringLiteral("NaN");
        return;
    } else if (qIsInf(num)) {
        *result = QLatin1String(num < 0 ? "-Infinity" : "Infinity");
        return;
    }

    if (radix == 10) {
        char str[100];
        double_conversion::StringBuilder builder(str, sizeof(str));
        double_conversion::DoubleToStringConverter::EcmaScriptConverter().ToShortest(num, &builder);
        *result = QString::fromLatin1(builder.Finalize());
        return;
    }

    result->clear();
    bool negative = false;

    if (num < 0) {
        negative = true;
        num = -num;
    }

    double frac = num - ::floor(num);
    num = Value::toInteger(num);

    do {
        char c = (char)::fmod(num, radix);
        c = (c < 10) ? (c + '0') : (c - 10 + 'a');
        result->prepend(QLatin1Char(c));
        num = ::floor(num / radix);
    } while (num != 0);

    if (frac != 0) {
        result->append(QLatin1Char('.'));
        do {
            frac = frac * radix;
            char c = (char)::floor(frac);
            c = (c < 10) ? (c + '0') : (c - 10 + 'a');
            result->append(QLatin1Char(c));
            frac = frac - ::floor(frac);
        } while (frac != 0);
    }

    if (negative)
        result->prepend(QLatin1Char('-'));
}

void __qmljs_init_closure(ExecutionContext *ctx, ValueRef result, int functionId)
{
    QV4::Function *clos = ctx->compilationUnit->runtimeFunctions[functionId];
    assert(clos);
    *result = Value::fromObject(FunctionObject::creatScriptFunction(ctx, clos));
}

void __qmljs_delete_subscript(ExecutionContext *ctx, ValueRef result, const Value &base, const Value &index)
{
    if (Object *o = base.asObject()) {
        uint n = index.asArrayIndex();
        if (n < UINT_MAX) {
            Value res = Value::fromBoolean(o->deleteIndexedProperty(n));
            if (result)
                *result = res;
            return;
        }
    }

    String *name = index.toString(ctx);
    __qmljs_delete_member(ctx, result, base, name);
}

void __qmljs_delete_member(ExecutionContext *ctx, ValueRef result, const Value &base, String *name)
{
    Object *obj = base.toObject(ctx);
    Value res = Value::fromBoolean(obj->deleteProperty(name));
    if (result)
        *result = res;
}

void __qmljs_delete_name(ExecutionContext *ctx, ValueRef result, String *name)
{
    Value res = Value::fromBoolean(ctx->deleteProperty(name));
    if (result)
        *result = res;
}

void __qmljs_add_helper(ExecutionContext *ctx, ValueRef result, const ValueRef left, const ValueRef right)
{
    ValueScope scope(ctx);

    ScopedValue pleft(scope, __qmljs_to_primitive(*left, PREFERREDTYPE_HINT));
    ScopedValue pright(scope, __qmljs_to_primitive(*right, PREFERREDTYPE_HINT));
    if (pleft->isString() || pright->isString()) {
        if (!pleft->isString())
            pleft = __qmljs_to_string(pleft, ctx);
        if (!pright->isString())
            pright = __qmljs_to_string(pright, ctx);
        String *string = __qmljs_string_concat(ctx, pleft->stringValue(), pright->stringValue());
        *result = Value::fromString(string);
        return;
    }
    double x = __qmljs_to_number(pleft);
    double y = __qmljs_to_number(pright);
    *result = Value::fromDouble(x + y);
}

void __qmljs_instanceof(ExecutionContext *ctx, ValueRef result, const Value &left, const Value &right)
{
    Object *o = right.asObject();
    if (!o)
        ctx->throwTypeError();

    bool r = o->hasInstance(left);
    *result = Value::fromBoolean(r);
}

void __qmljs_in(ExecutionContext *ctx, ValueRef result, const Value &left, const Value &right)
{
    if (!right.isObject())
        ctx->throwTypeError();
    String *s = left.toString(ctx);
    bool r = right.objectValue()->__hasProperty__(s);
    *result = Value::fromBoolean(r);
}

void inplaceBitOp(ExecutionContext *ctx, String *name, const Value &value, BinOp op)
{
    Value lhs = ctx->getProperty(name);
    Value result;
    op(&result, lhs, value);
    ctx->setProperty(name, result);
}


void __qmljs_inplace_bit_and_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_bit_and);
}

void __qmljs_inplace_bit_or_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_bit_or);
}

void __qmljs_inplace_bit_xor_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_bit_xor);
}

void __qmljs_inplace_add_name(ExecutionContext *ctx, String *name, const Value &value)
{
    Value lhs = ctx->getProperty(name);
    Value result;
    __qmljs_add(ctx, &result, lhs, value);
    ctx->setProperty(name, result);
}

void __qmljs_inplace_sub_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_sub);
}

void __qmljs_inplace_mul_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_mul);
}

void __qmljs_inplace_div_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_div);
}

void __qmljs_inplace_mod_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_mod);
}

void __qmljs_inplace_shl_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_shl);
}

void __qmljs_inplace_shr_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_shr);
}

void __qmljs_inplace_ushr_name(ExecutionContext *ctx, String *name, const Value &value)
{
    inplaceBitOp(ctx, name, value, __qmljs_ushr);
}

void __qmljs_inplace_bit_and_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_bit_and, index, rhs);
}

void __qmljs_inplace_bit_or_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_bit_or, index, rhs);
}

void __qmljs_inplace_bit_xor_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_bit_xor, index, rhs);
}

void __qmljs_inplace_add_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_add, index, rhs);
}

void __qmljs_inplace_sub_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_sub, index, rhs);
}

void __qmljs_inplace_mul_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_mul, index, rhs);
}

void __qmljs_inplace_div_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_div, index, rhs);
}

void __qmljs_inplace_mod_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_mod, index, rhs);
}

void __qmljs_inplace_shl_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_shl, index, rhs);
}

void __qmljs_inplace_shr_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_shr, index, rhs);
}

void __qmljs_inplace_ushr_element(ExecutionContext *ctx, const Value &base, const Value &index, const Value &rhs)
{
    Object *obj = base.toObject(ctx);
    obj->inplaceBinOp(ctx, __qmljs_ushr, index, rhs);
}

void __qmljs_inplace_bit_and_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_bit_and, name, rhs);
}

void __qmljs_inplace_bit_or_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_bit_or, name, rhs);
}

void __qmljs_inplace_bit_xor_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_bit_xor, name, rhs);
}

void __qmljs_inplace_add_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_add, name, rhs);
}

void __qmljs_inplace_sub_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_sub, name, rhs);
}

void __qmljs_inplace_mul_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_mul, name, rhs);
}

void __qmljs_inplace_div_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_div, name, rhs);
}

void __qmljs_inplace_mod_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_mod, name, rhs);
}

void __qmljs_inplace_shl_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_shl, name, rhs);
}

void __qmljs_inplace_shr_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_shr, name, rhs);
}

void __qmljs_inplace_ushr_member(ExecutionContext *ctx, const Value &base, String *name, const Value &rhs)
{
    Object *o = base.toObject(ctx);
    o->inplaceBinOp(ctx, __qmljs_ushr, name, rhs);
}

double __qmljs_string_to_number(const QString &string)
{
    QString s = string.trimmed();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(0, 16);
    bool ok;
    QByteArray ba = s.toLatin1();
    const char *begin = ba.constData();
    const char *end = 0;
    double d = qstrtod(begin, &end, &ok);
    if (end - begin != ba.size()) {
        if (ba == "Infinity" || ba == "+Infinity")
            d = Q_INFINITY;
        else if (ba == "-Infinity")
            d = -Q_INFINITY;
        else
            d = std::numeric_limits<double>::quiet_NaN();
    }
    return d;
}

Value __qmljs_string_from_number(ExecutionContext *ctx, double number)
{
    QString qstr;
    __qmljs_numberToString(&qstr, number, 10);
    String *string = ctx->engine->newString(qstr);
    return Value::fromString(string);
}

String *__qmljs_string_concat(ExecutionContext *ctx, String *first, String *second)
{
    const QString &a = first->toQString();
    const QString &b = second->toQString();
    QString newStr(a.length() + b.length(), Qt::Uninitialized);
    QChar *data = newStr.data();
    memcpy(data, a.constData(), a.length()*sizeof(QChar));
    data += a.length();
    memcpy(data, b.constData(), b.length()*sizeof(QChar));

    return ctx->engine->newString(newStr);
}

Value __qmljs_object_default_value(Object *object, int typeHint)
{
    if (typeHint == PREFERREDTYPE_HINT) {
        if (object->asDateObject())
            typeHint = STRING_HINT;
        else
            typeHint = NUMBER_HINT;
    }

    ExecutionEngine *engine = object->internalClass->engine;
    String *meth1 = engine->newString("toString");
    String *meth2 = engine->newString("valueOf");

    if (typeHint == NUMBER_HINT)
        qSwap(meth1, meth2);

    ExecutionContext *ctx = engine->current;

    Value conv = object->get(meth1);
    if (FunctionObject *o = conv.asFunctionObject()) {
        ScopedCallData callData(engine, 0);
        callData->thisObject = Value::fromObject(object);
        Value r = o->call(callData);
        if (r.isPrimitive())
            return r;
    }

    conv = object->get(meth2);
    if (FunctionObject *o = conv.asFunctionObject()) {
        ScopedCallData callData(engine, 0);
        callData->thisObject = Value::fromObject(object);
        Value r = o->call(callData);
        if (r.isPrimitive())
            return r;
    }

    ctx->throwTypeError();
    return Value::undefinedValue();
}

Bool __qmljs_to_boolean(const Value &value)
{
    return value.toBoolean();
}


Object *__qmljs_convert_to_object(ExecutionContext *ctx, const Value &value)
{
    assert(!value.isObject());
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        ctx->throwTypeError();
    case Value::Boolean_Type:
        return ctx->engine->newBooleanObject(value);
    case Value::String_Type:
        return ctx->engine->newStringObject(value);
        break;
    case Value::Object_Type:
        Q_UNREACHABLE();
    case Value::Integer_Type:
    default: // double
        return ctx->engine->newNumberObject(value);
    }
}

String *__qmljs_convert_to_string(ExecutionContext *ctx, const Value &value)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return ctx->engine->id_undefined;
    case Value::Null_Type:
        return ctx->engine->id_null;
    case Value::Boolean_Type:
        if (value.booleanValue())
            return ctx->engine->id_true;
        else
            return ctx->engine->id_false;
    case Value::String_Type:
        return value.stringValue();
    case Value::Object_Type: {
        Value prim = __qmljs_to_primitive(value, STRING_HINT);
        if (prim.isPrimitive())
            return __qmljs_convert_to_string(ctx, prim);
        else
            ctx->throwTypeError();
    }
    case Value::Integer_Type:
        return __qmljs_string_from_number(ctx, value.int_32).stringValue();
    default: // double
        return __qmljs_string_from_number(ctx, value.doubleValue()).stringValue();
    } // switch
}

void __qmljs_set_property(ExecutionContext *ctx, const Value &object, String *name, const Value &value)
{
    Object *o = object.toObject(ctx);
    o->put(name, value);
}

void __qmljs_get_element(ExecutionContext *ctx, ValueRef result, const Value &object, const Value &index)
{
    uint idx = index.asArrayIndex();

    Object *o = object.asObject();
    if (!o) {
        if (idx < UINT_MAX) {
            if (String *str = object.asString()) {
                if (idx >= (uint)str->toQString().length()) {
                    if (result)
                        *result = Value::undefinedValue();
                    return;
                }
                const QString s = str->toQString().mid(idx, 1);
                if (result)
                    *result = Value::fromString(ctx, s);
                return;
            }
        }

        if (object.isNull() || object.isUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(index.toQString()).arg(object.toQString());
            ctx->throwTypeError(message);
        }

        o = __qmljs_convert_to_object(ctx, object);
    }

    if (idx < UINT_MAX) {
        uint pidx = o->propertyIndexFromArrayIndex(idx);
        if (pidx < UINT_MAX) {
            if (!o->arrayAttributes || o->arrayAttributes[pidx].isData()) {
                if (result)
                    *result = o->arrayData[pidx].value;
                return;
            }
        }

        Value res = o->getIndexed(idx);
        if (result)
            *result = res;
        return;
    }

    String *name = index.toString(ctx);
    Value res = o->get(name);
    if (result)
        *result = res;
}

void __qmljs_set_element(ExecutionContext *ctx, const Value &object, const Value &index, const Value &value)
{
    Object *o = object.toObject(ctx);

    uint idx = index.asArrayIndex();
    if (idx < UINT_MAX) {
        uint pidx = o->propertyIndexFromArrayIndex(idx);
        if (pidx < UINT_MAX) {
            if (o->arrayAttributes && !o->arrayAttributes[pidx].isEmpty() && !o->arrayAttributes[pidx].isWritable()) {
                if (ctx->strictMode)
                    ctx->throwTypeError();
                return;
            }

            Property *p = o->arrayData + pidx;
            if (!o->arrayAttributes || o->arrayAttributes[pidx].isData()) {
                p->value = value;
                return;
            }

            if (o->arrayAttributes[pidx].isAccessor()) {
                FunctionObject *setter = p->setter();
                if (!setter) {
                    if (ctx->strictMode)
                        ctx->throwTypeError();
                    return;
                }

                ScopedCallData callData(ctx->engine, 1);
                callData->thisObject = Value::fromObject(o);
                callData->args[0] = value;
                setter->call(callData);
                return;
            }
        }
        o->putIndexed(idx, value);
        return;
    }

    String *name = index.toString(ctx);
    o->put(name, value);
}

void __qmljs_foreach_iterator_object(ExecutionContext *ctx, ValueRef result, const Value &in)
{
    Object *o = 0;
    if (!in.isNull() && !in.isUndefined())
        o = in.toObject(ctx);
    Object *it = ctx->engine->newForEachIteratorObject(ctx, o);
    *result = Value::fromObject(it);
}

void __qmljs_foreach_next_property_name(ValueRef result, const Value &foreach_iterator)
{
    assert(foreach_iterator.isObject());

    ForEachIteratorObject *it = static_cast<ForEachIteratorObject *>(foreach_iterator.objectValue());
    assert(it->as<ForEachIteratorObject>());

    *result = it->nextPropertyName();
}


void __qmljs_set_activation_property(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->setProperty(name, value);
}

void __qmljs_get_property(ExecutionContext *ctx, ValueRef result, const Value &object, String *name)
{
    Value res;
    Managed *m = object.asManaged();
    if (m) {
        res = m->get(name);
    } else {
        if (object.isNull() || object.isUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(name->toQString()).arg(object.toQString());
            ctx->throwTypeError(message);
        }

        m = __qmljs_convert_to_object(ctx, object);
        res = m->get(name);
    }
    if (result)
        *result = res;
}

void __qmljs_get_activation_property(ExecutionContext *ctx, ValueRef result, String *name)
{
    *result = ctx->getProperty(name);
}

uint __qmljs_equal_helper(const Value &x, const Value &y)
{
    Q_ASSERT(x.type() != y.type());

    if (x.isNumber() && y.isNumber())
        return x.asDouble() == y.asDouble();
    if (x.isNull() && y.isUndefined()) {
        return true;
    } else if (x.isUndefined() && y.isNull()) {
        return true;
    } else if (x.isNumber() && y.isString()) {
        double dy = __qmljs_to_number(y);
        return x.asDouble() == dy;
    } else if (x.isString() && y.isNumber()) {
        double dx = __qmljs_to_number(x);
        return dx == y.asDouble();
    } else if (x.isBoolean()) {
        Value nx = Value::fromDouble((double) x.booleanValue());
        return __qmljs_cmp_eq(nx, y);
    } else if (y.isBoolean()) {
        Value ny = Value::fromDouble((double) y.booleanValue());
        return __qmljs_cmp_eq(x, ny);
    } else if ((x.isNumber() || x.isString()) && y.isObject()) {
        Value py = __qmljs_to_primitive(y, PREFERREDTYPE_HINT);
        return __qmljs_cmp_eq(x, py);
    } else if (x.isObject() && (y.isNumber() || y.isString())) {
        Value px = __qmljs_to_primitive(x, PREFERREDTYPE_HINT);
        return __qmljs_cmp_eq(px, y);
    }

    return false;
}

Bool __qmljs_strict_equal(const Value &x, const Value &y)
{
    TRACE2(x, y);

    if (x.rawValue() == y.rawValue())
        // NaN != NaN
        return (x.tag & QV4::Value::NotDouble_Mask) != QV4::Value::NaN_Mask;

    if (x.isNumber())
        return y.isNumber() && x.asDouble() == y.asDouble();
    if (x.isString())
        return y.isString() && x.stringValue()->isEqualTo(y.stringValue());
    return false;
}


void __qmljs_call_global_lookup(ExecutionContext *context, ValueRef result, uint index, CallDataRef callData)
{
    Q_ASSERT(callData->thisObject.isUndefined());

    Lookup *l = context->lookups + index;
    Value v;
    l->globalGetter(l, context, &v);
    FunctionObject *o = v.asFunctionObject();
    if (!o)
        context->throwTypeError();

    if (o == context->engine->evalFunction && l->name->isEqualTo(context->engine->id_eval)) {
        Value res = static_cast<EvalFunction *>(o)->evalCall(callData->thisObject, callData->args, callData->argc, true);
        if (result)
            *result = res;
        return;
    }

    Value res = o->call(callData);
    if (result)
        *result = res;
}


void __qmljs_call_activation_property(ExecutionContext *context, ValueRef result, String *name, CallDataRef callData)
{
    Q_ASSERT(callData->thisObject.isUndefined());

    Object *base;
    Value func = context->getPropertyAndBase(name, &base);
    if (base)
        callData->thisObject = Value::fromObject(base);

    FunctionObject *o = func.asFunctionObject();
    if (!o) {
        QString objectAsString = QStringLiteral("[null]");
        if (base)
            objectAsString = Value::fromObject(base).toQString();
        QString msg = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString()).arg(objectAsString);
        context->throwTypeError(msg);
    }

    if (o == context->engine->evalFunction && name->isEqualTo(context->engine->id_eval)) {
        Value res = static_cast<EvalFunction *>(o)->evalCall(callData->thisObject, callData->args, callData->argc, true);
        if (result)
            *result = res;
        return;
    }

    Value res = o->call(callData);
    if (result)
        *result = res;
}

void __qmljs_call_property(ExecutionContext *context, ValueRef result, String *name, CallDataRef callData)
{
    Managed *baseObject = callData->thisObject.asManaged();
    if (!baseObject) {
        if (callData->thisObject.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot call method '%1' of %2").arg(name->toQString()).arg(callData->thisObject.toQString());
            context->throwTypeError(message);
        }

        baseObject = __qmljs_convert_to_object(context, callData->thisObject);
        callData->thisObject = Value::fromObject(static_cast<Object *>(baseObject));
    }

    FunctionObject *o = baseObject->get(name).asFunctionObject();
    if (!o) {
        QString error = QString("Property '%1' of object %2 is not a function").arg(name->toQString(), callData->thisObject.toQString());
        context->throwTypeError(error);
    }

    Value res = o->call(callData);
    if (result)
        *result = res;
}

void __qmljs_call_property_lookup(ExecutionContext *context, ValueRef result, uint index, CallDataRef callData)
{
    Value func;

    Lookup *l = context->lookups + index;
    l->getter(l, &func, callData->thisObject);

    Object *o = func.asObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(callData);
    if (result)
        *result = res;
}

void __qmljs_call_element(ExecutionContext *context, ValueRef result, const Value &index, CallDataRef callData)
{
    Object *baseObject = callData->thisObject.toObject(context);
    callData->thisObject = Value::fromObject(baseObject);

    Object *o = baseObject->get(index.toString(context)).asObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(callData);
    if (result)
        *result = res;
}

void __qmljs_call_value(ExecutionContext *context, ValueRef result, const Value &func, CallDataRef callData)
{
    Object *o = func.asObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(callData);
    if (result)
        *result = res;
}


void __qmljs_construct_global_lookup(ExecutionContext *context, ValueRef result, uint index, CallDataRef callData)
{
    Q_ASSERT(callData->thisObject.isUndefined());

    Value func;

    Lookup *l = context->lookups + index;
    l->globalGetter(l, context, &func);

    Object *f = func.asObject();
    if (!f)
        context->throwTypeError();

    Value res = f->construct(callData);
    if (result)
        *result = res;
}


void __qmljs_construct_activation_property(ExecutionContext *context, ValueRef result, String *name, CallDataRef callData)
{
    Value func = context->getProperty(name);
    Object *f = func.asObject();
    if (!f)
        context->throwTypeError();

    Value res = f->construct(callData);
    if (result)
        *result = res;
}

void __qmljs_construct_value(ExecutionContext *context, ValueRef result, const Value &func, CallDataRef callData)
{
    Object *f = func.asObject();
    if (!f)
        context->throwTypeError();

    Value res = f->construct(callData);
    if (result)
        *result = res;
}

void __qmljs_construct_property(ExecutionContext *context, ValueRef result, const Value &base, String *name, CallDataRef callData)
{
    Object *thisObject = base.toObject(context);

    Value func = thisObject->get(name);
    Object *f = func.asObject();
    if (!f)
        context->throwTypeError();

    Value res = f->construct(callData);
    if (result)
        *result = res;
}

void __qmljs_throw(ExecutionContext *context, const Value &value)
{
    Exception::throwException(context, value);
}

void __qmljs_builtin_typeof(ExecutionContext *ctx, ValueRef result, const Value &value)
{
    if (!result)
        return;
    String *res = 0;
    switch (value.type()) {
    case Value::Undefined_Type:
        res = ctx->engine->id_undefined;
        break;
    case Value::Null_Type:
        res = ctx->engine->id_object;
        break;
    case Value::Boolean_Type:
        res = ctx->engine->id_boolean;
        break;
    case Value::String_Type:
        res = ctx->engine->id_string;
        break;
    case Value::Object_Type:
        if (value.objectValue()->asFunctionObject())
            res = ctx->engine->id_function;
        else
            res = ctx->engine->id_object; // ### implementation-defined
        break;
    default:
        res = ctx->engine->id_number;
        break;
    }
    *result = Value::fromString(res);
}

void __qmljs_builtin_typeof_name(ExecutionContext *context, ValueRef result, String *name)
{
    Value res;
    __qmljs_builtin_typeof(context, &res, context->getPropertyNoThrow(name));
    if (result)
        *result = res;
}

void __qmljs_builtin_typeof_member(ExecutionContext *context, ValueRef result, const Value &base,
                                   String *name)
{
    Object *obj = base.toObject(context);
    Value res;
    __qmljs_builtin_typeof(context, &res, obj->get(name));
    if (result)
        *result = res;
}

void __qmljs_builtin_typeof_element(ExecutionContext *context, ValueRef result, const Value &base,
                                    const Value &index)
{
    String *name = index.toString(context);
    Object *obj = base.toObject(context);
    Value res;
    __qmljs_builtin_typeof(context, &res, obj->get(name));
    if (result)
        *result = res;
}

void __qmljs_builtin_post_increment(ValueRef result, ValueRef val)
{
    if (val->isInteger() && val->integerValue() < INT_MAX) {
        if (result)
            *result = *val;
        val->int_32 += 1;
        return;
    }

    double d = __qmljs_to_number(*val);
    *val = Value::fromDouble(d + 1);
    if (result)
        *result = Value::fromDouble(d);
}

void __qmljs_builtin_post_increment_name(ExecutionContext *context, ValueRef result, String *name)
{
    Value v = context->getProperty(name);

    if (v.isInteger() && v.integerValue() < INT_MAX) {
        if (result)
            *result = v;
        v.int_32 += 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d + 1);
    }

    context->setProperty(name, v);
}

void __qmljs_builtin_post_increment_member(ExecutionContext *context, ValueRef result, const Value &base, String *name)
{
    Object *o = base.toObject(context);

    Value v = o->get(name);

    if (v.isInteger() && v.integerValue() < INT_MAX) {
        if (result)
            *result = v;
        v.int_32 += 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d + 1);
    }

    o->put(name, v);
}

void __qmljs_builtin_post_increment_element(ExecutionContext *context, ValueRef result, const Value &base, const Value *index)
{
    Object *o = base.toObject(context);

    uint idx = index->asArrayIndex();

    if (idx == UINT_MAX) {
        String *s = index->toString(context);
        return __qmljs_builtin_post_increment_member(context, result, base, s);
    }

    Value v = o->getIndexed(idx);

    if (v.isInteger() && v.integerValue() < INT_MAX) {
        if (result)
            *result = v;
        v.int_32 += 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d + 1);
    }

    o->putIndexed(idx, v);
}

void __qmljs_builtin_post_decrement(ValueRef result, ValueRef val)
{
    if (val->isInteger() && val->integerValue() > INT_MIN) {
        if (result)
            *result = *val;
        val->int_32 -= 1;
        return;
    }

    double d = __qmljs_to_number(*val);
    *val = Value::fromDouble(d - 1);
    if (result)
        *result = Value::fromDouble(d);
}

void __qmljs_builtin_post_decrement_name(ExecutionContext *context, ValueRef result, String *name)
{
    Value v = context->getProperty(name);

    if (v.isInteger() && v.integerValue() > INT_MIN) {
        if (result)
            *result = v;
        v.int_32 -= 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d - 1);
    }

    context->setProperty(name, v);
}

void __qmljs_builtin_post_decrement_member(ExecutionContext *context, ValueRef result, const Value &base, String *name)
{
    Object *o = base.toObject(context);

    Value v = o->get(name);

    if (v.isInteger() && v.integerValue() > INT_MIN) {
        if (result)
            *result = v;
        v.int_32 -= 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d - 1);
    }

    o->put(name, v);
}

void __qmljs_builtin_post_decrement_element(ExecutionContext *context, ValueRef result, const Value &base, const Value &index)
{
    Object *o = base.toObject(context);

    uint idx = index.asArrayIndex();

    if (idx == UINT_MAX) {
        String *s = index.toString(context);
        return __qmljs_builtin_post_decrement_member(context, result, base, s);
    }

    Value v = o->getIndexed(idx);

    if (v.isInteger() && v.integerValue() > INT_MIN) {
        if (result)
            *result = v;
        v.int_32 -= 1;
    } else {
        double d = __qmljs_to_number(v);
        if (result)
            *result = Value::fromDouble(d);
        v = Value::fromDouble(d - 1);
    }

    o->putIndexed(idx, v);
}

ExecutionContext *__qmljs_builtin_push_with_scope(const Value &o, ExecutionContext *ctx)
{
    Object *obj = o.toObject(ctx);
    return ctx->newWithContext(obj);
}

ExecutionContext *__qmljs_builtin_push_catch_scope(String *exceptionVarName, const Value &exceptionValue, ExecutionContext *ctx)
{
    return ctx->newCatchContext(exceptionVarName, exceptionValue);
}

ExecutionContext *__qmljs_builtin_pop_scope(ExecutionContext *ctx)
{
    return ctx->engine->popContext();
}

void __qmljs_builtin_declare_var(ExecutionContext *ctx, bool deletable, String *name)
{
    ctx->createMutableBinding(name, deletable);
}

void __qmljs_builtin_define_property(ExecutionContext *ctx, const Value &object, String *name, Value *val)
{
    Object *o = object.asObject();
    assert(o);

    uint idx = name->asArrayIndex();
    Property *pd = (idx != UINT_MAX) ? o->arrayInsert(idx) : o->insertMember(name, Attr_Data);
    pd->value = val ? *val : Value::undefinedValue();
}

void __qmljs_builtin_define_array(ExecutionContext *ctx, ValueRef array, Value *values, uint length)
{
    ArrayObject *a = ctx->engine->newArrayObject();

    // ### FIXME: We need to allocate the array data to avoid crashes other places
    // This should rather be done when required
    a->arrayReserve(length);
    if (length) {
        a->arrayDataLen = length;
        Property *pd = a->arrayData;
        for (uint i = 0; i < length; ++i) {
            if (values[i].isEmpty()) {
                a->ensureArrayAttributes();
                pd->value = Value::undefinedValue();
                a->arrayAttributes[i].clear();
            } else {
                pd->value = values[i];
            }
            ++pd;
        }
        a->setArrayLengthUnchecked(length);
    }
    *array = Value::fromObject(a);
}

void __qmljs_builtin_define_getter_setter(ExecutionContext *ctx, const Value &object, String *name, const Value *getter, const Value *setter)
{
    Object *o = object.asObject();
    assert(o);

    uint idx = name->asArrayIndex();
    Property *pd = (idx != UINT_MAX) ? o->arrayInsert(idx, Attr_Accessor) : o->insertMember(name, Attr_Accessor);
    pd->setGetter(getter ? getter->asFunctionObject() : 0);
    pd->setSetter(setter ? setter->asFunctionObject() : 0);
}

void __qmljs_builtin_define_object_literal(QV4::ExecutionContext *ctx, ValueRef result, const QV4::Value *args, int classId)
{
    QV4::InternalClass *klass = ctx->compilationUnit->runtimeClasses[classId];
    Object *o = ctx->engine->newObject(klass);

    for (int i = 0; i < klass->size; ++i) {
        if (klass->propertyData[i].isData())
            o->memberData[i].value = *args++;
        else {
            o->memberData[i].setGetter(args->asFunctionObject());
            args++;
            o->memberData[i].setSetter(args->asFunctionObject());
            args++;
        }
    }

    *result = Value::fromObject(o);
}

void __qmljs_builtin_setup_arguments_object(ExecutionContext *ctx, ValueRef result)
{
    assert(ctx->type >= ExecutionContext::Type_CallContext);
    CallContext *c = static_cast<CallContext *>(ctx);
    ArgumentsObject *args = new (c->engine->memoryManager) ArgumentsObject(c);
    *result = Value::fromObject(args);
}

void __qmljs_increment(QV4::ValueRef result, const QV4::ValueRef value)
{
    TRACE1(value);

    if (value->isInteger())
        *result = Value::fromInt32(value->integerValue() + 1);
    else {
        double d = __qmljs_to_number(*value);
        *result = Value::fromDouble(d + 1);
    }
}

void __qmljs_decrement(QV4::ValueRef result, const QV4::ValueRef value)
{
    TRACE1(value);

    if (value->isInteger())
        *result = Value::fromInt32(value->integerValue() - 1);
    else {
        double d = __qmljs_to_number(*value);
        *result = Value::fromDouble(d - 1);
    }
}

void __qmljs_value_to_double(double *result, const Value &value)
{
    *result = __qmljs_to_number(value);
}

int __qmljs_value_to_int32(const Value &value)
{
    return value.toInt32();
}

int __qmljs_double_to_int32(const double &d)
{
    return Value::toInt32(d);
}

unsigned __qmljs_value_to_uint32(const Value &value)
{
    return value.toUInt32();
}

unsigned __qmljs_double_to_uint32(const double &d)
{
    return Value::toUInt32(d);
}

void __qmljs_value_from_string(ValueRef result, String *string)
{
    *result = Value::fromString(string);
}

void __qmljs_lookup_runtime_regexp(ExecutionContext *ctx, ValueRef result, int id)
{
    *result = ctx->compilationUnit->runtimeRegularExpressions[id];
}

} // namespace QV4

QT_END_NAMESPACE
