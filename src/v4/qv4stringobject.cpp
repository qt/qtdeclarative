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


#include "qv4stringobject.h"
#include "qv4regexpobject.h"
#include "qv4objectproto.h"
#include "qv4mm.h"
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
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include <qv4isel_masm_p.h>

#ifndef Q_WS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QQmlJS::VM;

StringObject::StringObject(ExecutionContext *ctx, const Value &value)
    : value(value)
{
    type = Type_StringObject;

    tmpProperty.type = PropertyDescriptor::Data;
    tmpProperty.enumberable = PropertyDescriptor::Enabled;
    tmpProperty.writable = PropertyDescriptor::Disabled;
    tmpProperty.configurable = PropertyDescriptor::Disabled;
    tmpProperty.value = Value::undefinedValue();

    assert(value.isString());
    defineReadonlyProperty(ctx->engine->id_length, Value::fromUInt32(value.stringValue()->toQString().length()));
}

PropertyDescriptor *StringObject::getIndex(ExecutionContext *ctx, uint index)
{
    QString str = value.stringValue()->toQString();
    if (index >= (uint)str.length())
        return 0;
    String *result = ctx->engine->newString(str.mid(index, 1));
    tmpProperty.value = Value::fromString(result);
    return &tmpProperty;
}

void StringObject::markObjects()
{
    value.stringValue()->mark();
    Object::markObjects();
}


StringCtor::StringCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value StringCtor::construct(ExecutionContext *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::fromString(ctx->argument(0).toString(ctx));
    else
        value = Value::fromString(ctx, QString());
    return Value::fromObject(ctx->engine->newStringObject(ctx, value));
}

Value StringCtor::call(ExecutionContext *ctx)
{
    Value value;
    if (ctx->argumentCount)
        value = Value::fromString(ctx->argument(0).toString(ctx));
    else
        value = Value::fromString(ctx, QString());
    return value;
}

void StringPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineDefaultProperty(ctx, QStringLiteral("fromCharCode"), method_fromCharCode, 1);

    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_toString); // valueOf and toString are identical
    defineDefaultProperty(ctx, QStringLiteral("charAt"), method_charAt, 1);
    defineDefaultProperty(ctx, QStringLiteral("charCodeAt"), method_charCodeAt, 1);
    defineDefaultProperty(ctx, QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(ctx, QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(ctx, QStringLiteral("localeCompare"), method_localeCompare, 1);
    defineDefaultProperty(ctx, QStringLiteral("match"), method_match, 1);
    defineDefaultProperty(ctx, QStringLiteral("replace"), method_replace, 2);
    defineDefaultProperty(ctx, QStringLiteral("search"), method_search, 1);
    defineDefaultProperty(ctx, QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(ctx, QStringLiteral("split"), method_split, 2);
    defineDefaultProperty(ctx, QStringLiteral("substr"), method_substr, 2);
    defineDefaultProperty(ctx, QStringLiteral("substring"), method_substring, 2);
    defineDefaultProperty(ctx, QStringLiteral("toLowerCase"), method_toLowerCase);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    defineDefaultProperty(ctx, QStringLiteral("toUpperCase"), method_toUpperCase);
    defineDefaultProperty(ctx, QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
    defineDefaultProperty(ctx, QStringLiteral("trim"), method_trim);
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

static QString getThisString(ExecutionContext *parentCtx, Value thisObject)
{
    if (thisObject.isString())
        return thisObject.stringValue()->toQString();

    String* str = 0;
    if (StringObject *thisString = thisObject.asStringObject())
        str = thisString->value.stringValue();
    else if (thisObject.isUndefined() || thisObject.isNull())
        parentCtx->throwTypeError();
    else
        str = thisObject.toString(parentCtx);
    return str->toQString();
}

Value StringPrototype::method_toString(ExecutionContext *parentCtx, Value thisObject, Value *, int)
{
    if (thisObject.isString())
        return thisObject;

    StringObject *o = thisObject.asStringObject();
    if (!o)
        parentCtx->throwTypeError();
    return o->value;
}

Value StringPrototype::method_charAt(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    const QString str = getThisString(parentCtx, thisObject);

    int pos = 0;
    if (argc > 0)
        pos = (int) argv[0].toInteger(parentCtx);

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return Value::fromString(parentCtx, result);
}

Value StringPrototype::method_charCodeAt(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    const QString str = getThisString(parentCtx, thisObject);

    int pos = 0;
    if (argc > 0)
        pos = (int) argv[0].toInteger(parentCtx);


    if (pos >= 0 && pos < str.length())
        return Value::fromInt32(str.at(pos).unicode());

    return Value::fromDouble(qSNaN());
}

Value StringPrototype::method_concat(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    QString value = getThisString(parentCtx, thisObject);

    for (int i = 0; i < argc; ++i) {
        Value v = __qmljs_to_string(argv[i], parentCtx);
        assert(v.isString());
        value += v.stringValue()->toQString();
    }

    return Value::fromString(parentCtx, value);
}

Value StringPrototype::method_indexOf(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    QString value = getThisString(parentCtx, thisObject);

    QString searchString;
    if (argc)
        searchString = argv[0].toString(parentCtx)->toQString();

    int pos = 0;
    if (argc > 1)
        pos = (int) argv[1].toInteger(parentCtx);

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    return Value::fromDouble(index);
}

Value StringPrototype::method_lastIndexOf(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    const QString value = getThisString(parentCtx, thisObject);

    QString searchString;
    if (argc) {
        Value v = __qmljs_to_string(argv[0], parentCtx);
        searchString = v.stringValue()->toQString();
    }

    Value posArg = argc > 1 ? argv[1] : Value::undefinedValue();
    double position = __qmljs_to_number(posArg, parentCtx);
    if (std::isnan(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    int index = value.lastIndexOf(searchString, pos);
    return Value::fromDouble(index);
}

Value StringPrototype::method_localeCompare(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    const QString value = getThisString(parentCtx, thisObject);
    const QString that = (argc ? argv[0] : Value::undefinedValue()).toString(parentCtx)->toQString();
    return Value::fromDouble(QString::localeAwareCompare(value, that));
}

Value StringPrototype::method_match(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    if (thisObject.isUndefined() || thisObject.isNull())
        __qmljs_throw_type_error(parentCtx);

    String *s = thisObject.toString(parentCtx);

    Value regexp = argc ? argv[0] : Value::undefinedValue();
    RegExpObject *rx = regexp.asRegExpObject();
    if (!rx)
        rx = parentCtx->engine->regExpCtor.asFunctionObject()->construct(parentCtx, &regexp, 1).asRegExpObject();

    if (!rx)
        // ### CHECK
        __qmljs_throw_type_error(parentCtx);

    bool global = rx->global;

    // ### use the standard builtin function, not the one that might be redefined in the proto
    FunctionObject *exec = parentCtx->engine->regExpPrototype->__get__(parentCtx, parentCtx->engine->newString(QStringLiteral("exec")), 0).asFunctionObject();

    Value arg = Value::fromString(s);
    if (!global)
        return exec->call(parentCtx, Value::fromObject(rx), &arg, 1);

    String *lastIndex = parentCtx->engine->newString(QStringLiteral("lastIndex"));
    rx->__put__(parentCtx, lastIndex, Value::fromInt32(0));
    ArrayObject *a = parentCtx->engine->newArrayObject(parentCtx);

    double previousLastIndex = 0;
    uint n = 0;
    while (1) {
        Value result = exec->call(parentCtx, Value::fromObject(rx), &arg, 1);
        if (result.isNull())
            break;
        assert(result.isObject());
        double thisIndex = rx->__get__(parentCtx, lastIndex, 0).toInteger(parentCtx);
        if (previousLastIndex == thisIndex) {
            previousLastIndex = thisIndex + 1;
            rx->__put__(parentCtx, lastIndex, Value::fromDouble(previousLastIndex));
        } else {
            previousLastIndex = thisIndex;
        }
        Value matchStr = result.objectValue()->__get__(parentCtx, (uint)0, (bool *)0);
        a->array.set(n, matchStr);
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

Value StringPrototype::method_replace(ExecutionContext *ctx)
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
    RegExpObject *regExp = searchValue.asRegExpObject();
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
            regExp->lastIndexProperty->value = Value::fromUInt32(0);
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
        int argc = numCaptures + 2;
        Value *args = (Value*)alloca((numCaptures + 2) * sizeof(Value));
        for (int i = 0; i < numStringMatches; ++i) {
            for (int k = 0; k < numCaptures; ++k) {
                int idx = (i * numCaptures + k) * 2;
                uint start = matchOffsets[idx];
                uint end = matchOffsets[idx + 1];
                Value entry = Value::undefinedValue();
                if (start != JSC::Yarr::offsetNoMatch && end != JSC::Yarr::offsetNoMatch)
                    entry = Value::fromString(ctx, string.mid(start, end - start));
                args[k] = entry;
            }
            uint matchStart = matchOffsets[i * numCaptures * 2];
            uint matchEnd = matchOffsets[i * numCaptures * 2 + 1];
            args[numCaptures] = Value::fromUInt32(matchStart);
            args[numCaptures + 1] = Value::fromString(ctx, string);
            Value replacement = searchCallback->call(ctx, Value::undefinedValue(), args, argc);
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

Value StringPrototype::method_search(ExecutionContext *ctx)
{
    QString string;
    if (StringObject *thisString = ctx->thisObject.asStringObject())
        string = thisString->value.stringValue()->toQString();
    else
        string = ctx->thisObject.toString(ctx)->toQString();

    Value regExpValue = ctx->argument(0);
    RegExpObject *regExp = regExpValue.asRegExpObject();
    if (!regExp) {
        regExpValue = ctx->engine->regExpCtor.asFunctionObject()->construct(ctx, &regExpValue, 1);
        regExp = regExpValue.asRegExpObject();
    }
    uint* matchOffsets = (uint*)alloca(regExp->value->captureCount() * 2 * sizeof(uint));
    uint result = regExp->value->match(string, /*offset*/0, matchOffsets);
    if (result == JSC::Yarr::offsetNoMatch)
        return Value::fromInt32(-1);
    return Value::fromUInt32(result);
}

Value StringPrototype::method_slice(ExecutionContext *ctx)
{
    const QString text = getThisString(ctx);
    const double length = text.length();

    double start = ctx->argument(0).toInteger(ctx);
    double end = ctx->argument(1).isUndefined()
            ? length : ctx->argument(1).toInteger(ctx);

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

Value StringPrototype::method_split(ExecutionContext *ctx)
{
    QString text;
    if (StringObject *thisObject = ctx->thisObject.asStringObject())
        text = thisObject->value.stringValue()->toQString();
    else
        text = ctx->thisObject.toString(ctx)->toQString();

    Value separatorValue = ctx->argumentCount > 0 ? ctx->argument(0) : Value::undefinedValue();
    Value limitValue = ctx->argumentCount > 1 ? ctx->argument(1) : Value::undefinedValue();

    ArrayObject* array = ctx->engine->newArrayObject(ctx);
    Value result = Value::fromObject(array);

    if (separatorValue.isUndefined()) {
        if (limitValue.isUndefined()) {
            array->array.push_back(Value::fromString(ctx, text));
            return result;
        }
        return Value::fromString(ctx, text.left(limitValue.toInteger(ctx)));
    }

    uint limit = limitValue.isUndefined() ? UINT_MAX : limitValue.toUInt32(ctx);

    if (limit == 0)
        return result;

    if (RegExpObject* re = separatorValue.asRegExpObject()) {
        if (re->value->pattern().isEmpty()) {
            re = 0;
            separatorValue = Value::fromString(ctx, QString());
        }
    }

    if (RegExpObject* re = separatorValue.asRegExpObject()) {
        uint offset = 0;
        uint* matchOffsets = (uint*)alloca(re->value->captureCount() * 2 * sizeof(uint));
        while (true) {
            uint result = re->value->match(text, offset, matchOffsets);
            if (result == JSC::Yarr::offsetNoMatch)
                break;

            array->array.push_back(Value::fromString(ctx, text.mid(offset, matchOffsets[0] - offset)));
            offset = qMax(offset + 1, matchOffsets[1]);

            if (array->array.length() >= limit)
                break;

            for (int i = 1; i < re->value->captureCount(); ++i) {
                uint start = matchOffsets[i * 2];
                uint end = matchOffsets[i * 2 + 1];
                array->array.push_back(Value::fromString(ctx, text.mid(start, end - start)));
                if (array->array.length() >= limit)
                    break;
            }
        }
        if (array->array.length() < limit)
            array->array.push_back(Value::fromString(ctx, text.mid(offset)));
    } else {
        QString separator = separatorValue.toString(ctx)->toQString();
        if (separator.isEmpty()) {
            for (uint i = 0; i < qMin(limit, uint(text.length())); ++i)
                array->array.push_back(Value::fromString(ctx, text.mid(i, 1)));
            return result;
        }

        int start = 0;
        int end;
        while ((end = text.indexOf(separator, start)) != -1) {
            array->array.push_back(Value::fromString(ctx, text.mid(start, end - start)));
            start = end + separator.size();
            if (array->array.length() >= limit)
                break;
        }
        if (array->array.length() < limit && start != -1)
            array->array.push_back(Value::fromString(ctx, text.mid(start)));
    }
    return result;
}

Value StringPrototype::method_substr(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    const QString value = getThisString(parentCtx, thisObject);

    double start = 0;
    if (argc > 0)
        start = argv[0].toInteger(parentCtx);

    double length = +qInf();
    if (argc > 1)
        length = argv[1].toInteger(parentCtx);

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Value::toInt32(start);
    qint32 y = Value::toInt32(length);
    return Value::fromString(parentCtx, value.mid(x, y));
}

Value StringPrototype::method_substring(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc)
{
    QString value = getThisString(parentCtx, thisObject);
    int length = value.length();

    double start = 0;
    double end = length;

    if (argc > 0)
        start = argv[0].toInteger(parentCtx);

    Value endValue = argc > 1 ? argv[1] : Value::undefinedValue();
    if (!endValue.isUndefined())
        end = endValue.toInteger(parentCtx);

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
    return Value::fromString(parentCtx, value.mid(x, y));
}

Value StringPrototype::method_toLowerCase(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toLower());
}

Value StringPrototype::method_toLocaleLowerCase(ExecutionContext *ctx)
{
    return method_toLowerCase(ctx);
}

Value StringPrototype::method_toUpperCase(ExecutionContext *ctx)
{
    QString value = getThisString(ctx);
    return Value::fromString(ctx, value.toUpper());
}

Value StringPrototype::method_toLocaleUpperCase(ExecutionContext *ctx)
{
    return method_toUpperCase(ctx);
}

Value StringPrototype::method_fromCharCode(ExecutionContext *parentCtx, Value, Value *argv, int argc)
{
    QString str(argc, Qt::Uninitialized);
    QChar *ch = str.data();
    for (int i = 0; i < argc; ++i) {
        *ch = QChar(argv[i].toUInt16(parentCtx));
        ++ch;
    }
    return Value::fromString(parentCtx, str);
}

Value StringPrototype::method_trim(ExecutionContext *ctx)
{
    if (ctx->thisObject.isNull() || ctx->thisObject.isUndefined())
        __qmljs_throw_type_error(ctx);

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
