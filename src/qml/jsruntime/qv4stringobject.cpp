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


#include "qv4stringobject_p.h"
#include "qv4regexpobject_p.h"
#include "qv4objectproto_p.h"
#include "qv4mm_p.h"
#include "qv4scopedvalue_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <cmath>
#include <qmath.h>
#include <qnumeric.h>
#include <cassert>

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>

#ifndef Q_OS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QV4;

DEFINE_MANAGED_VTABLE(StringObject);

StringObject::StringObject(InternalClass *ic)
    : Object(ic), value(Value::fromString(ic->engine, ""))
{
    vtbl = &static_vtbl;
    type = Type_StringObject;

    tmpProperty.value = Value::undefinedValue();

    defineReadonlyProperty(ic->engine->id_length, Value::fromInt32(0));
}

StringObject::StringObject(ExecutionEngine *engine, const Value &value)
    : Object(engine->stringClass), value(value)
{
    vtbl = &static_vtbl;
    type = Type_StringObject;

    tmpProperty.value = Value::undefinedValue();

    assert(value.isString());
    defineReadonlyProperty(engine->id_length, Value::fromUInt32(value.stringValue()->toQString().length()));
}

Property *StringObject::getIndex(uint index) const
{
    QString str = value.stringValue()->toQString();
    if (index >= (uint)str.length())
        return 0;
    String *result = internalClass->engine->newString(str.mid(index, 1));
    tmpProperty.value = Value::fromString(result);
    return &tmpProperty;
}

bool StringObject::deleteIndexedProperty(Managed *m, uint index)
{
    StringObject *o = m->asStringObject();
    if (!o)
        m->engine()->current->throwTypeError();

    if (index < o->value.stringValue()->toQString().length()) {
        if (m->engine()->current->strictMode)
            m->engine()->current->throwTypeError();
        return false;
    }
    return true;
}

Property *StringObject::advanceIterator(Managed *m, ObjectIterator *it, String **name, uint *index, PropertyAttributes *attrs)
{
    StringObject *s = static_cast<StringObject *>(m);
    uint slen = s->value.stringValue()->toQString().length();
    if (it->arrayIndex < slen) {
        while (it->arrayIndex < slen) {
            *index = it->arrayIndex;
            ++it->arrayIndex;
            if (attrs)
                *attrs = s->arrayAttributes ? s->arrayAttributes[it->arrayIndex] : PropertyAttributes(Attr_NotWritable|Attr_NotConfigurable);
            return s->__getOwnProperty__(*index);
        }
        it->arrayNode = s->sparseArrayBegin();
        // iterate until we're past the end of the string
        while (it->arrayNode && it->arrayNode->key() < slen)
            it->arrayNode = it->arrayNode->nextNode();
    }

    return Object::advanceIterator(m, it, name, index, attrs);
}

void StringObject::markObjects(Managed *that)
{
    StringObject *o = static_cast<StringObject *>(that);
    o->value.stringValue()->mark();
    Object::markObjects(that);
}

DEFINE_MANAGED_VTABLE(StringCtor);

StringCtor::StringCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("String")))
{
    vtbl = &static_vtbl;
}

ReturnedValue StringCtor::construct(Managed *m, CallData *callData)
{
    Value value;
    if (callData->argc)
        value = Value::fromString(callData->args[0].toString(m->engine()->current));
    else
        value = Value::fromString(m->engine()->current, QString());
    return Encode(m->engine()->newStringObject(value));
}

ReturnedValue StringCtor::call(Managed *m, CallData *callData)
{
    Value value;
    if (callData->argc)
        value = Value::fromString(callData->args[0].toString(m->engine()->current));
    else
        value = Value::fromString(m->engine()->current, QString());
    return value.asReturnedValue();
}

void StringPrototype::init(ExecutionEngine *engine, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(QStringLiteral("fromCharCode"), method_fromCharCode, 1);

    defineDefaultProperty(QStringLiteral("constructor"), ctor);
    defineDefaultProperty(QStringLiteral("toString"), method_toString);
    defineDefaultProperty(QStringLiteral("valueOf"), method_toString); // valueOf and toString are identical
    defineDefaultProperty(QStringLiteral("charAt"), method_charAt, 1);
    defineDefaultProperty(QStringLiteral("charCodeAt"), method_charCodeAt, 1);
    defineDefaultProperty(QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(QStringLiteral("localeCompare"), method_localeCompare, 1);
    defineDefaultProperty(QStringLiteral("match"), method_match, 1);
    defineDefaultProperty(QStringLiteral("replace"), method_replace, 2);
    defineDefaultProperty(QStringLiteral("search"), method_search, 1);
    defineDefaultProperty(QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(QStringLiteral("split"), method_split, 2);
    defineDefaultProperty(QStringLiteral("substr"), method_substr, 2);
    defineDefaultProperty(QStringLiteral("substring"), method_substring, 2);
    defineDefaultProperty(QStringLiteral("toLowerCase"), method_toLowerCase);
    defineDefaultProperty(QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    defineDefaultProperty(QStringLiteral("toUpperCase"), method_toUpperCase);
    defineDefaultProperty(QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
    defineDefaultProperty(QStringLiteral("trim"), method_trim);
}

static QString getThisString(ExecutionContext *ctx)
{
    String* str = 0;
    Value thisObject = ctx->thisObject;
    if (StringObject *thisString = thisObject.asStringObject())
        str = thisString->value.stringValue();
    else if (thisObject.isUndefined() || thisObject.isNull())
        ctx->throwTypeError();
    else
        str = ctx->thisObject.toString(ctx);
    return str->toQString();
}

static QString getThisString(ExecutionContext *context, Value thisObject)
{
    if (thisObject.isString())
        return thisObject.stringValue()->toQString();

    String* str = 0;
    if (StringObject *thisString = thisObject.asStringObject())
        str = thisString->value.stringValue();
    else if (thisObject.isUndefined() || thisObject.isNull())
        context->throwTypeError();
    else
        str = thisObject.toString(context);
    return str->toQString();
}

ReturnedValue StringPrototype::method_toString(SimpleCallContext *context)
{
    if (context->thisObject.isString())
        return context->thisObject.asReturnedValue();

    StringObject *o = context->thisObject.asStringObject();
    if (!o)
        context->throwTypeError();
    return o->value.asReturnedValue();
}

ReturnedValue StringPrototype::method_charAt(SimpleCallContext *context)
{
    const QString str = getThisString(context, context->thisObject);

    int pos = 0;
    if (context->argumentCount > 0)
        pos = (int) context->arguments[0].toInteger();

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return Value::fromString(context, result).asReturnedValue();
}

ReturnedValue StringPrototype::method_charCodeAt(SimpleCallContext *context)
{
    const QString str = getThisString(context, context->thisObject);

    int pos = 0;
    if (context->argumentCount > 0)
        pos = (int) context->arguments[0].toInteger();


    if (pos >= 0 && pos < str.length())
        return Encode(str.at(pos).unicode());

    return Encode(qSNaN());
}

ReturnedValue StringPrototype::method_concat(SimpleCallContext *context)
{
    Scope scope(context);

    QString value = getThisString(context, context->thisObject);

    ScopedValue v(scope);
    for (int i = 0; i < context->argumentCount; ++i) {
        v = __qmljs_to_string(ValueRef(&context->arguments[i]), context);
        assert(v->isString());
        value += v->stringValue()->toQString();
    }

    return Value::fromString(context, value).asReturnedValue();
}

ReturnedValue StringPrototype::method_indexOf(SimpleCallContext *context)
{
    QString value = getThisString(context, context->thisObject);

    QString searchString;
    if (context->argumentCount)
        searchString = context->arguments[0].toString(context)->toQString();

    int pos = 0;
    if (context->argumentCount > 1)
        pos = (int) context->arguments[1].toInteger();

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    return Encode(index);
}

ReturnedValue StringPrototype::method_lastIndexOf(SimpleCallContext *context)
{
    Scope scope(context);

    const QString value = getThisString(context, context->thisObject);

    QString searchString;
    if (context->argumentCount)
        searchString = context->arguments[0].toQString();

    ScopedValue posArg(scope, context->argumentCount > 1 ? context->arguments[1] : Value::undefinedValue());
    double position = __qmljs_to_number(posArg);
    if (std::isnan(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    if (searchString.isNull() && pos == 0)
        return Encode(-1);
    int index = value.lastIndexOf(searchString, pos);
    return Encode(index);
}

ReturnedValue StringPrototype::method_localeCompare(SimpleCallContext *context)
{
    const QString value = getThisString(context, context->thisObject);
    const QString that = (context->argumentCount ? context->arguments[0] : Value::undefinedValue()).toString(context)->toQString();
    return Encode(QString::localeAwareCompare(value, that));
}

ReturnedValue StringPrototype::method_match(SimpleCallContext *context)
{
    if (context->thisObject.isUndefined() || context->thisObject.isNull())
        context->throwTypeError();

    Scope scope(context);
    String *s = context->thisObject.toString(context);

    Value regexp = context->argumentCount ? context->arguments[0] : Value::undefinedValue();
    RegExpObject *rx = regexp.as<RegExpObject>();
    if (!rx) {
        ScopedCallData callData(scope, 1);
        callData->args[0] = regexp;
        rx = Value::fromReturnedValue(context->engine->regExpCtor.asFunctionObject()->construct(callData)).as<RegExpObject>();
    }

    if (!rx)
        // ### CHECK
        context->throwTypeError();

    bool global = rx->global;

    // ### use the standard builtin function, not the one that might be redefined in the proto
    Scoped<FunctionObject> exec(scope, context->engine->regExpClass->prototype->get(context->engine->newString(QStringLiteral("exec")), 0));

    ScopedCallData callData(scope, 1);
    callData->thisObject = Value::fromObject(rx);
    callData->args[0] = Value::fromString(s);
    if (!global)
        return exec->call(callData);

    String *lastIndex = context->engine->newString(QStringLiteral("lastIndex"));
    rx->put(lastIndex, Value::fromInt32(0));
    Scoped<ArrayObject> a(scope, context->engine->newArrayObject());

    double previousLastIndex = 0;
    uint n = 0;
    ScopedValue result(scope);
    ScopedValue matchStr(scope);
    ScopedValue index(scope);
    while (1) {
        result = exec->call(callData);
        if (result->isNull())
            break;
        assert(result->isObject());
        index = rx->get(lastIndex, 0);
        double thisIndex = index->toInteger();
        if (previousLastIndex == thisIndex) {
            previousLastIndex = thisIndex + 1;
            rx->put(lastIndex, Value::fromDouble(previousLastIndex));
        } else {
            previousLastIndex = thisIndex;
        }
        matchStr = result->objectValue()->getIndexed(0);
        a->arraySet(n, matchStr);
        ++n;
    }
    if (!n)
        return Encode::null();

    return a.asReturnedValue();

}

static void appendReplacementString(QString *result, const QString &input, const QString& replaceValue, uint* matchOffsets, int captureCount)
{
    result->reserve(result->length() + replaceValue.length());
    for (int i = 0; i < replaceValue.length(); ++i) {
        if (replaceValue.at(i) == QLatin1Char('$') && i < replaceValue.length() - 1) {
            ushort ch = replaceValue.at(++i).unicode();
            uint substStart = JSC::Yarr::offsetNoMatch;
            uint substEnd = JSC::Yarr::offsetNoMatch;
            if (ch == '$') {
                *result += ch;
                continue;
            } else if (ch == '&') {
                substStart = matchOffsets[0];
                substEnd = matchOffsets[1];
            } else if (ch == '`') {
                substStart = 0;
                substEnd = matchOffsets[0];
            } else if (ch == '\'') {
                substStart = matchOffsets[1];
                substEnd = input.length();
            } else if (ch >= '1' && ch <= '9') {
                uint capture = ch - '0';
                if (capture > 0 && capture < captureCount) {
                    substStart = matchOffsets[capture * 2];
                    substEnd = matchOffsets[capture * 2 + 1];
                }
            } else if (ch == '0' && i < replaceValue.length() - 1) {
                int capture = (ch - '0') * 10;
                ch = replaceValue.at(++i).unicode();
                if (ch >= '0' && ch <= '9') {
                    capture += ch - '0';
                    if (capture > 0 && capture < captureCount) {
                        substStart = matchOffsets[capture * 2];
                        substEnd = matchOffsets[capture * 2 + 1];
                    }
                }
            }
            if (substStart != JSC::Yarr::offsetNoMatch && substEnd != JSC::Yarr::offsetNoMatch)
                *result += input.midRef(substStart, substEnd - substStart);
        } else {
            *result += replaceValue.at(i);
        }
    }
}

ReturnedValue StringPrototype::method_replace(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    int numCaptures = 0;
    int numStringMatches = 0;

    uint allocatedMatchOffsets = 32;
    uint _matchOffsets[32];
    uint *matchOffsets = _matchOffsets;
    uint nMatchOffsets = 0;

    ScopedValue searchValue(scope, ctx->argument(0));
    Scoped<RegExpObject> regExp(scope, searchValue);
    if (regExp) {
        uint offset = 0;
        while (true) {
            int oldSize = nMatchOffsets;
            if (allocatedMatchOffsets < nMatchOffsets + regExp->value->captureCount() * 2) {
                allocatedMatchOffsets = qMax(allocatedMatchOffsets * 2, nMatchOffsets + regExp->value->captureCount() * 2);
                uint *newOffsets = (uint *)malloc(allocatedMatchOffsets*sizeof(uint));
                memcpy(newOffsets, matchOffsets, nMatchOffsets*sizeof(uint));
                if (matchOffsets != _matchOffsets)
                    free(matchOffsets);
                matchOffsets = newOffsets;
            }
            if (regExp->value->match(string, offset, matchOffsets + oldSize) == JSC::Yarr::offsetNoMatch) {
                nMatchOffsets = oldSize;
                break;
            }
            nMatchOffsets += regExp->value->captureCount() * 2;
            if (!regExp->global)
                break;
            offset = qMax(offset + 1, matchOffsets[oldSize + 1]);
        }
        if (regExp->global)
            regExp->lastIndexProperty(ctx)->value = Value::fromUInt32(0);
        numStringMatches = nMatchOffsets / (regExp->value->captureCount() * 2);
        numCaptures = regExp->value->captureCount();
    } else {
        numCaptures = 1;
        QString searchString = searchValue->toString(ctx)->toQString();
        int idx = string.indexOf(searchString);
        if (idx != -1) {
            numStringMatches = 1;
            nMatchOffsets = 2;
            matchOffsets[0] = idx;
            matchOffsets[1] = idx + searchString.length();
        }
    }

    QString result;
    ScopedValue replacement(scope);
    ScopedValue replaceValue(scope, ctx->argument(1));
    Scoped<FunctionObject> searchCallback(scope, replaceValue);
    if (!!searchCallback) {
        result.reserve(string.length() + 10*numStringMatches);
        ScopedCallData callData(scope, numCaptures + 2);
        callData->thisObject = Value::undefinedValue();
        int lastEnd = 0;
        for (int i = 0; i < numStringMatches; ++i) {
            for (int k = 0; k < numCaptures; ++k) {
                int idx = (i * numCaptures + k) * 2;
                uint start = matchOffsets[idx];
                uint end = matchOffsets[idx + 1];
                Value entry = Value::undefinedValue();
                if (start != JSC::Yarr::offsetNoMatch && end != JSC::Yarr::offsetNoMatch)
                    entry = Value::fromString(ctx, string.mid(start, end - start));
                callData->args[k] = entry;
            }
            uint matchStart = matchOffsets[i * numCaptures * 2];
            Q_ASSERT(matchStart >= lastEnd);
            uint matchEnd = matchOffsets[i * numCaptures * 2 + 1];
            callData->args[numCaptures] = Value::fromUInt32(matchStart);
            callData->args[numCaptures + 1] = Value::fromString(ctx, string);

            replacement = searchCallback->call(callData);
            result += string.midRef(lastEnd, matchStart - lastEnd);
            result += replacement->toString(ctx)->toQString();
            lastEnd = matchEnd;
        }
        result += string.midRef(lastEnd);
    } else {
        QString newString = replaceValue->toString(ctx)->toQString();
        result.reserve(string.length() + numStringMatches*newString.size());

        int lastEnd = 0;
        for (int i = 0; i < numStringMatches; ++i) {
            int baseIndex = i * numCaptures * 2;
            uint matchStart = matchOffsets[baseIndex];
            uint matchEnd = matchOffsets[baseIndex + 1];
            if (matchStart == JSC::Yarr::offsetNoMatch)
                continue;

            result += string.midRef(lastEnd, matchStart - lastEnd);
            appendReplacementString(&result, string, newString, matchOffsets + baseIndex, numCaptures);
            lastEnd = matchEnd;
        }
        result += string.midRef(lastEnd);
    }

    if (matchOffsets != _matchOffsets)
        free(matchOffsets);

    return Value::fromString(ctx, result).asReturnedValue();
}

ReturnedValue StringPrototype::method_search(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    ScopedValue regExpValue(scope, ctx->argument(0));
    RegExpObject *regExp = regExpValue->as<RegExpObject>();
    if (!regExp) {
        ScopedCallData callData(scope, 1);
        callData->args[0] = regExpValue;
        regExpValue = ctx->engine->regExpCtor.asFunctionObject()->construct(callData);
        regExp = regExpValue->as<RegExpObject>();
    }
    uint* matchOffsets = (uint*)alloca(regExp->value->captureCount() * 2 * sizeof(uint));
    uint result = regExp->value->match(string, /*offset*/0, matchOffsets);
    if (result == JSC::Yarr::offsetNoMatch)
        return Encode(-1);
    return Encode(result);
}

ReturnedValue StringPrototype::method_slice(SimpleCallContext *ctx)
{
    const QString text = getThisString(ctx);
    const double length = text.length();

    double start = ctx->argumentCount ? ctx->arguments[0].toInteger() : 0;
    double end = (ctx->argumentCount < 2 || ctx->arguments[1].isUndefined())
            ? length : ctx->arguments[1].toInteger();

    if (start < 0)
        start = qMax(length + start, 0.);
    else
        start = qMin(start, length);

    if (end < 0)
        end = qMax(length + end, 0.);
    else
        end = qMin(end, length);

    const int intStart = int(start);
    const int intEnd = int(end);

    int count = qMax(0, intEnd - intStart);
    return Value::fromString(ctx, text.mid(intStart, count)).asReturnedValue();
}

ReturnedValue StringPrototype::method_split(SimpleCallContext *ctx)
{
    Scope scope(ctx);
    QString text;
    if (StringObject *thisObject = ctx->thisObject.asStringObject())
        text = thisObject->value.stringValue()->toQString();
    else
        text = ctx->thisObject.toString(ctx)->toQString();

    ScopedValue separatorValue(scope, ctx->argument(0));
    ScopedValue limitValue(scope, ctx->argument(1));

    Scoped<ArrayObject> array(scope, ctx->engine->newArrayObject());

    if (separatorValue->isUndefined()) {
        if (limitValue->isUndefined()) {
            array->push_back(Value::fromString(ctx, text));
            return array.asReturnedValue();
        }
        return Value::fromString(ctx, text.left(limitValue->toInteger())).asReturnedValue();
    }

    uint limit = limitValue->isUndefined() ? UINT_MAX : limitValue->toUInt32();

    if (limit == 0)
        return array.asReturnedValue();

    Scoped<RegExpObject> re(scope, separatorValue);
    if (!!re) {
        if (re->value->pattern().isEmpty()) {
            re = (RegExpObject *)0;
            separatorValue = Value::fromString(ctx, QString());
        }
    }

    if (!!re) {
        uint offset = 0;
        uint* matchOffsets = (uint*)alloca(re->value->captureCount() * 2 * sizeof(uint));
        while (true) {
            uint result = re->value->match(text, offset, matchOffsets);
            if (result == JSC::Yarr::offsetNoMatch)
                break;

            array->push_back(Value::fromString(ctx, text.mid(offset, matchOffsets[0] - offset)));
            offset = qMax(offset + 1, matchOffsets[1]);

            if (array->arrayLength() >= limit)
                break;

            for (int i = 1; i < re->value->captureCount(); ++i) {
                uint start = matchOffsets[i * 2];
                uint end = matchOffsets[i * 2 + 1];
                array->push_back(Value::fromString(ctx, text.mid(start, end - start)));
                if (array->arrayLength() >= limit)
                    break;
            }
        }
        if (array->arrayLength() < limit)
            array->push_back(Value::fromString(ctx, text.mid(offset)));
    } else {
        QString separator = separatorValue->toString(ctx)->toQString();
        if (separator.isEmpty()) {
            for (uint i = 0; i < qMin(limit, uint(text.length())); ++i)
                array->push_back(Value::fromString(ctx, text.mid(i, 1)));
            return array.asReturnedValue();
        }

        int start = 0;
        int end;
        while ((end = text.indexOf(separator, start)) != -1) {
            array->push_back(Value::fromString(ctx, text.mid(start, end - start)));
            start = end + separator.size();
            if (array->arrayLength() >= limit)
                break;
        }
        if (array->arrayLength() < limit && start != -1)
            array->push_back(Value::fromString(ctx, text.mid(start)));
    }
    return array.asReturnedValue();
}

ReturnedValue StringPrototype::method_substr(SimpleCallContext *context)
{
    const QString value = getThisString(context, context->thisObject);

    double start = 0;
    if (context->argumentCount > 0)
        start = context->arguments[0].toInteger();

    double length = +qInf();
    if (context->argumentCount > 1)
        length = context->arguments[1].toInteger();

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(length);
    return Value::fromString(context, value.mid(x, y)).asReturnedValue();
}

ReturnedValue StringPrototype::method_substring(SimpleCallContext *context)
{
    QString value = getThisString(context, context->thisObject);
    int length = value.length();

    double start = 0;
    double end = length;

    if (context->argumentCount > 0)
        start = context->arguments[0].toInteger();

    Value endValue = context->argumentCount > 1 ? context->arguments[1] : Value::undefinedValue();
    if (!endValue.isUndefined())
        end = endValue.toInteger();

    if (std::isnan(start) || start < 0)
        start = 0;

    if (std::isnan(end) || end < 0)
        end = 0;

    if (start > length)
        start = length;

    if (end > length)
        end = length;

    if (start > end) {
        double was = start;
        start = end;
        end = was;
    }

    qint32 x = (int)start;
    qint32 y = (int)(end - start);
    return Value::fromString(context, value.mid(x, y)).asReturnedValue();
}

ReturnedValue StringPrototype::method_toLowerCase(SimpleCallContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toLower()).asReturnedValue();
}

ReturnedValue StringPrototype::method_toLocaleLowerCase(SimpleCallContext *ctx)
{
    return method_toLowerCase(ctx);
}

ReturnedValue StringPrototype::method_toUpperCase(SimpleCallContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toUpper()).asReturnedValue();
}

ReturnedValue StringPrototype::method_toLocaleUpperCase(SimpleCallContext *ctx)
{
    return method_toUpperCase(ctx);
}

ReturnedValue StringPrototype::method_fromCharCode(SimpleCallContext *context)
{
    QString str(context->argumentCount, Qt::Uninitialized);
    QChar *ch = str.data();
    for (int i = 0; i < context->argumentCount; ++i) {
        *ch = QChar(context->arguments[i].toUInt16());
        ++ch;
    }
    return Value::fromString(context, str).asReturnedValue();
}

ReturnedValue StringPrototype::method_trim(SimpleCallContext *ctx)
{
    if (ctx->thisObject.isNull() || ctx->thisObject.isUndefined())
        ctx->throwTypeError();

    QString s = ctx->thisObject.toQString();
    const QChar *chars = s.constData();
    int start, end;
    for (start = 0; start < s.length(); ++start) {
        if (!chars[start].isSpace() && chars[start].unicode() != 0xfeff)
            break;
    }
    for (end = s.length() - 1; end >= start; --end) {
        if (!chars[end].isSpace() && chars[end].unicode() != 0xfeff)
            break;
    }

    return Value::fromString(ctx, QString(chars + start, end - start + 1)).asReturnedValue();
}
