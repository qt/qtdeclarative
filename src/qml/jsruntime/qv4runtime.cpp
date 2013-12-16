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
#include "private/qlocale_tools_p.h"
#include "qv4scopedvalue_p.h"
#include <private/qqmlcontextwrapper_p.h>
#include "qv4qobjectwrapper_p.h"
#include <private/qv8engine_p.h>

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

#ifdef QV4_COUNT_RUNTIME_FUNCTIONS
struct RuntimeCounters::Data {
    enum Type {
        None = 0,
        Undefined = 1,
        Null = 2,
        Boolean = 3,
        Integer = 4,
        Managed = 5,
        Double = 7
    };

    static const char *pretty(Type t) {
        switch (t) {
        case None: return "";
        case Undefined: return "Undefined";
        case Null: return "Null";
        case Boolean: return "Boolean";
        case Integer: return "Integer";
        case Managed: return "Managed";
        case Double: return "Double";
        default: return "Unknown";
        }
    }

    static unsigned mangle(unsigned tag) {
        switch (tag) {
        case Value::Undefined_Type: return Undefined;
        case Value::Null_Type: return Null;
        case Value::Boolean_Type: return Boolean;
        case Value::Integer_Type: return Integer;
        case Value::Managed_Type: return Managed;
        default: return Double;
        }
    }

    static unsigned mangle(unsigned tag1, unsigned tag2) {
        return (mangle(tag1) << 3) | mangle(tag2);
    }

    static void unmangle(unsigned signature, Type &tag1, Type &tag2) {
        tag1 = Type((signature >> 3) & 7);
        tag2 = Type(signature & 7);
    }

    typedef QVector<quint64> Counters;
    QHash<const char *, Counters> counters;

    inline void count(const char *func) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[0] += 1;
    }

    inline void count(const char *func, unsigned tag) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[mangle(tag)] += 1;
    }

    inline void count(const char *func, unsigned tag1, unsigned tag2) {
        QVector<quint64> &cnt = counters[func];
        if (cnt.isEmpty())
            cnt.resize(64);
        cnt[mangle(tag1, tag2)] += 1;
    }

    struct Line {
        const char *func;
        Type tag1, tag2;
        quint64 count;

        static bool less(const Line &line1, const Line &line2) {
            return line1.count > line2.count;
        }
    };

    void dump() const {
        QTextStream outs(stderr, QIODevice::WriteOnly);
        QList<Line> lines;
        foreach (const char *func, counters.keys()) {
            const Counters &fCount = counters[func];
            for (int i = 0, ei = fCount.size(); i != ei; ++i) {
                quint64 count = fCount[i];
                if (!count)
                    continue;
                Line line;
                line.func = func;
                unmangle(i, line.tag1, line.tag2);
                line.count = count;
                lines.append(line);
            }
        }
        qSort(lines.begin(), lines.end(), Line::less);
        outs << lines.size() << " counters:" << endl;
        foreach (const Line &line, lines)
            outs << qSetFieldWidth(10) << line.count << qSetFieldWidth(0)
                 << " | " << line.func
                 << " | " << pretty(line.tag1)
                 << " | " << pretty(line.tag2)
                 << endl;
    }
};

RuntimeCounters *RuntimeCounters::instance = 0;
static RuntimeCounters runtimeCountersInstance;
RuntimeCounters::RuntimeCounters()
    : d(new Data)
{
    if (!instance)
        instance = this;
}

RuntimeCounters::~RuntimeCounters()
{
    d->dump();
    delete d;
}

void RuntimeCounters::count(const char *func)
{
    d->count(func);
}

void RuntimeCounters::count(const char *func, uint tag)
{
    d->count(func, tag);
}

void RuntimeCounters::count(const char *func, uint tag1, uint tag2)
{
    d->count(func, tag1, tag2);
}

#endif // QV4_COUNT_RUNTIME_FUNCTIONS

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
    num = Primitive::toInteger(num);

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

ReturnedValue __qmljs_init_closure(ExecutionContext *ctx, int functionId)
{
    QV4::Function *clos = ctx->compilationUnit->runtimeFunctions[functionId];
    Q_ASSERT(clos);
    FunctionObject *f = FunctionObject::creatScriptFunction(ctx, clos);
    return f->asReturnedValue();
}

ReturnedValue __qmljs_delete_subscript(ExecutionContext *ctx, const ValueRef base, const ValueRef index)
{
    Scope scope(ctx);
    ScopedObject o(scope, base);
    if (o) {
        uint n = index->asArrayIndex();
        if (n < UINT_MAX) {
            return Encode((bool)o->deleteIndexedProperty(n));
        }
    }

    ScopedString name(scope, index->toString(ctx));
    return __qmljs_delete_member(ctx, base, name);
}

ReturnedValue __qmljs_delete_member(ExecutionContext *ctx, const ValueRef base, const StringRef name)
{
    Scope scope(ctx);
    ScopedObject obj(scope, base->toObject(ctx));
    if (scope.engine->hasException)
        return Encode::undefined();
    return Encode(obj->deleteProperty(name));
}

ReturnedValue __qmljs_delete_name(ExecutionContext *ctx, const StringRef name)
{
    Scope scope(ctx);
    return Encode(ctx->deleteProperty(name));
}

QV4::ReturnedValue __qmljs_instanceof(ExecutionContext *ctx, const ValueRef left, const ValueRef right)
{
    FunctionObject *f = right->asFunctionObject();
    if (!f)
        return ctx->throwTypeError();

    if (f->subtype == FunctionObject::BoundFunction)
        f = static_cast<BoundFunction *>(f)->target;

    Scope scope(ctx->engine);
    ScopedObject v(scope, left);
    if (!v)
        return Encode(false);

    Scoped<Object> o(scope, f->protoProperty());
    if (!o) {
        scope.engine->currentContext()->throwTypeError();
        return Encode(false);
    }

    while (v) {
        v = v->prototype();

        if (! v)
            break;
        else if (o.getPointer() == v)
            return Encode(true);
    }

    return Encode(false);
}

QV4::ReturnedValue __qmljs_in(ExecutionContext *ctx, const ValueRef left, const ValueRef right)
{
    if (!right->isObject())
        return ctx->throwTypeError();
    Scope scope(ctx);
    ScopedString s(scope, left->toString(ctx));
    if (scope.hasException())
        return Encode::undefined();
    bool r = right->objectValue()->__hasProperty__(s);
    return Encode(r);
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

Returned<String> *__qmljs_string_from_number(ExecutionContext *ctx, double number)
{
    QString qstr;
    __qmljs_numberToString(&qstr, number, 10);
    return ctx->engine->newString(qstr);
}

ReturnedValue __qmljs_object_default_value(Object *object, int typeHint)
{
    if (typeHint == PREFERREDTYPE_HINT) {
        if (object->asDateObject())
            typeHint = STRING_HINT;
        else
            typeHint = NUMBER_HINT;
    }

    ExecutionEngine *engine = object->internalClass->engine;
    if (engine->hasException)
        return Encode::undefined();

    SafeString *meth1 = &engine->id_toString;
    SafeString *meth2 = &engine->id_valueOf;

    if (typeHint == NUMBER_HINT)
        qSwap(meth1, meth2);

    ExecutionContext *ctx = engine->currentContext();
    Scope scope(ctx);
    ScopedCallData callData(scope, 0);
    callData->thisObject = object;

    ScopedValue conv(scope, object->get(*meth1));
    if (FunctionObject *o = conv->asFunctionObject()) {
        ScopedValue r(scope, o->call(callData));
        if (r->isPrimitive())
            return r->asReturnedValue();
    }

    if (engine->hasException)
        return Encode::undefined();

    conv = object->get(*meth2);
    if (FunctionObject *o = conv->asFunctionObject()) {
        ScopedValue r(scope, o->call(callData));
        if (r->isPrimitive())
            return r->asReturnedValue();
    }

    return ctx->throwTypeError();
}

Bool __qmljs_to_boolean(const ValueRef value)
{
    return value->toBoolean();
}


Returned<Object> *__qmljs_convert_to_object(ExecutionContext *ctx, const ValueRef value)
{
    assert(!value->isObject());
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        ctx->throwTypeError();
        return 0;
    case Value::Boolean_Type:
        return ctx->engine->newBooleanObject(value);
    case Value::Managed_Type:
        Q_ASSERT(value->isString());
        return ctx->engine->newStringObject(value);
    case Value::Integer_Type:
    default: // double
        return ctx->engine->newNumberObject(value);
    }
}

Returned<String> *__qmljs_convert_to_string(ExecutionContext *ctx, const ValueRef value)
{
    switch (value->type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return ctx->engine->id_undefined.ret();
    case Value::Null_Type:
        return ctx->engine->id_null.ret();
    case Value::Boolean_Type:
        if (value->booleanValue())
            return ctx->engine->id_true.ret();
        else
            return ctx->engine->id_false.ret();
    case Value::Managed_Type:
        if (value->isString())
            return value->stringValue()->asReturned<String>();
        {
            Scope scope(ctx);
            ScopedValue prim(scope, __qmljs_to_primitive(value, STRING_HINT));
            return __qmljs_convert_to_string(ctx, prim);
        }
    case Value::Integer_Type:
        return __qmljs_string_from_number(ctx, value->int_32);
    default: // double
        return __qmljs_string_from_number(ctx, value->doubleValue());
    } // switch
}

// This is slightly different from the method above, as
// the + operator requires a slightly different conversion
static Returned<String> *convert_to_string_add(ExecutionContext *ctx, const ValueRef value)
{
    switch (value->type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return ctx->engine->id_undefined.ret();
    case Value::Null_Type:
        return ctx->engine->id_null.ret();
    case Value::Boolean_Type:
        if (value->booleanValue())
            return ctx->engine->id_true.ret();
        else
            return ctx->engine->id_false.ret();
    case Value::Managed_Type:
        if (value->isString())
            return value->stringValue()->asReturned<String>();
        {
            Scope scope(ctx);
            ScopedValue prim(scope, __qmljs_to_primitive(value, PREFERREDTYPE_HINT));
            return __qmljs_convert_to_string(ctx, prim);
        }
    case Value::Integer_Type:
        return __qmljs_string_from_number(ctx, value->int_32);
    default: // double
        return __qmljs_string_from_number(ctx, value->doubleValue());
    } // switch
}

QV4::ReturnedValue __qmljs_add_helper(ExecutionContext *ctx, const ValueRef left, const ValueRef right)
{
    Scope scope(ctx);

    ScopedValue pleft(scope, __qmljs_to_primitive(left, PREFERREDTYPE_HINT));
    ScopedValue pright(scope, __qmljs_to_primitive(right, PREFERREDTYPE_HINT));
    if (pleft->isString() || pright->isString()) {
        if (!pleft->isString())
            pleft = convert_to_string_add(ctx, pleft);
        if (!pright->isString())
            pright = convert_to_string_add(ctx, pright);
        if (scope.engine->hasException)
            return Encode::undefined();
        if (!pleft->stringValue()->length())
            return pright->asReturnedValue();
        if (!pright->stringValue()->length())
            return pleft->asReturnedValue();
        return (new (ctx->engine->memoryManager) String(ctx->engine, pleft->stringValue(), pright->stringValue()))->asReturnedValue();
    }
    double x = __qmljs_to_number(pleft);
    double y = __qmljs_to_number(pright);
    return Encode(x + y);
}

QV4::ReturnedValue __qmljs_add_string(QV4::ExecutionContext *ctx, const QV4::ValueRef left, const QV4::ValueRef right)
{
    Q_ASSERT(left->isString() || right->isString());

    if (left->isString() && right->isString()) {
        if (!left->stringValue()->length())
            return right->asReturnedValue();
        if (!right->stringValue()->length())
            return left->asReturnedValue();
        return (new (ctx->engine->memoryManager) String(ctx->engine, left->stringValue(), right->stringValue()))->asReturnedValue();
    }

    Scope scope(ctx);
    ScopedValue pleft(scope, *left);
    ScopedValue pright(scope, *right);

    if (!pleft->isString())
        pleft = convert_to_string_add(ctx, left);
    if (!pright->isString())
        pright = convert_to_string_add(ctx, right);
    if (scope.engine->hasException)
        return Encode::undefined();
    if (!pleft->stringValue()->length())
        return pright->asReturnedValue();
    if (!pright->stringValue()->length())
        return pleft->asReturnedValue();
    return (new (ctx->engine->memoryManager) String(ctx->engine, pleft->stringValue(), pright->stringValue()))->asReturnedValue();
}

void __qmljs_set_property(ExecutionContext *ctx, const ValueRef object, const StringRef name, const ValueRef value)
{
    Scope scope(ctx);
    ScopedObject o(scope, object->toObject(ctx));
    if (!o)
        return;
    o->put(name, value);
}

ReturnedValue __qmljs_get_element(ExecutionContext *ctx, const ValueRef object, const ValueRef index)
{
    Scope scope(ctx);
    uint idx = index->asArrayIndex();

    Scoped<Object> o(scope, object);
    if (!o) {
        if (idx < UINT_MAX) {
            if (String *str = object->asString()) {
                if (idx >= (uint)str->toQString().length()) {
                    return Encode::undefined();
                }
                const QString s = str->toQString().mid(idx, 1);
                return scope.engine->newString(s)->asReturnedValue();
            }
        }

        if (object->isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot read property '%1' of %2").arg(index->toQStringNoThrow()).arg(object->toQStringNoThrow());
            return ctx->throwTypeError(message);
        }

        o = __qmljs_convert_to_object(ctx, object);
        if (!o) // type error
            return Encode::undefined();
    }

    if (idx < UINT_MAX) {
        uint pidx = o->propertyIndexFromArrayIndex(idx);
        if (pidx < UINT_MAX) {
            if (!o->arrayAttributes || o->arrayAttributes[pidx].isData()) {
                if (!o->arrayData[pidx].value.isEmpty())
                    return o->arrayData[pidx].value.asReturnedValue();
            }
        }

        return o->getIndexed(idx);
    }

    ScopedString name(scope, index->toString(ctx));
    if (scope.hasException())
        return Encode::undefined();
    return o->get(name);
}

void __qmljs_set_element(ExecutionContext *ctx, const ValueRef object, const ValueRef index, const ValueRef value)
{
    Scope scope(ctx);
    ScopedObject o(scope, object->toObject(ctx));
    if (scope.engine->hasException)
        return;

    uint idx = index->asArrayIndex();
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
                p->value = *value;
                return;
            }

            if (o->arrayAttributes[pidx].isAccessor()) {
                FunctionObject *setter = p->setter();
                if (!setter) {
                    if (ctx->strictMode)
                        ctx->throwTypeError();
                    return;
                }

                ScopedCallData callData(scope, 1);
                callData->thisObject = o;
                callData->args[0] = *value;
                setter->call(callData);
                return;
            }
        }
        o->putIndexed(idx, value);
        return;
    }

    ScopedString name(scope, index->toString(ctx));
    o->put(name, value);
}

ReturnedValue __qmljs_foreach_iterator_object(ExecutionContext *ctx, const ValueRef in)
{
    Scope scope(ctx);
    Scoped<Object> o(scope, (Object *)0);
    if (!in->isNullOrUndefined())
        o = in;
    Scoped<Object> it(scope, ctx->engine->newForEachIteratorObject(ctx, o));
    return it.asReturnedValue();
}

ReturnedValue __qmljs_foreach_next_property_name(const ValueRef foreach_iterator)
{
    Q_ASSERT(foreach_iterator->isObject());

    ForEachIteratorObject *it = static_cast<ForEachIteratorObject *>(foreach_iterator->objectValue());
    Q_ASSERT(it->as<ForEachIteratorObject>());

    return it->nextPropertyName();
}


void __qmljs_set_activation_property(ExecutionContext *ctx, const StringRef name, const ValueRef value)
{
    ctx->setProperty(name, value);
}

ReturnedValue __qmljs_get_property(ExecutionContext *ctx, const ValueRef object, const StringRef name)
{
    Scope scope(ctx);

    Scoped<Object> o(scope, object);
    if (o)
        return o->get(name);

    if (object->isNullOrUndefined()) {
        QString message = QStringLiteral("Cannot read property '%1' of %2").arg(name->toQString()).arg(object->toQStringNoThrow());
        return ctx->throwTypeError(message);
    }

    o = __qmljs_convert_to_object(ctx, object);
    if (!o) // type error
        return Encode::undefined();
    return o->get(name);
}

ReturnedValue __qmljs_get_activation_property(ExecutionContext *ctx, const StringRef name)
{
    return ctx->getProperty(name);
}

uint __qmljs_equal_helper(const ValueRef x, const ValueRef y)
{
    Q_ASSERT(x->type() != y->type() || (x->isManaged() && (x->isString() != y->isString())));

    if (x->isNumber() && y->isNumber())
        return x->asDouble() == y->asDouble();
    if (x->isNull() && y->isUndefined()) {
        return true;
    } else if (x->isUndefined() && y->isNull()) {
        return true;
    } else if (x->isNumber() && y->isString()) {
        double dy = __qmljs_to_number(y);
        return x->asDouble() == dy;
    } else if (x->isString() && y->isNumber()) {
        double dx = __qmljs_to_number(x);
        return dx == y->asDouble();
    } else if (x->isBoolean()) {
        return __qmljs_cmp_eq(Primitive::fromDouble((double) x->booleanValue()), y);
    } else if (y->isBoolean()) {
        return __qmljs_cmp_eq(x, Primitive::fromDouble((double) y->booleanValue()));
    } else if ((x->isNumber() || x->isString()) && y->isObject()) {
        Scope scope(y->objectValue()->engine());
        ScopedValue py(scope, __qmljs_to_primitive(y, PREFERREDTYPE_HINT));
        return __qmljs_cmp_eq(x, py);
    } else if (x->isObject() && (y->isNumber() || y->isString())) {
        Scope scope(x->objectValue()->engine());
        ScopedValue px(scope, __qmljs_to_primitive(x, PREFERREDTYPE_HINT));
        return __qmljs_cmp_eq(px, y);
    }

    return false;
}

Bool __qmljs_strict_equal(const ValueRef x, const ValueRef y)
{
    TRACE2(x, y);

    if (x->rawValue() == y->rawValue())
        // NaN != NaN
        return !x->isNaN();

    if (x->isNumber())
        return y->isNumber() && x->asDouble() == y->asDouble();
    if (x->isManaged())
        return y->isManaged() && x->managed()->isEqualTo(y->managed());
    return false;
}

QV4::Bool __qmljs_cmp_gt(const QV4::ValueRef l, const QV4::ValueRef r)
{
    TRACE2(l, r);
    if (QV4::Value::integerCompatible(*l, *r))
        return l->integerValue() > r->integerValue();
    if (QV4::Value::bothDouble(*l, *r))
        return l->doubleValue() > r->doubleValue();
    if (l->isString() && r->isString())
        return r->stringValue()->compare(l->stringValue());

    if (l->isObject() || r->isObject()) {
        QV4::ExecutionEngine *e = (l->isObject() ? l->objectValue() : r->objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, __qmljs_to_primitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, __qmljs_to_primitive(r, QV4::NUMBER_HINT));
        return __qmljs_cmp_gt(pl, pr);
    }

    double dl = __qmljs_to_number(l);
    double dr = __qmljs_to_number(r);
    return dl > dr;
}

QV4::Bool __qmljs_cmp_lt(const QV4::ValueRef l, const QV4::ValueRef r)
{
    TRACE2(l, r);
    if (QV4::Value::integerCompatible(*l, *r))
        return l->integerValue() < r->integerValue();
    if (QV4::Value::bothDouble(*l, *r))
        return l->doubleValue() < r->doubleValue();
    if (l->isString() && r->isString())
        return l->stringValue()->compare(r->stringValue());

    if (l->isObject() || r->isObject()) {
        QV4::ExecutionEngine *e = (l->isObject() ? l->objectValue() : r->objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, __qmljs_to_primitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, __qmljs_to_primitive(r, QV4::NUMBER_HINT));
        return __qmljs_cmp_lt(pl, pr);
    }

    double dl = __qmljs_to_number(l);
    double dr = __qmljs_to_number(r);
    return dl < dr;
}

QV4::Bool __qmljs_cmp_ge(const QV4::ValueRef l, const QV4::ValueRef r)
{
    TRACE2(l, r);
    if (QV4::Value::integerCompatible(*l, *r))
        return l->integerValue() >= r->integerValue();
    if (QV4::Value::bothDouble(*l, *r))
        return l->doubleValue() >= r->doubleValue();
    if (l->isString() && r->isString())
        return !l->stringValue()->compare(r->stringValue());

    if (l->isObject() || r->isObject()) {
        QV4::ExecutionEngine *e = (l->isObject() ? l->objectValue() : r->objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, __qmljs_to_primitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, __qmljs_to_primitive(r, QV4::NUMBER_HINT));
        return __qmljs_cmp_ge(pl, pr);
    }

    double dl = __qmljs_to_number(l);
    double dr = __qmljs_to_number(r);
    return dl >= dr;
}

QV4::Bool __qmljs_cmp_le(const QV4::ValueRef l, const QV4::ValueRef r)
{
    TRACE2(l, r);
    if (QV4::Value::integerCompatible(*l, *r))
        return l->integerValue() <= r->integerValue();
    if (QV4::Value::bothDouble(*l, *r))
        return l->doubleValue() <= r->doubleValue();
    if (l->isString() && r->isString())
        return !r->stringValue()->compare(l->stringValue());

    if (l->isObject() || r->isObject()) {
        QV4::ExecutionEngine *e = (l->isObject() ? l->objectValue() : r->objectValue())->engine();
        QV4::Scope scope(e);
        QV4::ScopedValue pl(scope, __qmljs_to_primitive(l, QV4::NUMBER_HINT));
        QV4::ScopedValue pr(scope, __qmljs_to_primitive(r, QV4::NUMBER_HINT));
        return __qmljs_cmp_le(pl, pr);
    }

    double dl = __qmljs_to_number(l);
    double dr = __qmljs_to_number(r);
    return dl <= dr;
}


ReturnedValue __qmljs_call_global_lookup(ExecutionContext *context, uint index, CallDataRef callData)
{
    Scope scope(context);
    Q_ASSERT(callData->thisObject.isUndefined());

    Lookup *l = context->lookups + index;
    Scoped<FunctionObject> o(scope, l->globalGetter(l, context));
    if (!o)
        return context->throwTypeError();

    if (o.getPointer() == context->engine->evalFunction && l->name->equals(context->engine->id_eval))
        return static_cast<EvalFunction *>(o.getPointer())->evalCall(callData, true);

    return o->call(callData);
}


ReturnedValue __qmljs_call_activation_property(ExecutionContext *context, const StringRef name, CallDataRef callData)
{
    Q_ASSERT(callData->thisObject.isUndefined());
    Scope scope(context);

    ScopedObject base(scope);
    ScopedValue func(scope, context->getPropertyAndBase(name, base));
    if (context->engine->hasException)
        return Encode::undefined();

    if (base)
        callData->thisObject = base;

    FunctionObject *o = func->asFunctionObject();
    if (!o) {
        QString objectAsString = QStringLiteral("[null]");
        if (base)
            objectAsString = ScopedValue(scope, base.asReturnedValue())->toQStringNoThrow();
        QString msg = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString()).arg(objectAsString);
        return context->throwTypeError(msg);
    }

    if (o == context->engine->evalFunction && name->equals(context->engine->id_eval)) {
        return static_cast<EvalFunction *>(o)->evalCall(callData, true);
    }

    return o->call(callData);
}

ReturnedValue __qmljs_call_property(ExecutionContext *context, const StringRef name, CallDataRef callData)
{
    Scope scope(context);
    Scoped<Object> baseObject(scope, callData->thisObject);
    if (!baseObject) {
        Q_ASSERT(!callData->thisObject.isEmpty());
        if (callData->thisObject.isNullOrUndefined()) {
            QString message = QStringLiteral("Cannot call method '%1' of %2").arg(name->toQString()).arg(callData->thisObject.toQStringNoThrow());
            return context->throwTypeError(message);
        }

        baseObject = __qmljs_convert_to_object(context, ValueRef(&callData->thisObject));
        if (!baseObject) // type error
            return Encode::undefined();
        callData->thisObject = baseObject.asReturnedValue();
    }

    Scoped<FunctionObject> o(scope, baseObject->get(name));
    if (!o) {
        QString error = QStringLiteral("Property '%1' of object %2 is not a function").arg(name->toQString(), callData->thisObject.toQStringNoThrow());
        return context->throwTypeError(error);
    }

    return o->call(callData);
}

ReturnedValue __qmljs_call_property_lookup(ExecutionContext *context, uint index, CallDataRef callData)
{
    Lookup *l = context->lookups + index;
    SafeValue v;
    v = l->getter(l, callData->thisObject);
    if (!v.isManaged())
        return context->throwTypeError();

    return v.managed()->call(callData);
}

ReturnedValue __qmljs_call_element(ExecutionContext *context, const ValueRef index, CallDataRef callData)
{
    Scope scope(context);
    ScopedObject baseObject(scope, callData->thisObject.toObject(context));
    ScopedString s(scope, index->toString(context));

    if (scope.engine->hasException)
        return Encode::undefined();
    callData->thisObject = baseObject;

    ScopedObject o(scope, baseObject->get(s));
    if (!o)
        return context->throwTypeError();

    return o->call(callData);
}

ReturnedValue __qmljs_call_value(ExecutionContext *context, const ValueRef func, CallDataRef callData)
{
    if (!func->isManaged())
        return context->throwTypeError();

    return func->managed()->call(callData);
}


ReturnedValue __qmljs_construct_global_lookup(ExecutionContext *context, uint index, CallDataRef callData)
{
    Scope scope(context);
    Q_ASSERT(callData->thisObject.isUndefined());

    Lookup *l = context->lookups + index;
    Scoped<Object> f(scope, l->globalGetter(l, context));
    if (!f)
        return context->throwTypeError();

    return f->construct(callData);
}


ReturnedValue __qmljs_construct_activation_property(ExecutionContext *context, const StringRef name, CallDataRef callData)
{
    Scope scope(context);
    ScopedValue func(scope, context->getProperty(name));
    if (context->engine->hasException)
        return Encode::undefined();

    Object *f = func->asObject();
    if (!f)
        return context->throwTypeError();

    return f->construct(callData);
}

ReturnedValue __qmljs_construct_value(ExecutionContext *context, const ValueRef func, CallDataRef callData)
{
    Object *f = func->asObject();
    if (!f)
        return context->throwTypeError();

    return f->construct(callData);
}

ReturnedValue __qmljs_construct_property(ExecutionContext *context, const StringRef name, CallDataRef callData)
{
    Scope scope(context);
    ScopedObject thisObject(scope, callData->thisObject.toObject(context));
    if (scope.engine->hasException)
        return Encode::undefined();

    Scoped<Object> f(scope, thisObject->get(name));
    if (!f)
        return context->throwTypeError();

    return f->construct(callData);
}

ReturnedValue __qmljs_construct_property_lookup(ExecutionContext *context, uint index, CallDataRef callData)
{
    Lookup *l = context->lookups + index;
    SafeValue v;
    v = l->getter(l, callData->thisObject);
    if (!v.isManaged())
        return context->throwTypeError();

    return v.managed()->construct(callData);
}


void __qmljs_throw(ExecutionContext *context, const ValueRef value)
{
    if (!value->isEmpty())
        context->throwError(value);
}

ReturnedValue __qmljs_builtin_typeof(ExecutionContext *ctx, const ValueRef value)
{
    Scope scope(ctx);
    ScopedString res(scope);
    switch (value->type()) {
    case Value::Undefined_Type:
        res = ctx->engine->id_undefined;
        break;
    case Value::Null_Type:
        res = ctx->engine->id_object;
        break;
    case Value::Boolean_Type:
        res = ctx->engine->id_boolean;
        break;
    case Value::Managed_Type:
        if (value->isString())
            res = ctx->engine->id_string;
        else if (value->objectValue()->asFunctionObject())
            res = ctx->engine->id_function;
        else
            res = ctx->engine->id_object; // ### implementation-defined
        break;
    default:
        res = ctx->engine->id_number;
        break;
    }
    return res.asReturnedValue();
}

QV4::ReturnedValue __qmljs_builtin_typeof_name(ExecutionContext *context, const StringRef name)
{
    Scope scope(context);
    ScopedValue prop(scope, context->getProperty(name));
    // typeof doesn't throw. clear any possible exception
    context->engine->hasException = false;
    return __qmljs_builtin_typeof(context, prop);
}

QV4::ReturnedValue __qmljs_builtin_typeof_member(ExecutionContext *context, const ValueRef base, const StringRef name)
{
    Scope scope(context);
    ScopedObject obj(scope, base->toObject(context));
    if (scope.engine->hasException)
        return Encode::undefined();
    ScopedValue prop(scope, obj->get(name));
    return __qmljs_builtin_typeof(context, prop);
}

QV4::ReturnedValue __qmljs_builtin_typeof_element(ExecutionContext *context, const ValueRef base, const ValueRef index)
{
    Scope scope(context);
    ScopedString name(scope, index->toString(context));
    ScopedObject obj(scope, base->toObject(context));
    if (scope.engine->hasException)
        return Encode::undefined();
    ScopedValue prop(scope, obj->get(name));
    return __qmljs_builtin_typeof(context, prop);
}

ExecutionContext *__qmljs_builtin_push_with_scope(const ValueRef o, ExecutionContext *ctx)
{
    Scope scope(ctx);
    ScopedObject obj(scope, o->toObject(ctx));
    return ctx->newWithContext(obj);
}

ReturnedValue __qmljs_builtin_unwind_exception(ExecutionContext *ctx)
{
    if (!ctx->engine->hasException)
        return Primitive::emptyValue().asReturnedValue();
    return ctx->engine->catchException(ctx, 0);
}

ExecutionContext *__qmljs_builtin_push_catch_scope(ExecutionContext *ctx, const StringRef exceptionVarName)
{
    Scope scope(ctx);
    ScopedValue v(scope, ctx->engine->catchException(ctx, 0));
    return ctx->newCatchContext(exceptionVarName, v);
}

ExecutionContext *__qmljs_builtin_pop_scope(ExecutionContext *ctx)
{
    return ctx->engine->popContext();
}

void __qmljs_builtin_declare_var(ExecutionContext *ctx, bool deletable, const StringRef name)
{
    ctx->createMutableBinding(name, deletable);
}

void __qmljs_builtin_define_property(ExecutionContext *ctx, const ValueRef object, const StringRef name, ValueRef val)
{
    Scope scope(ctx);
    ScopedObject o(scope, object->asObject());
    assert(o);

    uint idx = name->asArrayIndex();
    Property *pd = (idx != UINT_MAX) ? o->arrayInsert(idx) : o->insertMember(name, Attr_Data);
    pd->value = val ? *val : Primitive::undefinedValue();
}

ReturnedValue __qmljs_builtin_define_array(ExecutionContext *ctx, Value *values, uint length)
{
    Scope scope(ctx);
    Scoped<ArrayObject> a(scope, ctx->engine->newArrayObject());

    // ### FIXME: We need to allocate the array data to avoid crashes other places
    // This should rather be done when required
    a->arrayReserve(length);
    if (length) {
        a->arrayDataLen = length;
        Property *pd = a->arrayData;
        for (uint i = 0; i < length; ++i) {
            pd->value = values[i];
            ++pd;
        }
        a->setArrayLengthUnchecked(length);
    }
    return a.asReturnedValue();
}

void __qmljs_builtin_define_getter_setter(ExecutionContext *ctx, const ValueRef object, const StringRef name, const ValueRef getter, const ValueRef setter)
{
    Scope scope(ctx);
    ScopedObject o(scope, object->asObject());
    Q_ASSERT(!!o);

    uint idx = name->asArrayIndex();
    Property *pd = (idx != UINT_MAX) ? o->arrayInsert(idx, Attr_Accessor) : o->insertMember(name, Attr_Accessor);
    pd->setGetter(getter ? getter->asFunctionObject() : 0);
    pd->setSetter(setter ? setter->asFunctionObject() : 0);
}

ReturnedValue __qmljs_builtin_define_object_literal(QV4::ExecutionContext *ctx, const QV4::Value *args, int classId)
{
    Scope scope(ctx);
    QV4::InternalClass *klass = ctx->compilationUnit->runtimeClasses[classId];
    Scoped<Object> o(scope, ctx->engine->newObject(klass));

    for (uint i = 0; i < klass->size; ++i) {
        if (klass->propertyData[i].isData())
            o->memberData[i].value = *args++;
        else {
            o->memberData[i].setGetter(args->asFunctionObject());
            args++;
            o->memberData[i].setSetter(args->asFunctionObject());
            args++;
        }
    }

    return o.asReturnedValue();
}

QV4::ReturnedValue __qmljs_builtin_setup_arguments_object(ExecutionContext *ctx)
{
    assert(ctx->type >= ExecutionContext::Type_CallContext);
    CallContext *c = static_cast<CallContext *>(ctx);
    return (new (c->engine->memoryManager) ArgumentsObject(c))->asReturnedValue();
}

QV4::ReturnedValue __qmljs_increment(const QV4::ValueRef value)
{
    TRACE1(value);

    if (value->isInteger() && value->integerValue() < INT_MAX)
        return Encode(value->integerValue() + 1);
    else {
        double d = value->toNumber();
        return Encode(d + 1.);
    }
}

QV4::ReturnedValue __qmljs_decrement(const QV4::ValueRef value)
{
    TRACE1(value);

    if (value->isInteger() && value->integerValue() > INT_MIN)
        return Encode(value->integerValue() - 1);
    else {
        double d = value->toNumber();
        return Encode(d - 1.);
    }
}

QV4::ReturnedValue __qmljs_to_string(QV4::ExecutionContext *ctx, const QV4::ValueRef value)
{
    if (value->isString())
        return value.asReturnedValue();
    return __qmljs_convert_to_string(ctx, value)->asReturnedValue();
}

QV4::ReturnedValue __qmljs_to_object(QV4::ExecutionContext *ctx, const QV4::ValueRef value)
{
    if (value->isObject())
        return value.asReturnedValue();

    Returned<Object> *o = __qmljs_convert_to_object(ctx, value);
    if (!o) // type error
        return Encode::undefined();

    return Encode(o);
}

ReturnedValue __qmljs_value_to_double(const ValueRef value)
{
    TRACE1(value);
    return Encode(value->toNumber());
}

int __qmljs_value_to_int32(const ValueRef value)
{
    TRACE1(value);
    return value->toInt32();
}

int __qmljs_double_to_int32(const double &d)
{
    TRACE0();
    return Primitive::toInt32(d);
}

unsigned __qmljs_value_to_uint32(const ValueRef value)
{
    TRACE1(value);
    return value->toUInt32();
}

unsigned __qmljs_double_to_uint32(const double &d)
{
    TRACE0();
    return Primitive::toUInt32(d);
}

ReturnedValue __qmljs_value_from_string(String *string)
{
    TRACE0();
    return string->asReturnedValue();
}

ReturnedValue __qmljs_lookup_runtime_regexp(ExecutionContext *ctx, int id)
{
    return ctx->compilationUnit->runtimeRegularExpressions[id].asReturnedValue();
}

ReturnedValue __qmljs_get_id_array(NoThrowContext *ctx)
{
    return ctx->engine->qmlContextObject()->getPointer()->as<QmlContextWrapper>()->idObjectsArray();
}

ReturnedValue __qmljs_get_context_object(NoThrowContext *ctx)
{
    QQmlContextData *context = QmlContextWrapper::callingContext(ctx->engine);
    return QObjectWrapper::wrap(ctx->engine, context->contextObject);
}

ReturnedValue __qmljs_get_scope_object(NoThrowContext *ctx)
{
    Scope scope(ctx);
    QV4::Scoped<QmlContextWrapper> c(scope, ctx->engine->qmlContextObject()->getPointer()->as<QmlContextWrapper>());
    return QObjectWrapper::wrap(ctx->engine, c->getScopeObject());
}

ReturnedValue __qmljs_get_qobject_property(ExecutionContext *ctx, const ValueRef object, int propertyIndex, bool captureRequired)
{
    Scope scope(ctx);
    QV4::Scoped<QObjectWrapper> wrapper(scope, object);
    if (!wrapper) {
        ctx->throwTypeError(QStringLiteral("Cannot read property of null"));
        return Encode::undefined();
    }
    return QV4::QObjectWrapper::getProperty(wrapper->object(), ctx, propertyIndex, captureRequired);
}

QV4::ReturnedValue __qmljs_get_attached_property(ExecutionContext *ctx, int attachedPropertiesId, int propertyIndex)
{
    Scope scope(ctx);
    QV4::Scoped<QmlContextWrapper> c(scope, ctx->engine->qmlContextObject()->getPointer()->as<QmlContextWrapper>());
    QObject *scopeObject = c->getScopeObject();
    QObject *attachedObject = qmlAttachedPropertiesObjectById(attachedPropertiesId, scopeObject);

    QQmlEngine *qmlEngine = ctx->engine->v8Engine->engine();
    QQmlData::ensurePropertyCache(qmlEngine, attachedObject);
    return QV4::QObjectWrapper::getProperty(attachedObject, ctx, propertyIndex, /*captureRequired*/true);
}

void __qmljs_set_qobject_property(ExecutionContext *ctx, const ValueRef object, int propertyIndex, const ValueRef value)
{
    Scope scope(ctx);
    QV4::Scoped<QObjectWrapper> wrapper(scope, object);
    if (!wrapper) {
        ctx->throwTypeError(QStringLiteral("Cannot write property of null"));
        return;
    }
    wrapper->setProperty(ctx, propertyIndex, value);
}

ReturnedValue __qmljs_get_imported_scripts(NoThrowContext *ctx)
{
    QQmlContextData *context = QmlContextWrapper::callingContext(ctx->engine);
    return context->importedScripts.value();
}

QV4::ReturnedValue __qmljs_get_qml_singleton(QV4::NoThrowContext *ctx, const QV4::StringRef name)
{
    return ctx->engine->qmlContextObject()->getPointer()->as<QmlContextWrapper>()->qmlSingletonWrapper(name);
}

void __qmljs_builtin_convert_this_to_object(ExecutionContext *ctx)
{
    SafeValue *t = &ctx->callData->thisObject;
    if (t->isObject())
        return;
    if (t->isNullOrUndefined()) {
        *t = ctx->engine->globalObject->asReturnedValue();
    } else {
        *t = t->toObject(ctx)->asReturnedValue();
    }
}

} // namespace QV4

QT_END_NAMESPACE
