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

#include "qv4arrayobject.h"
#include "qv4array.h"

using namespace QQmlJS::VM;

ArrayCtor::ArrayCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value ArrayCtor::call(ExecutionContext *ctx)
{
    ArrayObject *a = ctx->engine->newArrayObject(ctx);
    Array &value = a->array;
    if (ctx->argumentCount == 1 && ctx->argument(0).isNumber()) {
        bool ok;
        uint len = ctx->argument(0).asArrayLength(ctx, &ok);

        if (!ok) {
            ctx->throwRangeError(ctx->argument(0));
            return Value::undefinedValue();
        }

        value.setLengthUnchecked(len);
    } else {
        for (unsigned int i = 0; i < ctx->argumentCount; ++i) {
            value.set(i, ctx->argument(i));
        }
    }

    return Value::fromObject(a);
}

void ArrayPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("isArray"), method_isArray, 1);
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    defineDefaultProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(ctx, QStringLiteral("join"), method_join, 1);
    defineDefaultProperty(ctx, QStringLiteral("pop"), method_pop, 0);
    defineDefaultProperty(ctx, QStringLiteral("push"), method_push, 1);
    defineDefaultProperty(ctx, QStringLiteral("reverse"), method_reverse, 0);
    defineDefaultProperty(ctx, QStringLiteral("shift"), method_shift, 0);
    defineDefaultProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(ctx, QStringLiteral("sort"), method_sort, 1);
    defineDefaultProperty(ctx, QStringLiteral("splice"), method_splice, 2);
    defineDefaultProperty(ctx, QStringLiteral("unshift"), method_unshift, 1);
    defineDefaultProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("every"), method_every, 1);
    defineDefaultProperty(ctx, QStringLiteral("some"), method_some, 1);
    defineDefaultProperty(ctx, QStringLiteral("forEach"), method_forEach, 1);
    defineDefaultProperty(ctx, QStringLiteral("map"), method_map, 1);
    defineDefaultProperty(ctx, QStringLiteral("filter"), method_filter, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduce"), method_reduce, 1);
    defineDefaultProperty(ctx, QStringLiteral("reduceRight"), method_reduceRight, 1);
}

uint ArrayPrototype::getLength(ExecutionContext *ctx, Object *o)
{
    if (o->isArrayObject())
        return o->array.length();
    return o->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
}

Value ArrayPrototype::method_isArray(ExecutionContext *ctx)
{
    Value arg = ctx->argument(0);
    bool isArray = arg.asArrayObject();
    return Value::fromBoolean(isArray);
}

Value ArrayPrototype::method_toString(ExecutionContext *ctx)
{
    return method_join(ctx);
}

Value ArrayPrototype::method_toLocaleString(ExecutionContext *ctx)
{
    return method_toString(ctx);
}

Value ArrayPrototype::method_concat(ExecutionContext *ctx)
{
    Array result;

    if (ArrayObject *instance = ctx->thisObject.asArrayObject())
        result = instance->array;
    else {
        QString v = ctx->thisObject.toString(ctx)->toQString();
        result.set(0, Value::fromString(ctx, v));
    }

    for (uint i = 0; i < ctx->argumentCount; ++i) {
        quint32 k = result.length();
        Value arg = ctx->argument(i);

        if (ArrayObject *elt = arg.asArrayObject())
            result.concat(elt->array);

        else
            result.set(k, arg);
    }

    return Value::fromObject(ctx->engine->newArrayObject(ctx, result));
}

Value ArrayPrototype::method_join(ExecutionContext *ctx)
{
    Value arg = ctx->argument(0);

    QString r4;
    if (arg.isUndefined())
        r4 = QStringLiteral(",");
    else
        r4 = arg.toString(ctx)->toQString();

    Value self = ctx->thisObject;
    const Value length = self.property(ctx, ctx->engine->id_length);
    const quint32 r2 = Value::toUInt32(length.isUndefined() ? 0 : length.toNumber(ctx));

    static QSet<Object *> visitedArrayElements;

    if (! r2 || visitedArrayElements.contains(self.objectValue()))
        return Value::fromString(ctx, QString());

    // avoid infinite recursion
    visitedArrayElements.insert(self.objectValue());

    QString R;

    // ### FIXME
    if (ArrayObject *a = self.asArrayObject()) {
        for (uint i = 0; i < a->array.length(); ++i) {
            if (i)
                R += r4;

            Value e = a->__get__(ctx, i);
            if (! (e.isUndefined() || e.isNull()))
                R += e.toString(ctx)->toQString();
        }
    } else {
        //
        // crazy!
        //
        Value r6 = self.property(ctx, ctx->engine->identifier(QStringLiteral("0")));
        if (!(r6.isUndefined() || r6.isNull()))
            R = r6.toString(ctx)->toQString();

        for (quint32 k = 1; k < r2; ++k) {
            R += r4;

            String *name = Value::fromDouble(k).toString(ctx);
            Value r12 = self.property(ctx, name);

            if (! (r12.isUndefined() || r12.isNull()))
                R += r12.toString(ctx)->toQString();
        }
    }

    visitedArrayElements.remove(self.objectValue());
    return Value::fromString(ctx, R);
}

Value ArrayPrototype::method_pop(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    if (!len) {
        if (!instance->isArrayObject())
            instance->__put__(ctx, ctx->engine->id_length, Value::fromInt32(0));
        return Value::undefinedValue();
    }

    Value result = instance->__get__(ctx, len - 1);

    instance->__delete__(ctx, len - 1);
    if (instance->isArrayObject())
        instance->array.setLengthUnchecked(len - 1);
    else
        instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(len - 1));
    return result;
}

Value ArrayPrototype::method_push(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    if (len + ctx->argumentCount < len) {
        // ughh...
        double l = len;
        for (double i = 0; i < ctx->argumentCount; ++i) {
            Value idx = Value::fromDouble(l + i);
            instance->__put__(ctx, idx.toString(ctx), ctx->argument(i));
        }
        double newLen = l + ctx->argumentCount;
        if (!instance->isArrayObject())
            instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(newLen));
        else
            ctx->throwRangeError(Value::fromString(ctx, QStringLiteral("Array.prototype.push: Overflow")));
        return Value::fromDouble(newLen);
    }

    bool protoHasArray = false;
    Object *p = instance;
    while ((p = p->prototype))
        if (p->array.length())
            protoHasArray = true;

    if (!protoHasArray && len == instance->array.length()) {
        for (uint i = 0; i < ctx->argumentCount; ++i) {
            Value v = ctx->argument(i);
            instance->array.push_back(v);
        }
    } else {
        for (uint i = 0; i < ctx->argumentCount; ++i)
            instance->__put__(ctx, len + i, ctx->argument(i));
    }
    uint newLen = len + ctx->argumentCount;
    if (!instance->isArrayObject())
        instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(newLen));

    if (newLen < INT_MAX)
        return Value::fromInt32(newLen);
    return Value::fromDouble((double)newLen);

}

Value ArrayPrototype::method_reverse(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint length = getLength(ctx, instance);

    int lo = 0, hi = length - 1;

    for (; lo < hi; ++lo, --hi) {
        bool loExists, hiExists;
        Value lval = instance->__get__(ctx, lo, &loExists);
        Value hval = instance->__get__(ctx, hi, &hiExists);
        if (hiExists)
            instance->__put__(ctx, lo, hval);
        else
            instance->__delete__(ctx, lo);
        if (loExists)
            instance->__put__(ctx, hi, lval);
        else
            instance->__delete__(ctx, hi);
    }
    return Value::fromObject(instance);
}

Value ArrayPrototype::method_shift(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    if (!len) {
        if (!instance->isArrayObject())
            instance->__put__(ctx, ctx->engine->id_length, Value::fromInt32(0));
        return Value::undefinedValue();
    }

    Value result = instance->getValueChecked(ctx, instance->array.front());

    bool protoHasArray = false;
    Object *p = instance;
    while ((p = p->prototype))
        if (p->array.length())
            protoHasArray = true;

    if (!protoHasArray && len >= instance->array.length()) {
        instance->array.pop_front();
    } else {
        // do it the slow way
        for (uint k = 1; k < len; ++k) {
            bool exists;
            Value v = instance->__get__(ctx, k, &exists);
            if (exists)
                instance->__put__(ctx, k - 1, v);
            else
                instance->__delete__(ctx, k - 1);
        }
        instance->__delete__(ctx, len - 1);
    }

    if (!instance->isArrayObject())
        instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(len - 1));
    return result;
}

Value ArrayPrototype::method_slice(ExecutionContext *ctx)
{
    Object *o = ctx->thisObject.toObject(ctx).objectValue();

    Array result;
    uint len = o->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
    double s = ctx->argument(0).toInteger(ctx);
    uint start;
    if (s < 0)
        start = (uint)qMax(len + s, 0.);
    else if (s > len)
        start = len;
    else
        start = (uint) s;
    uint end = len;
    if (!ctx->argument(1).isUndefined()) {
        double e = ctx->argument(1).toInteger(ctx);
        if (e < 0)
            end = (uint)qMax(len + e, 0.);
        else if (e > len)
            end = len;
        else
            end = (uint) e;
    }

    uint n = 0;
    for (uint i = start; i < end; ++i) {
        bool exists;
        Value v = o->__get__(ctx, i, &exists);
        if (exists)
            result.set(n, v);
        ++n;
    }
    return Value::fromObject(ctx->engine->newArrayObject(ctx, result));
}

Value ArrayPrototype::method_sort(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    Value comparefn = ctx->argument(0);
    instance->array.sort(ctx, instance, comparefn, len);
    return ctx->thisObject;
}

Value ArrayPrototype::method_splice(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    ArrayObject *newArray = ctx->engine->newArrayObject(ctx);

    double rs = ctx->argument(0).toInteger(ctx);
    uint start;
    if (rs < 0)
        start = (uint) qMax(0., len + rs);
    else
        start = (uint) qMin(rs, (double)len);

    uint deleteCount = (uint)qMin(qMax(ctx->argument(1).toInteger(ctx), 0.), (double)(len - start));

    newArray->array.values.resize(deleteCount);
    PropertyDescriptor *pd = newArray->array.values.data();
    for (uint i = 0; i < deleteCount; ++i) {
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->value = instance->__get__(ctx, start + i);
        ++pd;
    }
    newArray->array.setLengthUnchecked(deleteCount);

    uint itemCount = ctx->argumentCount < 2 ? 0 : ctx->argumentCount - 2;

    if (itemCount < deleteCount) {
        for (uint k = start; k < len - deleteCount; ++k) {
            bool exists;
            Value v = instance->__get__(ctx, k + deleteCount, &exists);
            if (exists)
                instance->array.set(k + itemCount, v);
            else
                instance->__delete__(ctx, k + itemCount);
        }
        for (uint k = len; k > len - deleteCount + itemCount; --k)
            instance->__delete__(ctx, k - 1);
    } else if (itemCount > deleteCount) {
        uint k = len - deleteCount;
        while (k > start) {
            bool exists;
            Value v = instance->__get__(ctx, k + deleteCount - 1, &exists);
            if (exists)
                instance->array.set(k + itemCount - 1, v);
            else
                instance->__delete__(ctx, k + itemCount - 1);
            --k;
        }
    }

    for (uint i = 0; i < itemCount; ++i)
        instance->array.set(start + i, ctx->argument(i + 2));

    ctx->strictMode = true;
    instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(len - deleteCount + itemCount));

    return Value::fromObject(newArray);
}

Value ArrayPrototype::method_unshift(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);

    bool protoHasArray = false;
    Object *p = instance;
    while ((p = p->prototype))
        if (p->array.length())
            protoHasArray = true;

    if (!protoHasArray && len >= instance->array.length()) {
        for (int i = ctx->argumentCount - 1; i >= 0; --i) {
            Value v = ctx->argument(i);
            instance->array.push_front(v);
        }
    } else {
        for (uint k = len; k > 0; --k) {
            bool exists;
            Value v = instance->__get__(ctx, k - 1, &exists);
            if (exists)
                instance->__put__(ctx, k + ctx->argumentCount - 1, v);
            else
                instance->__delete__(ctx, k + ctx->argumentCount - 1);
        }
        for (uint i = 0; i < ctx->argumentCount; ++i)
            instance->__put__(ctx, i, ctx->argument(i));
    }
    uint newLen = len + ctx->argumentCount;
    if (!instance->isArrayObject())
        instance->__put__(ctx, ctx->engine->id_length, Value::fromDouble(newLen));

    if (newLen < INT_MAX)
        return Value::fromInt32(newLen);
    return Value::fromDouble((double)newLen);
}

Value ArrayPrototype::method_indexOf(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1);

    Value searchValue;
    uint fromIndex = 0;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger(ctx);
        if (f >= len)
            return Value::fromInt32(-1);
        if (f < 0)
            f = qMax(len + f, 0.);
        fromIndex = (uint) f;
    }

    if (instance->isStringObject()) {
        for (uint k = fromIndex; k < len; ++k) {
            bool exists;
            Value v = instance->__get__(ctx, k, &exists);
            if (exists && __qmljs_strict_equal(v, searchValue))
                return Value::fromDouble(k);
        }
        return Value::fromInt32(-1);
    }

    return instance->array.indexOf(searchValue, fromIndex, len, ctx, instance);
}

Value ArrayPrototype::method_lastIndexOf(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();
    uint len = getLength(ctx, instance);
    if (!len)
        return Value::fromInt32(-1);

    Value searchValue;
    uint fromIndex = len;

    if (ctx->argumentCount >= 1)
        searchValue = ctx->argument(0);
    else
        searchValue = Value::undefinedValue();

    if (ctx->argumentCount >= 2) {
        double f = ctx->argument(1).toInteger(ctx);
        if (f > 0)
            f = qMin(f, (double)(len - 1));
        else if (f < 0) {
            f = len + f;
            if (f < 0)
                return Value::fromInt32(-1);
        }
        fromIndex = (uint) f + 1;
    }

    for (uint k = fromIndex; k > 0;) {
        --k;
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (exists && __qmljs_strict_equal(v, searchValue))
            return Value::fromDouble(k);
    }
    return Value::fromInt32(-1);
}

Value ArrayPrototype::method_every(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    bool ok = true;
    for (uint k = 0; ok && k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value r = callback->call(ctx, thisArg, args, 3);
        ok = __qmljs_to_boolean(r, ctx);
    }
    return Value::fromBoolean(ok);
}

Value ArrayPrototype::method_some(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value r = callback->call(ctx, thisArg, args, 3);
        if (__qmljs_to_boolean(r, ctx))
            return Value::fromBoolean(true);
    }
    return Value::fromBoolean(false);
}

Value ArrayPrototype::method_forEach(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        callback->call(ctx, thisArg, args, 3);
    }
    return Value::undefinedValue();
}

Value ArrayPrototype::method_map(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject(ctx);
    a->array.setLength(len);

    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value mapped = callback->call(ctx, thisArg, args, 3);
        a->array.set(k, mapped);
    }
    return Value::fromObject(a);
}

Value ArrayPrototype::method_filter(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    Value thisArg = ctx->argument(1);

    ArrayObject *a = ctx->engine->newArrayObject(ctx);

    uint to = 0;
    for (uint k = 0; k < len; ++k) {
        bool exists;
        Value v = instance->__get__(ctx, k, &exists);
        if (!exists)
            continue;

        Value args[3];
        args[0] = v;
        args[1] = Value::fromDouble(k);
        args[2] = ctx->thisObject;
        Value selected = callback->call(ctx, thisArg, args, 3);
        if (__qmljs_to_boolean(selected, ctx)) {
            a->array.set(to, v);
            ++to;
        }
    }
    return Value::fromObject(a);
}

Value ArrayPrototype::method_reduce(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    uint k = 0;
    Value acc;
    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k < len && !kPresent) {
            Value v = instance->__get__(ctx, k, &kPresent);
            if (kPresent)
                acc = v;
            ++k;
        }
        if (!kPresent)
            __qmljs_throw_type_error(ctx);
    }

    while (k < len) {
        bool kPresent;
        Value v = instance->__get__(ctx, k, &kPresent);
        if (kPresent) {
            Value args[4];
            args[0] = acc;
            args[1] = v;
            args[2] = Value::fromDouble(k);
            args[3] = ctx->thisObject;
            acc = callback->call(ctx, Value::undefinedValue(), args, 4);
        }
        ++k;
    }
    return acc;
}

Value ArrayPrototype::method_reduceRight(ExecutionContext *ctx)
{
    Object *instance = __qmljs_to_object(ctx->thisObject, ctx).objectValue();

    uint len = getLength(ctx, instance);

    FunctionObject *callback = ctx->argument(0).asFunctionObject();
    if (!callback)
        __qmljs_throw_type_error(ctx);

    if (len == 0) {
        if (ctx->argumentCount == 1)
            __qmljs_throw_type_error(ctx);
        return ctx->argument(1);
    }

    uint k = len;
    Value acc;
    if (ctx->argumentCount > 1) {
        acc = ctx->argument(1);
    } else {
        bool kPresent = false;
        while (k > 0 && !kPresent) {
            Value v = instance->__get__(ctx, k - 1, &kPresent);
            if (kPresent)
                acc = v;
            --k;
        }
        if (!kPresent)
            __qmljs_throw_type_error(ctx);
    }

    while (k > 0) {
        bool kPresent;
        Value v = instance->__get__(ctx, k - 1, &kPresent);
        if (kPresent) {
            Value args[4];
            args[0] = acc;
            args[1] = v;
            args[2] = Value::fromDouble(k - 1);
            args[3] = ctx->thisObject;
            acc = callback->call(ctx, Value::undefinedValue(), args, 4);
        }
        --k;
    }
    return acc;
}

