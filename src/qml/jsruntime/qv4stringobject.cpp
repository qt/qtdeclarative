/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qv4stringobject_p.h"
#include "qv4regexp_p.h"
#include "qv4regexpobject_p.h"
#include "qv4objectproto_p.h"
#include <private/qv4mm_p.h>
#include "qv4scopedvalue_p.h"
#include "qv4alloca_p.h"
#include "qv4jscall_p.h"
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include <cassert>

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

DEFINE_OBJECT_VTABLE(StringObject);

void Heap::StringObject::init()
{
    Object::init();
    Q_ASSERT(vtable() == QV4::StringObject::staticVTable());
    string.set(internalClass->engine, internalClass->engine->id_empty()->d());
    setProperty(internalClass->engine, LengthPropertyIndex, Primitive::fromInt32(0));
}

void Heap::StringObject::init(const QV4::String *str)
{
    Object::init();
    string.set(internalClass->engine, str->d());
    setProperty(internalClass->engine, LengthPropertyIndex, Primitive::fromInt32(length()));
}

Heap::String *Heap::StringObject::getIndex(uint index) const
{
    QString str = string->toQString();
    if (index >= (uint)str.length())
        return nullptr;
    return internalClass->engine->newString(str.mid(index, 1));
}

uint Heap::StringObject::length() const
{
    return string->length();
}

bool StringObject::deleteIndexedProperty(Managed *m, uint index)
{
    ExecutionEngine *v4 = static_cast<StringObject *>(m)->engine();
    Scope scope(v4);
    Scoped<StringObject> o(scope, m->as<StringObject>());
    Q_ASSERT(!!o);

    if (index < static_cast<uint>(o->d()->string->toQString().length()))
        return false;
    return true;
}

void StringObject::advanceIterator(Managed *m, ObjectIterator *it, Value *name, uint *index, Property *p, PropertyAttributes *attrs)
{
    name->setM(nullptr);
    StringObject *s = static_cast<StringObject *>(m);
    uint slen = s->d()->string->toQString().length();
    if (it->arrayIndex <= slen) {
        while (it->arrayIndex < slen) {
            *index = it->arrayIndex;
            ++it->arrayIndex;
            PropertyAttributes a;
            Property pd;
            s->getOwnProperty(*index, &a, &pd);
            if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
                *attrs = a;
                p->copy(&pd, a);
                return;
            }
        }
        if (s->arrayData()) {
            it->arrayNode = s->sparseBegin();
            // iterate until we're past the end of the string
            while (it->arrayNode && it->arrayNode->key() < slen)
                it->arrayNode = it->arrayNode->nextNode();
        }
    }

    return Object::advanceIterator(m, it, name, index, p, attrs);
}

DEFINE_OBJECT_VTABLE(StringCtor);

void Heap::StringCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("String"));
}

ReturnedValue StringCtor::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
{
    ExecutionEngine *v4 = static_cast<const Object *>(f)->engine();
    Scope scope(v4);
    ScopedString value(scope);
    if (argc)
        value = argv[0].toString(v4);
    else
        value = v4->newString();
    return Encode(v4->newStringObject(value));
}

ReturnedValue StringCtor::call(const FunctionObject *m, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *v4 = m->engine();
    if (argc)
        return argv[0].toString(v4)->asReturnedValue();
    else
        return v4->newString()->asReturnedValue();
}

void StringPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);

    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    ctor->defineReadonlyProperty(engine->id_length(), Primitive::fromInt32(1));
    ctor->defineDefaultProperty(QStringLiteral("fromCharCode"), method_fromCharCode, 1);

    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString(), method_toString);
    defineDefaultProperty(engine->id_valueOf(), method_toString); // valueOf and toString are identical
    defineDefaultProperty(QStringLiteral("charAt"), method_charAt, 1);
    defineDefaultProperty(QStringLiteral("charCodeAt"), method_charCodeAt, 1);
    defineDefaultProperty(QStringLiteral("concat"), method_concat, 1);
    defineDefaultProperty(QStringLiteral("endsWith"), method_endsWith, 1);
    defineDefaultProperty(QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(QStringLiteral("includes"), method_includes, 1);
    defineDefaultProperty(QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(QStringLiteral("localeCompare"), method_localeCompare, 1);
    defineDefaultProperty(QStringLiteral("match"), method_match, 1);
    defineDefaultProperty(QStringLiteral("repeat"), method_repeat, 1);
    defineDefaultProperty(QStringLiteral("replace"), method_replace, 2);
    defineDefaultProperty(QStringLiteral("search"), method_search, 1);
    defineDefaultProperty(QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(QStringLiteral("split"), method_split, 2);
    defineDefaultProperty(QStringLiteral("startsWith"), method_startsWith, 1);
    defineDefaultProperty(QStringLiteral("substr"), method_substr, 2);
    defineDefaultProperty(QStringLiteral("substring"), method_substring, 2);
    defineDefaultProperty(QStringLiteral("toLowerCase"), method_toLowerCase);
    defineDefaultProperty(QStringLiteral("toLocaleLowerCase"), method_toLocaleLowerCase);
    defineDefaultProperty(QStringLiteral("toUpperCase"), method_toUpperCase);
    defineDefaultProperty(QStringLiteral("toLocaleUpperCase"), method_toLocaleUpperCase);
    defineDefaultProperty(QStringLiteral("trim"), method_trim);
}

static Heap::String *thisAsString(ExecutionEngine *v4, const QV4::Value *thisObject)
{
    if (String *s = thisObject->stringValue())
        return s->d();
    if (const StringObject *thisString = thisObject->as<StringObject>())
        return thisString->d()->string;
    return thisObject->toString(v4);
}

static QString getThisString(ExecutionEngine *v4, const QV4::Value *thisObject)
{
    if (String *s = thisObject->stringValue())
        return s->toQString();
    if (const StringObject *thisString = thisObject->as<StringObject>())
        return thisString->d()->string->toQString();
    if (thisObject->isUndefined() || thisObject->isNull()) {
        v4->throwTypeError();
        return QString();
    }
    return thisObject->toQString();
}

ReturnedValue StringPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    if (thisObject->isString())
        return thisObject->asReturnedValue();

    ExecutionEngine *v4 = b->engine();
    const StringObject *o = thisObject->as<StringObject>();
    if (!o)
        return v4->throwTypeError();
    return o->d()->string->asReturnedValue();
}

ReturnedValue StringPrototype::method_charAt(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString str = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    int pos = 0;
    if (argc > 0)
        pos = (int) argv[0].toInteger();

    QString result;
    if (pos >= 0 && pos < str.length())
        result += str.at(pos);

    return Encode(v4->newString(result));
}

ReturnedValue StringPrototype::method_charCodeAt(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString str = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    int pos = 0;
    if (argc > 0)
        pos = (int) argv[0].toInteger();


    if (pos >= 0 && pos < str.length())
        RETURN_RESULT(Encode(str.at(pos).unicode()));

    return Encode(qt_qnan());
}

ReturnedValue StringPrototype::method_concat(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    Scope scope(v4);
    ScopedString s(scope);
    for (int i = 0; i < argc; ++i) {
        s = argv[i].toString(scope.engine);
        if (v4->hasException)
            return QV4::Encode::undefined();

        Q_ASSERT(s->isString());
        value += s->toQString();
    }

    return Encode(v4->newString(value));
}

ReturnedValue StringPrototype::method_endsWith(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    QString searchString;
    if (argc) {
        if (argv[0].as<RegExpObject>())
            return v4->throwTypeError();
        searchString = argv[0].toQString();
    }

    int pos = value.length();
    if (argc > 1)
        pos = (int) argv[1].toInteger();

    if (pos == value.length())
        RETURN_RESULT(Encode(value.endsWith(searchString)));

    QStringRef stringToSearch = value.leftRef(pos);
    return Encode(stringToSearch.endsWith(searchString));
}

ReturnedValue StringPrototype::method_indexOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    QString searchString;
    if (argc)
        searchString = argv[0].toQString();

    int pos = 0;
    if (argc > 1)
        pos = (int) argv[1].toInteger();

    int index = -1;
    if (! value.isEmpty())
        index = value.indexOf(searchString, qMin(qMax(pos, 0), value.length()));

    return Encode(index);
}

ReturnedValue StringPrototype::method_includes(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    QString searchString;
    if (argc) {
        if (argv[0].as<RegExpObject>())
            return v4->throwTypeError();
        searchString = argv[0].toQString();
    }

    int pos = 0;
    if (argc > 1) {
        const Value &posArg = argv[1];
        pos = (int) posArg.toInteger();
        if (!posArg.isInteger() && posArg.isNumber() && qIsInf(posArg.toNumber()))
            pos = value.length();
    }

    if (pos == 0)
        RETURN_RESULT(Encode(value.contains(searchString)));

    QStringRef stringToSearch = value.midRef(pos);
    return Encode(stringToSearch.contains(searchString));
}

ReturnedValue StringPrototype::method_lastIndexOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    QString searchString;
    if (argc)
        searchString = argv[0].toQString();

    double position = argc > 1 ? RuntimeHelpers::toNumber(argv[1]) : +qInf();
    if (std::isnan(position))
        position = +qInf();
    else
        position = trunc(position);

    int pos = trunc(qMin(qMax(position, 0.0), double(value.length())));
    if (!searchString.isEmpty() && pos == value.length())
        --pos;
    if (searchString.isNull() && pos == 0)
        RETURN_RESULT(Encode(-1));
    int index = value.lastIndexOf(searchString, pos);
    return Encode(index);
}

ReturnedValue StringPrototype::method_localeCompare(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    const QString that = (argc ? argv[0] : Primitive::undefinedValue()).toQString();
    return Encode(QString::localeAwareCompare(value, that));
}

ReturnedValue StringPrototype::method_match(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    if (thisObject->isNullOrUndefined())
        return v4->throwTypeError();

    Scope scope(v4);
    ScopedString s(scope, thisObject->toString(v4));
    if (v4->hasException)
        return Encode::undefined();

    Scoped<RegExpObject> that(scope, argc ? argv[0] : Primitive::undefinedValue());
    if (!that) {
        // convert args[0] to a regexp
        that = RegExpCtor::callAsConstructor(b, argv, argc);
        if (v4->hasException)
            return Encode::undefined();
    }
    Q_ASSERT(!!that);

    bool global = that->global();

    if (!global)
        return RegExpPrototype::method_exec(b, that, s, 1);

    // rx is now in thisObject
    that->setLastIndex(0);
    ScopedArrayObject a(scope, scope.engine->newArrayObject());

    int previousLastIndex = 0;
    uint n = 0;
    while (1) {
        Value result = Primitive::fromReturnedValue(RegExpPrototype::execFirstMatch(b, that, s, 1));
        if (result.isNull())
            break;
        int index = that->lastIndex();
        if (previousLastIndex == index) {
            previousLastIndex = index + 1;
            that->setLastIndex(previousLastIndex);
        } else {
            previousLastIndex = index;
        }
        a->arraySet(n, result);
        ++n;
    }
    if (!n)
        return Encode::null();
    else
        return a.asReturnedValue();
}

ReturnedValue StringPrototype::method_repeat(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    double repeats = (argc ? argv[0] : Primitive::undefinedValue()).toInteger();

    if (repeats < 0 || qIsInf(repeats))
        return v4->throwRangeError(QLatin1String("Invalid count value"));

    return Encode(v4->newString(value.repeated(int(repeats))));
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
                *result += QChar(ch);
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
                Q_ASSERT(capture > 0);
                if (capture < static_cast<uint>(captureCount)) {
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

ReturnedValue StringPrototype::method_replace(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    QString string;
    if (const StringObject *thisString = thisObject->as<StringObject>())
        string = thisString->d()->string->toQString();
    else
        string = thisObject->toQString();

    int numCaptures = 0;
    int numStringMatches = 0;

    uint allocatedMatchOffsets = 64;
    uint _matchOffsets[64];
    uint *matchOffsets = _matchOffsets;

    Scope scope(b);
    ScopedValue searchValue(scope, argc ? argv[0] : Primitive::undefinedValue());
    Scoped<RegExpObject> regExp(scope, searchValue);
    if (regExp) {
        uint offset = 0;
        uint nMatchOffsets = 0;

        // We extract the pointer here to work around a compiler bug on Android.
        Scoped<RegExp> re(scope, regExp->value());
        while (true) {
            int oldSize = nMatchOffsets;
            if (allocatedMatchOffsets < nMatchOffsets + re->captureCount() * 2) {
                allocatedMatchOffsets = qMax(allocatedMatchOffsets * 2, nMatchOffsets + re->captureCount() * 2);
                uint *newOffsets = (uint *)malloc(allocatedMatchOffsets*sizeof(uint));
                memcpy(newOffsets, matchOffsets, nMatchOffsets*sizeof(uint));
                if (matchOffsets != _matchOffsets)
                    free(matchOffsets);
                matchOffsets = newOffsets;
            }
            if (re->match(string, offset, matchOffsets + oldSize) == JSC::Yarr::offsetNoMatch) {
                nMatchOffsets = oldSize;
                break;
            }
            nMatchOffsets += re->captureCount() * 2;
            if (!regExp->global())
                break;
            offset = qMax(offset + 1, matchOffsets[oldSize + 1]);
        }
        if (regExp->global())
            regExp->setLastIndex(0);
        numStringMatches = nMatchOffsets / (regExp->value()->captureCount() * 2);
        numCaptures = regExp->value()->captureCount();
    } else {
        numCaptures = 1;
        QString searchString = searchValue->toQString();
        int idx = string.indexOf(searchString);
        if (idx != -1) {
            numStringMatches = 1;
            matchOffsets[0] = idx;
            matchOffsets[1] = idx + searchString.length();
        }
    }

    QString result;
    ScopedValue replacement(scope);
    ScopedValue replaceValue(scope, argc > 1 ? argv[1] : Primitive::undefinedValue());
    ScopedFunctionObject searchCallback(scope, replaceValue);
    if (!!searchCallback) {
        result.reserve(string.length() + 10*numStringMatches);
        ScopedValue entry(scope);
        Value *arguments = scope.alloc(numCaptures + 2);
        int lastEnd = 0;
        for (int i = 0; i < numStringMatches; ++i) {
            for (int k = 0; k < numCaptures; ++k) {
                int idx = (i * numCaptures + k) * 2;
                uint start = matchOffsets[idx];
                uint end = matchOffsets[idx + 1];
                entry = Primitive::undefinedValue();
                if (start != JSC::Yarr::offsetNoMatch && end != JSC::Yarr::offsetNoMatch)
                    entry = scope.engine->newString(string.mid(start, end - start));
                arguments[k] = entry;
            }
            uint matchStart = matchOffsets[i * numCaptures * 2];
            Q_ASSERT(matchStart >= static_cast<uint>(lastEnd));
            uint matchEnd = matchOffsets[i * numCaptures * 2 + 1];
            arguments[numCaptures] = Primitive::fromUInt32(matchStart);
            arguments[numCaptures + 1] = scope.engine->newString(string);

            Value that = Primitive::undefinedValue();
            replacement = searchCallback->call(&that, arguments, numCaptures + 2);
            result += string.midRef(lastEnd, matchStart - lastEnd);
            result += replacement->toQString();
            lastEnd = matchEnd;
        }
        result += string.midRef(lastEnd);
    } else {
        QString newString = replaceValue->toQString();
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

    return Encode(scope.engine->newString(result));
}

ReturnedValue StringPrototype::method_search(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    QString string = getThisString(scope.engine, thisObject);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    Scoped<RegExpObject> regExp(scope, argc ? argv[0] : Primitive::undefinedValue());
    if (!regExp) {
        regExp = scope.engine->regExpCtor()->callAsConstructor(argv, 1);
        if (scope.engine->hasException)
            return QV4::Encode::undefined();

        Q_ASSERT(regExp);
    }
    Scoped<RegExp> re(scope, regExp->value());
    Q_ALLOCA_VAR(uint, matchOffsets, regExp->value()->captureCount() * 2 * sizeof(uint));
    uint result = re->match(string, /*offset*/0, matchOffsets);
    if (result == JSC::Yarr::offsetNoMatch)
        return Encode(-1);
    else
        return Encode(result);
}

ReturnedValue StringPrototype::method_slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);
    ScopedString s(scope, thisAsString(v4, thisObject));
    if (v4->hasException)
        return QV4::Encode::undefined();
    Q_ASSERT(s);

    const double length = s->d()->length();

    double start = argc ? argv[0].toInteger() : 0;
    double end = (argc < 2 || argv[1].isUndefined())
            ? length : argv[1].toInteger();

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
    return Encode(v4->memoryManager->alloc<ComplexString>(s->d(), intStart, count));
}

ReturnedValue StringPrototype::method_split(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    QString text = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    Scope scope(v4);
    ScopedValue separatorValue(scope, argc ? argv[0] : Primitive::undefinedValue());
    ScopedValue limitValue(scope, argc > 1 ? argv[1] : Primitive::undefinedValue());

    ScopedArrayObject array(scope, scope.engine->newArrayObject());

    if (separatorValue->isUndefined()) {
        if (limitValue->isUndefined()) {
            ScopedString s(scope, scope.engine->newString(text));
            array->push_back(s);
            return array.asReturnedValue();
        }
        RETURN_RESULT(scope.engine->newString(text.left(limitValue->toInteger())));
    }

    uint limit = limitValue->isUndefined() ? UINT_MAX : limitValue->toUInt32();

    if (limit == 0)
        return array.asReturnedValue();

    Scoped<RegExpObject> re(scope, separatorValue);
    if (re) {
        if (re->value()->pattern->isEmpty()) {
            re = (RegExpObject *)nullptr;
            separatorValue = scope.engine->newString();
        }
    }

    ScopedString s(scope);
    if (re) {
        uint offset = 0;
        Q_ALLOCA_VAR(uint, matchOffsets, re->value()->captureCount() * 2 * sizeof(uint));
        while (true) {
            Scoped<RegExp> regexp(scope, re->value());
            uint result = regexp->match(text, offset, matchOffsets);
            if (result == JSC::Yarr::offsetNoMatch)
                break;

            array->push_back((s = scope.engine->newString(text.mid(offset, matchOffsets[0] - offset))));
            offset = qMax(offset + 1, matchOffsets[1]);

            if (array->getLength() >= limit)
                break;

            for (int i = 1; i < re->value()->captureCount(); ++i) {
                uint start = matchOffsets[i * 2];
                uint end = matchOffsets[i * 2 + 1];
                array->push_back((s = scope.engine->newString(text.mid(start, end - start))));
                if (array->getLength() >= limit)
                    break;
            }
        }
        if (array->getLength() < limit)
            array->push_back((s = scope.engine->newString(text.mid(offset))));
    } else {
        QString separator = separatorValue->toQString();
        if (separator.isEmpty()) {
            for (uint i = 0; i < qMin(limit, uint(text.length())); ++i)
                array->push_back((s = scope.engine->newString(text.mid(i, 1))));
            return array.asReturnedValue();
        }

        int start = 0;
        int end;
        while ((end = text.indexOf(separator, start)) != -1) {
            array->push_back((s = scope.engine->newString(text.mid(start, end - start))));
            start = end + separator.size();
            if (array->getLength() >= limit)
                break;
        }
        if (array->getLength() < limit && start != -1)
            array->push_back((s = scope.engine->newString(text.mid(start))));
    }
    return array.asReturnedValue();
}

ReturnedValue StringPrototype::method_startsWith(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    QString searchString;
    if (argc) {
        if (argv[0].as<RegExpObject>())
            return v4->throwTypeError();
        searchString = argv[0].toQString();
    }

    int pos = 0;
    if (argc > 1)
        pos = (int) argv[1].toInteger();

    if (pos == 0)
        return Encode(value.startsWith(searchString));

    QStringRef stringToSearch = value.midRef(pos);
    RETURN_RESULT(Encode(stringToSearch.startsWith(searchString)));
}

ReturnedValue StringPrototype::method_substr(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    double start = 0;
    if (argc > 0)
        start = argv[0].toInteger();

    double length = +qInf();
    if (argc > 1)
        length = argv[1].toInteger();

    double count = value.length();
    if (start < 0)
        start = qMax(count + start, 0.0);

    length = qMin(qMax(length, 0.0), count - start);

    qint32 x = Primitive::toInt32(start);
    qint32 y = Primitive::toInt32(length);
    return Encode(v4->newString(value.mid(x, y)));
}

ReturnedValue StringPrototype::method_substring(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    int length = value.length();

    double start = 0;
    double end = length;

    if (argc > 0)
        start = argv[0].toInteger();

    if (argc > 1 && !argv[1].isUndefined())
        end = argv[1].toInteger();

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
    return Encode(v4->newString(value.mid(x, y)));
}

ReturnedValue StringPrototype::method_toLowerCase(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    return Encode(v4->newString(value.toLower()));
}

ReturnedValue StringPrototype::method_toLocaleLowerCase(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return method_toLowerCase(b, thisObject, argv, argc);
}

ReturnedValue StringPrototype::method_toUpperCase(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const QString value = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

    return Encode(v4->newString(value.toUpper()));
}

ReturnedValue StringPrototype::method_toLocaleUpperCase(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return method_toUpperCase(b, thisObject, argv, argc);
}

ReturnedValue StringPrototype::method_fromCharCode(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QString str(argc, Qt::Uninitialized);
    QChar *ch = str.data();
    for (int i = 0, ei = argc; i < ei; ++i) {
        *ch = QChar(argv[i].toUInt16());
        ++ch;
    }
    return Encode(b->engine()->newString(str));
}

ReturnedValue StringPrototype::method_trim(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    QString s = getThisString(v4, thisObject);
    if (v4->hasException)
        return QV4::Encode::undefined();

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

    return Encode(v4->newString(QString(chars + start, end - start + 1)));
}
