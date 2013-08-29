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

Value StringCtor::construct(Managed *m, const CallData &d)
{
    Value value;
    if (d.argc)
        value = Value::fromString(d.args[0].toString(m->engine()->current));
    else
        value = Value::fromString(m->engine()->current, QString());
    return Value::fromObject(m->engine()->newStringObject(value));
}

Value StringCtor::call(Managed *m, const CallData &d)
{
    Value value;
    if (d.argc)
        value = Value::fromString(d.args[0].toString(m->engine()->current));
    else
        value = Value::fromString(m->engine()->current, QString());
    return value;
}

void StringPrototype::init(ExecutionEngine *engine, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(engine, QStringLiteral("fromCharCode"), method_fromCharCode, 1);

    defineDefaultProperty(engine, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(engine, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(engine, QStringLiteral("valueOf"), method_toString); // valueOf and toString are identical
    defineDefaultProperty(engine, QStringLiteral("charAt"), method_charAt, 1);
    defineDefaultProperty(engine, QStringLiteral("charCodeAt"), method_charCodeAt, 1);
    defineDefaultProperty(engine, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(engine, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(engine, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(engine, QStringLiteral("localeCompare"), method_localeCompare, 1);
    defineDefaultProperty(engine, QStringLiteral("match"), method_match, 1);
    defineDefaultProperty(engine, QStringLiteral("replace"), method_replace, 2);
    defineDefaultProperty(engine, QStringLiteral("search"), method_search, 1);
    defineDefaultProperty(engine, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(engine, QStringLiteral("split"), method_split, 2);
    defineDefaultProperty(engine, QStringLiteral("substr"), method_substr, 2);
    defineDefaultProperty(engine, QStringLiteral("substring"), method_substring, 2);
    defineDefaultProperty(engine, QStringLiteral("toLowerCase"), method_toLowerCase);
    defineDefaultProperty(engine, QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    defineDefaultProperty(engine, QStringLiteral("toUpperCase"), method_toUpperCase);
    defineDefaultProperty(engine, QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
    defineDefaultProperty(engine, QStringLiteral("trim"), method_trim);
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

Value StringPrototype::method_toString(SimpleCallContext *context)
{
    if (context->thisObject.isString())
        return context->thisObject;

    StringObject *o = context->thisObject.asStringObject();
    if (!o)
        context->throwTypeError();
    return o->value;
}

Value StringPrototype::method_charAt(SimpleCallContext *context)
{
    const QString str = getThisString(context, context->thisObject);

    int pos = 0;
    if (context->argumentCount > 0)
        pos = (int) context->arguments[0].toInteger();

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return Value::fromString(context, result);
}

Value StringPrototype::method_charCodeAt(SimpleCallContext *context)
{
    const QString str = getThisString(context, context->thisObject);

    int pos = 0;
    if (context->argumentCount > 0)
        pos = (int) context->arguments[0].toInteger();


    if (pos >= 0 && pos < str.length())
        return Value::fromInt32(str.at(pos).unicode());

    return Value::fromDouble(qSNaN());
}

Value StringPrototype::method_concat(SimpleCallContext *context)
{
    QString value = getThisString(context, context->thisObject);

    for (int i = 0; i < context->argumentCount; ++i) {
        Value v = __qmljs_to_string(context->arguments[i], context);
        assert(v.isString());
        value += v.stringValue()->toQString();
    }

    return Value::fromString(context, value);
}

Value StringPrototype::method_indexOf(SimpleCallContext *context)
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

    return Value::fromDouble(index);
}

Value StringPrototype::method_lastIndexOf(SimpleCallContext *context)
{
    const QString value = getThisString(context, context->thisObject);

    QString searchString;
    if (context->argumentCount) {
        Value v = __qmljs_to_string(context->arguments[0], context);
        searchString = v.stringValue()->toQString();
    }

    Value posArg = context->argumentCount > 1 ? context->arguments[1] : Value::undefinedValue();
    double position = __qmljs_to_number(posArg);
    if (std::isnan(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    if (searchString.isNull() && pos == 0)
        return Value::fromDouble(-1);
    int index = value.lastIndexOf(searchString, pos);
    return Value::fromDouble(index);
}

Value StringPrototype::method_localeCompare(SimpleCallContext *context)
{
    const QString value = getThisString(context, context->thisObject);
    const QString that = (context->argumentCount ? context->arguments[0] : Value::undefinedValue()).toString(context)->toQString();
    return Value::fromDouble(QString::localeAwareCompare(value, that));
}

Value StringPrototype::method_match(SimpleCallContext *context)
{
    if (context->thisObject.isUndefined() || context->thisObject.isNull())
        context->throwTypeError();

    String *s = context->thisObject.toString(context);

    Value regexp = context->argumentCount ? context->arguments[0] : Value::undefinedValue();
    RegExpObject *rx = regexp.as<RegExpObject>();
    if (!rx) {
        CALLDATA(1);
        d.args[0] = regexp;
        rx = context->engine->regExpCtor.asFunctionObject()->construct(d).as<RegExpObject>();
    }

    if (!rx)
        // ### CHECK
        context->throwTypeError();

    bool global = rx->global;

    // ### use the standard builtin function, not the one that might be redefined in the proto
    FunctionObject *exec = context->engine->regExpClass->prototype->get(context->engine->newString(QStringLiteral("exec")), 0).asFunctionObject();

    CALLDATA(1);
    d.thisObject = Value::fromObject(rx);
    d.args[0] = Value::fromString(s);
    if (!global)
        return exec->call(d);

    String *lastIndex = context->engine->newString(QStringLiteral("lastIndex"));
    rx->put(lastIndex, Value::fromInt32(0));
    ArrayObject *a = context->engine->newArrayObject();

    double previousLastIndex = 0;
    uint n = 0;
    while (1) {
        Value result = exec->call(d);
        if (result.isNull())
            break;
        assert(result.isObject());
        double thisIndex = rx->get(lastIndex, 0).toInteger();
        if (previousLastIndex == thisIndex) {
            previousLastIndex = thisIndex + 1;
            rx->put(lastIndex, Value::fromDouble(previousLastIndex));
        } else {
            previousLastIndex = thisIndex;
        }
        Value matchStr = result.objectValue()->getIndexed(0);
        a->arraySet(n, matchStr);
        ++n;
    }
    if (!n)
        return Value::nullValue();

    return Value::fromObject(a);

}

static QString makeReplacementString(const QString &input, const QString& replaceValue, uint* matchOffsets, int captureCount)
{
    QString result;
    result.reserve(replaceValue.length());
    for (int i = 0; i < replaceValue.length(); ++i) {
        if (replaceValue.at(i) == QLatin1Char('$') && i < replaceValue.length() - 1) {
            char ch = replaceValue.at(++i).toLatin1();
            uint substStart = JSC::Yarr::offsetNoMatch;
            uint substEnd = JSC::Yarr::offsetNoMatch;
            if (ch == '$') {
                result += ch;
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
                char capture = ch - '0';
                if (capture > 0 && capture < captureCount) {
                    substStart = matchOffsets[capture * 2];
                    substEnd = matchOffsets[capture * 2 + 1];
                }
            } else if (ch == '0' && i < replaceValue.length() - 1) {
                int capture = (ch - '0') * 10;
                ch = replaceValue.at(++i).toLatin1();
                capture += ch - '0';
                if (capture > 0 && capture < captureCount) {
                    substStart = matchOffsets[capture * 2];
                    substEnd = matchOffsets[capture * 2 + 1];
                }
            }
            if (substStart != JSC::Yarr::offsetNoMatch && substEnd != JSC::Yarr::offsetNoMatch)
                result += input.midRef(substStart, substEnd - substStart);
        } else {
            result += replaceValue.at(i);
        }
    }
    return result;
}

Value StringPrototype::method_replace(SimpleCallContext *ctx)
{
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    int numCaptures = 0;
    QVarLengthArray<uint, 16> matchOffsets;
    int numStringMatches = 0;

    Value searchValue = ctx->argument(0);
    RegExpObject *regExp = searchValue.as<RegExpObject>();
    if (regExp) {
        uint offset = 0;
        while (true) {
            int oldSize = matchOffsets.size();
            matchOffsets.resize(matchOffsets.size() + regExp->value->captureCount() * 2);
            if (regExp->value->match(string, offset, matchOffsets.data() + oldSize) == JSC::Yarr::offsetNoMatch) {
                matchOffsets.resize(oldSize);
                break;
            }
            if (!regExp->global)
                break;
            offset = qMax(offset + 1, matchOffsets[oldSize + 1]);
        }
        if (regExp->global)
            regExp->lastIndexProperty(ctx)->value = Value::fromUInt32(0);
        numStringMatches = matchOffsets.size() / (regExp->value->captureCount() * 2);
        numCaptures = regExp->value->captureCount();
    } else {
        numCaptures = 1;
        QString searchString = searchValue.toString(ctx)->toQString();
        int idx = string.indexOf(searchString);
        if (idx != -1) {
            numStringMatches = 1;
            matchOffsets.resize(2);
            matchOffsets[0] = idx;
            matchOffsets[1] = idx + searchString.length();
        }
    }

    QString result = string;
    Value replaceValue = ctx->argument(1);
    if (FunctionObject* searchCallback = replaceValue.asFunctionObject()) {
        int replacementDelta = 0;
        CALLDATA(numCaptures + 2);
        d.thisObject = Value::undefinedValue();
        for (int i = 0; i < numStringMatches; ++i) {
            for (int k = 0; k < numCaptures; ++k) {
                int idx = (i * numCaptures + k) * 2;
                uint start = matchOffsets[idx];
                uint end = matchOffsets[idx + 1];
                Value entry = Value::undefinedValue();
                if (start != JSC::Yarr::offsetNoMatch && end != JSC::Yarr::offsetNoMatch)
                    entry = Value::fromString(ctx, string.mid(start, end - start));
                d.args[k] = entry;
            }
            uint matchStart = matchOffsets[i * numCaptures * 2];
            uint matchEnd = matchOffsets[i * numCaptures * 2 + 1];
            d.args[numCaptures] = Value::fromUInt32(matchStart);
            d.args[numCaptures + 1] = Value::fromString(ctx, string);

            Value replacement = searchCallback->call(d);
            QString replacementString = replacement.toString(ctx)->toQString();
            result.replace(replacementDelta + matchStart, matchEnd - matchStart, replacementString);
            replacementDelta += replacementString.length() - matchEnd + matchStart;
        }
    } else {
        QString newString = replaceValue.toString(ctx)->toQString();
        int replacementDelta = 0;

        for (int i = 0; i < numStringMatches; ++i) {
            int baseIndex = i * numCaptures * 2;
            uint matchStart = matchOffsets[baseIndex];
            uint matchEnd = matchOffsets[baseIndex + 1];
            if (matchStart == JSC::Yarr::offsetNoMatch)
                continue;

            QString replacement = makeReplacementString(string, newString, matchOffsets.data() + baseIndex, numCaptures);
            result.replace(replacementDelta + matchStart, matchEnd - matchStart, replacement);
            replacementDelta += replacement.length() - matchEnd + matchStart;
        }
    }

    return Value::fromString(ctx, result);
}

Value StringPrototype::method_search(SimpleCallContext *ctx)
{
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    Value regExpValue = ctx->argument(0);
    RegExpObject *regExp = regExpValue.as<RegExpObject>();
    if (!regExp) {
        CALLDATA(1);
        d.args[0] = regExpValue;
        regExpValue = ctx->engine->regExpCtor.asFunctionObject()->construct(d);
        regExp = regExpValue.as<RegExpObject>();
    }
    uint* matchOffsets = (uint*)alloca(regExp->value->captureCount() * 2 * sizeof(uint));
    uint result = regExp->value->match(string, /*offset*/0, matchOffsets);
    if (result == JSC::Yarr::offsetNoMatch)
        return Value::fromInt32(-1);
    return Value::fromUInt32(result);
}

Value StringPrototype::method_slice(SimpleCallContext *ctx)
{
    const QString text = getThisString(ctx);
    const double length = text.length();

    double start = ctx->argument(0).toInteger();
    double end = ctx->argument(1).isUndefined()
            ? length : ctx->argument(1).toInteger();

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
    return Value::fromString(ctx, text.mid(intStart, count));
}

Value StringPrototype::method_split(SimpleCallContext *ctx)
{
    QString text;
    if (StringObject *thisObject = ctx->thisObject.asStringObject())
        text = thisObject->value.stringValue()->toQString();
    else
        text = ctx->thisObject.toString(ctx)->toQString();

    Value separatorValue = ctx->argumentCount > 0 ? ctx->argument(0) : Value::undefinedValue();
    Value limitValue = ctx->argumentCount > 1 ? ctx->argument(1) : Value::undefinedValue();

    ArrayObject* array = ctx->engine->newArrayObject();
    Value result = Value::fromObject(array);

    if (separatorValue.isUndefined()) {
        if (limitValue.isUndefined()) {
            array->push_back(Value::fromString(ctx, text));
            return result;
        }
        return Value::fromString(ctx, text.left(limitValue.toInteger()));
    }

    uint limit = limitValue.isUndefined() ? UINT_MAX : limitValue.toUInt32();

    if (limit == 0)
        return result;

    if (RegExpObject* re = separatorValue.as<RegExpObject>()) {
        if (re->value->pattern().isEmpty()) {
            re = 0;
            separatorValue = Value::fromString(ctx, QString());
        }
    }

    if (RegExpObject* re = separatorValue.as<RegExpObject>()) {
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
        QString separator = separatorValue.toString(ctx)->toQString();
        if (separator.isEmpty()) {
            for (uint i = 0; i < qMin(limit, uint(text.length())); ++i)
                array->push_back(Value::fromString(ctx, text.mid(i, 1)));
            return result;
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
    return result;
}

Value StringPrototype::method_substr(SimpleCallContext *context)
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
    return Value::fromString(context, value.mid(x, y));
}

Value StringPrototype::method_substring(SimpleCallContext *context)
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
    return Value::fromString(context, value.mid(x, y));
}

Value StringPrototype::method_toLowerCase(SimpleCallContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toLower());
}

Value StringPrototype::method_toLocaleLowerCase(SimpleCallContext *ctx)
{
    return method_toLowerCase(ctx);
}

Value StringPrototype::method_toUpperCase(SimpleCallContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toUpper());
}

Value StringPrototype::method_toLocaleUpperCase(SimpleCallContext *ctx)
{
    return method_toUpperCase(ctx);
}

Value StringPrototype::method_fromCharCode(SimpleCallContext *context)
{
    QString str(context->argumentCount, Qt::Uninitialized);
    QChar *ch = str.data();
    for (int i = 0; i < context->argumentCount; ++i) {
        *ch = QChar(context->arguments[i].toUInt16());
        ++ch;
    }
    return Value::fromString(context, str);
}

Value StringPrototype::method_trim(SimpleCallContext *ctx)
{
    if (ctx->thisObject.isNull() || ctx->thisObject.isUndefined())
        ctx->throwTypeError();

    QString s = __qmljs_to_string(ctx->thisObject, ctx).stringValue()->toQString();
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

    return Value::fromString(ctx, QString(chars + start, end - start + 1));
}
