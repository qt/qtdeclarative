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

#include "qv4global_p.h"
#include "qv4debugging_p.h"
#include "qv4runtime_p.h"
#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4objectproto_p.h"
#include "qv4globalobject_p.h"
#include "qv4stringobject_p.h"
#include "qv4lookup_p.h"
#include "qv4function_p.h"
#include "qv4unwindhelper_p.h"
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>
#include <typeinfo>
#include <stdlib.h>

#include "../../../3rdparty/double-conversion/double-conversion.h"

#if USE(LIBUNWIND_DEBUG)
#include <libunwind.h>
#include <execinfo.h>
#endif

using namespace QV4;

extern "C" {

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

void __qmljs_init_closure(ExecutionContext *ctx, Value *result, Function *clos)
{
    assert(clos);
    *result = Value::fromObject(ctx->engine->newScriptFunction(ctx, clos));
}

Function *__qmljs_register_function(ExecutionContext *ctx, String *name,
                                    bool hasDirectEval,
                                    bool usesArgumentsObject, bool isStrict,
                                    bool hasNestedFunctions,
                                    String **formals, unsigned formalCount,
                                    String **locals, unsigned localCount)
{
    Function *f = ctx->engine->newFunction(name ? name->toQString() : QString());

    f->hasDirectEval = hasDirectEval;
    f->usesArgumentsObject = usesArgumentsObject;
    f->isStrict = isStrict;
    f->hasNestedFunctions = hasNestedFunctions;

    for (unsigned i = 0; i < formalCount; ++i)
        if (formals[i])
            f->formals.append(formals[i]);
    for (unsigned i = 0; i < localCount; ++i)
        if (locals[i])
            f->locals.append(locals[i]);

    return f;
}

void __qmljs_delete_subscript(ExecutionContext *ctx, Value *result, const Value &base, const Value &index)
{
    if (Object *o = base.asObject()) {
        uint n = index.asArrayIndex();
        if (n < UINT_MAX) {
            Value res = Value::fromBoolean(o->deleteIndexedProperty(ctx, n));
            if (result)
                *result = res;
            return;
        }
    }

    String *name = index.toString(ctx);
    __qmljs_delete_member(ctx, result, base, name);
}

void __qmljs_delete_member(ExecutionContext *ctx, Value *result, const Value &base, String *name)
{
    Object *obj = base.toObject(ctx);
    Value res = Value::fromBoolean(obj->deleteProperty(ctx, name));
    if (result)
        *result = res;
}

void __qmljs_delete_name(ExecutionContext *ctx, Value *result, String *name)
{
    Value res = Value::fromBoolean(ctx->deleteProperty(name));
    if (result)
        *result = res;
}

void __qmljs_add_helper(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    Value pleft = __qmljs_to_primitive(left, PREFERREDTYPE_HINT);
    Value pright = __qmljs_to_primitive(right, PREFERREDTYPE_HINT);
    if (pleft.isString() || pright.isString()) {
        if (!pleft.isString())
            pleft = __qmljs_to_string(pleft, ctx);
        if (!pright.isString())
            pright = __qmljs_to_string(pright, ctx);
        String *string = __qmljs_string_concat(ctx, pleft.stringValue(), pright.stringValue());
        *result = Value::fromString(string);
        return;
    }
    double x = __qmljs_to_number(pleft);
    double y = __qmljs_to_number(pright);
    *result = Value::fromDouble(x + y);
}

void __qmljs_instanceof(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    Object *o = right.asObject();
    if (!o)
        ctx->throwTypeError();

    bool r = o->hasInstance(ctx, left);
    *result = Value::fromBoolean(r);
}

void __qmljs_in(ExecutionContext *ctx, Value *result, const Value &left, const Value &right)
{
    if (!right.isObject())
        ctx->throwTypeError();
    String *s = left.toString(ctx);
    bool r = right.objectValue()->__hasProperty__(s);
    *result = Value::fromBoolean(r);
}

void __qmljs_inplace_bit_and_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_bit_and);
}

void __qmljs_inplace_bit_or_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_bit_or);
}

void __qmljs_inplace_bit_xor_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_bit_xor);
}

void __qmljs_inplace_add_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_add);
}

void __qmljs_inplace_sub_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_sub);
}

void __qmljs_inplace_mul_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_mul);
}

void __qmljs_inplace_div_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_div);
}

void __qmljs_inplace_mod_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_mod);
}

void __qmljs_inplace_shl_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_shl);
}

void __qmljs_inplace_shr_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_shr);
}

void __qmljs_inplace_ushr_name(ExecutionContext *ctx, String *name, const Value &value)
{
    ctx->inplaceBitOp(name, value, __qmljs_ushr);
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

    Value conv = object->get(ctx, meth1);
    if (FunctionObject *o = conv.asFunctionObject()) {
        Value r = o->call(ctx, Value::fromObject(object), 0, 0);
        if (r.isPrimitive())
            return r;
    }

    conv = object->get(ctx, meth2);
    if (FunctionObject *o = conv.asFunctionObject()) {
        Value r = o->call(ctx, Value::fromObject(object), 0, 0);
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
    o->put(ctx, name, value);
}

void __qmljs_get_element(ExecutionContext *ctx, Value *result, const Value &object, const Value &index)
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

        Value res = o->getIndexed(ctx, idx);
        if (result)
            *result = res;
        return;
    }

    String *name = index.toString(ctx);
    Value res = o->get(ctx, name);
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

                Value args[1];
                args[0] = value;
                setter->call(ctx, Value::fromObject(o), args, 1);
                return;
            }
        }
        o->putIndexed(ctx, idx, value);
        return;
    }

    String *name = index.toString(ctx);
    o->put(ctx, name, value);
}

void __qmljs_foreach_iterator_object(ExecutionContext *ctx, Value *result, const Value &in)
{
    Object *o = 0;
    if (!in.isNull() && !in.isUndefined())
        o = in.toObject(ctx);
    Object *it = ctx->engine->newForEachIteratorObject(ctx, o);
    *result = Value::fromObject(it);
}

void __qmljs_foreach_next_property_name(Value *result, const Value &foreach_iterator)
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

void __qmljs_get_property(ExecutionContext *ctx, Value *result, const Value &object, String *name)
{
    Value res;
    Managed *m = object.asManaged();
    if (m) {
        res = m->get(ctx, name);
    } else {
        m = __qmljs_convert_to_object(ctx, object);
        res = m->get(ctx, name);
    }
    if (result)
        *result = res;
}

void __qmljs_get_activation_property(ExecutionContext *ctx, Value *result, String *name)
{
    *result = ctx->getProperty(name);
}

uint __qmljs_equal(const Value &x, const Value &y)
{
    if (x.type() == y.type()) {
        switch (x.type()) {
        case Value::Undefined_Type:
            return true;
        case Value::Null_Type:
            return true;
        case Value::Boolean_Type:
            return x.booleanValue() == y.booleanValue();
            break;
        case Value::Integer_Type:
            return x.integerValue() == y.integerValue();
        case Value::String_Type:
            return x.stringValue()->isEqualTo(y.stringValue());
        case Value::Object_Type:
            if (x.objectValue() == y.objectValue())
                return true;
            return x.objectValue()->isEqualTo(y.objectValue());
        default: // double
            return x.doubleValue() == y.doubleValue();
        }
        // unreachable
    } else {
        if (x.isNumber() && y.isNumber())
            return x.asDouble() == y.asDouble();
        if (x.isNull() && y.isUndefined()) {
            return true;
        } else if (x.isUndefined() && y.isNull()) {
            return true;
        } else if (x.isNumber() && y.isString()) {
            Value ny = Value::fromDouble(__qmljs_to_number(y));
            return __qmljs_equal(x, ny);
        } else if (x.isString() && y.isNumber()) {
            Value nx = Value::fromDouble(__qmljs_to_number(x));
            return __qmljs_equal(nx, y);
        } else if (x.isBoolean()) {
            Value nx = Value::fromDouble((double) x.booleanValue());
            return __qmljs_equal(nx, y);
        } else if (y.isBoolean()) {
            Value ny = Value::fromDouble((double) y.booleanValue());
            return __qmljs_equal(x, ny);
        } else if ((x.isNumber() || x.isString()) && y.isObject()) {
            Value py = __qmljs_to_primitive(y, PREFERREDTYPE_HINT);
            return __qmljs_equal(x, py);
        } else if (x.isObject() && (y.isNumber() || y.isString())) {
            Value px = __qmljs_to_primitive(x, PREFERREDTYPE_HINT);
            return __qmljs_equal(px, y);
        }
    }

    return false;
}

Bool __qmljs_strict_equal(const Value &x, const Value &y)
{
    TRACE2(x, y);

    if (x.isDouble() || y.isDouble())
        return x.asDouble() == y.asDouble();
    if (x.rawValue() == y.rawValue())
        return true;
    if (x.type() != y.type())
        return false;
    if (x.isString())
        return x.stringValue()->isEqualTo(y.stringValue());
    if (x.isObject())
        return x.objectValue()->isEqualTo(y.objectValue());
    return false;
}


void __qmljs_call_global_lookup(ExecutionContext *context, Value *result, uint index, Value *args, int argc)
{
    Lookup *l = context->lookups + index;
    Value v;
    l->globalGetter(l, context, &v);
    FunctionObject *o = v.asFunctionObject();
    if (!o)
        context->throwTypeError();

    Value thisObject = Value::undefinedValue();

    if (o == context->engine->evalFunction && l->name->isEqualTo(context->engine->id_eval)) {
        Value res = static_cast<EvalFunction *>(o)->evalCall(context, thisObject, args, argc, true);
        if (result)
            *result = res;
        return;
    }

    Value res = o->call(context, thisObject, args, argc);
    if (result)
        *result = res;
}


void __qmljs_call_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    Object *base;
    Value func = context->getPropertyAndBase(name, &base);
    FunctionObject *o = func.asFunctionObject();
    if (!o) {
        QString objectAsString = QStringLiteral("[null]");
        if (base)
            objectAsString = Value::fromObject(base).toQString();
        QString msg = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString()).arg(objectAsString);
        context->throwTypeError(msg);
    }

    Value thisObject = base ? Value::fromObject(base) : Value::undefinedValue();

    if (o == context->engine->evalFunction && name->isEqualTo(context->engine->id_eval)) {
        Value res = static_cast<EvalFunction *>(o)->evalCall(context, thisObject, args, argc, true);
        if (result)
            *result = res;
        return;
    }

    Value res = o->call(context, thisObject, args, argc);
    if (result)
        *result = res;
}

void __qmljs_call_property(ExecutionContext *context, Value *result, const Value &thatObject, String *name, Value *args, int argc)
{
    Value thisObject = thatObject;
    Managed *baseObject = thisObject.asManaged();
    if (!baseObject) {
        baseObject = __qmljs_convert_to_object(context, thisObject);
        thisObject = Value::fromObject(static_cast<Object *>(baseObject));
    }

    Value func = baseObject->get(context, name);
    FunctionObject *o = func.asFunctionObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(context, thisObject, args, argc);
    if (result)
        *result = res;
}

void __qmljs_call_property_lookup(ExecutionContext *context, Value *result, const Value &thisObject, uint index, Value *args, int argc)
{
    Lookup *l = context->lookups + index;

    Object *baseObject;
    if (thisObject.isObject())
        baseObject = thisObject.objectValue();
    else if (thisObject.isString())
        baseObject = context->engine->stringPrototype;
    else
        baseObject = __qmljs_convert_to_object(context, thisObject);

    PropertyAttributes attrs;
    Property *p = l->lookup(baseObject, &attrs);
    if (!p)
        context->throwTypeError();
    Value func = attrs.isData() ? p->value : baseObject->getValue(context, p, attrs);
    FunctionObject *o = func.asFunctionObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(context, thisObject, args, argc);
    if (result)
        *result = res;
}

void __qmljs_call_element(ExecutionContext *context, Value *result, const Value &that, const Value &index, Value *args, int argc)
{
    Object *baseObject = that.toObject(context);
    Value thisObject = Value::fromObject(baseObject);

    Value func = baseObject->get(context, index.toString(context));
    Object *o = func.asObject();
    if (!o)
        context->throwTypeError();

    Value res = o->call(context, thisObject, args, argc);
    if (result)
        *result = res;
}

void __qmljs_call_value(ExecutionContext *context, Value *result, const Value *thisObject, const Value &func, Value *args, int argc)
{
    Object *o = func.asObject();
    if (!o)
        context->throwTypeError();
    Value res = o->call(context, thisObject ? *thisObject : Value::undefinedValue(), args, argc);
    if (result)
        *result = res;
}


void __qmljs_construct_global_lookup(ExecutionContext *context, Value *result, uint index, Value *args, int argc)
{
    Lookup *l = context->lookups + index;
    Value func;
    l->globalGetter(l, context, &func);

    if (Object *f = func.asObject()) {
        Value res = f->construct(context, args, argc);
        if (result)
            *result = res;
        return;
    }

    context->throwTypeError();
}


void __qmljs_construct_activation_property(ExecutionContext *context, Value *result, String *name, Value *args, int argc)
{
    Value func = context->getProperty(name);
    __qmljs_construct_value(context, result, func, args, argc);
}

void __qmljs_construct_value(ExecutionContext *context, Value *result, const Value &func, Value *args, int argc)
{
    if (Object *f = func.asObject()) {
        Value res = f->construct(context, args, argc);
        if (result)
            *result = res;
        return;
    }

    context->throwTypeError();
}

void __qmljs_construct_property(ExecutionContext *context, Value *result, const Value &base, String *name, Value *args, int argc)
{
    Object *thisObject = base.toObject(context);

    Value func = thisObject->get(context, name);
    if (Object *f = func.asObject()) {
        Value res = f->construct(context, args, argc);
        if (result)
            *result = res;
        return;
    }

    context->throwTypeError();
}

void __qmljs_throw(ExecutionContext *context, const Value &value)
{
    if (context->engine->debugger)
        context->engine->debugger->aboutToThrow(value);

    UnwindHelper::prepareForUnwind(context);

#if USE(LIBUNWIND_DEBUG)
    printf("about to throw exception. walking stack first with libunwind:\n");
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        printf("ip = %lx, sp = %lx ", (long) ip, (long) sp);
        void * const addr = (void*)ip;
        char **symbol = backtrace_symbols(&addr, 1);
        printf("%s", symbol[0]);
        free(symbol);
        printf("\n");
    }
    printf("stack walked. throwing exception now...\n");
#endif

    throw Exception(context, value);
}

void __qmljs_builtin_typeof(ExecutionContext *ctx, Value *result, const Value &value)
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

void __qmljs_builtin_typeof_name(ExecutionContext *context, Value *result, String *name)
{
    Value res;
    __qmljs_builtin_typeof(context, &res, context->getPropertyNoThrow(name));
    if (result)
        *result = res;
}

void __qmljs_builtin_typeof_member(ExecutionContext *context, Value *result, const Value &base, String *name)
{
    Object *obj = base.toObject(context);
    Value res;
    __qmljs_builtin_typeof(context, &res, obj->get(context, name));
    if (result)
        *result = res;
}

void __qmljs_builtin_typeof_element(ExecutionContext *context, Value *result, const Value &base, const Value &index)
{
    String *name = index.toString(context);
    Object *obj = base.toObject(context);
    Value res;
    __qmljs_builtin_typeof(context, &res, obj->get(context, name));
    if (result)
        *result = res;
}

void __qmljs_builtin_post_increment(Value *result, Value *val)
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

void __qmljs_builtin_post_increment_name(ExecutionContext *context, Value *result, String *name)
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

void __qmljs_builtin_post_increment_member(ExecutionContext *context, Value *result, const Value &base, String *name)
{
    Object *o = base.toObject(context);

    Value v = o->get(context, name);

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

    o->put(context, name, v);
}

void __qmljs_builtin_post_increment_element(ExecutionContext *context, Value *result, const Value &base, const Value *index)
{
    Object *o = base.toObject(context);

    uint idx = index->asArrayIndex();

    if (idx == UINT_MAX) {
        String *s = index->toString(context);
        return __qmljs_builtin_post_increment_member(context, result, base, s);
    }

    Value v = o->getIndexed(context, idx);

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

    o->putIndexed(context, idx, v);
}

void __qmljs_builtin_post_decrement(Value *result, Value *val)
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

void __qmljs_builtin_post_decrement_name(ExecutionContext *context, Value *result, String *name)
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

void __qmljs_builtin_post_decrement_member(ExecutionContext *context, Value *result, const Value &base, String *name)
{
    Object *o = base.toObject(context);

    Value v = o->get(context, name);

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

    o->put(context, name, v);
}

void __qmljs_builtin_post_decrement_element(ExecutionContext *context, Value *result, const Value &base, const Value &index)
{
    Object *o = base.toObject(context);

    uint idx = index.asArrayIndex();

    if (idx == UINT_MAX) {
        String *s = index.toString(context);
        return __qmljs_builtin_post_decrement_member(context, result, base, s);
    }

    Value v = o->getIndexed(context, idx);

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

    o->putIndexed(context, idx, v);
}

ExecutionContext *__qmljs_builtin_push_with_scope(const Value &o, ExecutionContext *ctx)
{
    Object *obj = o.toObject(ctx);
    return ctx->engine->newWithContext(obj);
}

ExecutionContext *__qmljs_builtin_push_catch_scope(String *exceptionVarName, const Value &exceptionValue, ExecutionContext *ctx)
{
    return ctx->engine->newCatchContext(exceptionVarName, exceptionValue);
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

void __qmljs_builtin_define_array(ExecutionContext *ctx, Value *array, Value *values, uint length)
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

void __qmljs_builtin_define_object_literal(QV4::ExecutionContext *ctx, QV4::Value *result, const QV4::Value *args, QV4::InternalClass *klass)
{
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

void __qmljs_increment(Value *result, const Value &value)
{
    TRACE1(value);

    if (value.isInteger())
        *result = Value::fromInt32(value.integerValue() + 1);
    else {
        double d = __qmljs_to_number(value);
        *result = Value::fromDouble(d + 1);
    }
}

void __qmljs_decrement(Value *result, const Value &value)
{
    TRACE1(value);

    if (value.isInteger())
        *result = Value::fromInt32(value.integerValue() - 1);
    else {
        double d = __qmljs_to_number(value);
        *result = Value::fromDouble(d - 1);
    }
}

} // extern "C"
