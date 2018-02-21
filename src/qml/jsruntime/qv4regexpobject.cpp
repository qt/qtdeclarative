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

#include "qv4regexpobject_p.h"
#include "qv4objectproto_p.h"
#include "qv4regexp_p.h"
#include "qv4stringobject_p.h"
#include <private/qv4mm_p.h>
#include "qv4scopedvalue_p.h"
#include "qv4jscall_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/QDebug>
#include <QtCore/qregexp.h>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include "qv4alloca_p.h"

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &, QRegExp::PatternSyntax);

using namespace QV4;

DEFINE_OBJECT_VTABLE(RegExpObject);

void Heap::RegExpObject::init()
{
    Object::init();
    Scope scope(internalClass->engine);
    Scoped<QV4::RegExpObject> o(scope, this);
    value.set(scope.engine, QV4::RegExp::create(scope.engine, QString(), false, false));
    o->initProperties();
}

void Heap::RegExpObject::init(QV4::RegExp *value)
{
    Object::init();
    Scope scope(internalClass->engine);
    this->value.set(scope.engine, value->d());
    Scoped<QV4::RegExpObject> o(scope, this);
    o->initProperties();
}

// Converts a QRegExp to a JS RegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
void Heap::RegExpObject::init(const QRegExp &re)
{
    Object::init();

    // Convert the pattern to a ECMAScript pattern.
    QString pattern = QT_PREPEND_NAMESPACE(qt_regexp_toCanonical)(re.pattern(), re.patternSyntax());
    if (re.isMinimal()) {
        QString ecmaPattern;
        int len = pattern.length();
        ecmaPattern.reserve(len);
        int i = 0;
        const QChar *wc = pattern.unicode();
        bool inBracket = false;
        while (i < len) {
            QChar c = wc[i++];
            ecmaPattern += c;
            switch (c.unicode()) {
            case '?':
            case '+':
            case '*':
            case '}':
                if (!inBracket)
                    ecmaPattern += QLatin1Char('?');
                break;
            case '\\':
                if (i < len)
                    ecmaPattern += wc[i++];
                break;
            case '[':
                inBracket = true;
                break;
            case ']':
                inBracket = false;
                break;
            default:
                break;
            }
        }
        pattern = ecmaPattern;
    }

    Scope scope(internalClass->engine);
    Scoped<QV4::RegExpObject> o(scope, this);

    o->d()->value.set(scope.engine,
                        QV4::RegExp::create(scope.engine, pattern, re.caseSensitivity() == Qt::CaseInsensitive, false));

    o->initProperties();
}

void RegExpObject::initProperties()
{
    setProperty(Index_LastIndex, Primitive::fromInt32(0));

    Q_ASSERT(value());

    QString p = *value()->pattern;
    if (p.isEmpty()) {
        p = QStringLiteral("(?:)");
    } else {
        // escape certain parts, see ch. 15.10.4
        p.replace('/', QLatin1String("\\/"));
    }

    setProperty(Index_Source, engine()->newString(p));
    setProperty(Index_Global, Primitive::fromBoolean(global()));
    setProperty(Index_IgnoreCase, Primitive::fromBoolean(value()->ignoreCase));
    setProperty(Index_Multiline, Primitive::fromBoolean(value()->multiLine));
}

// Converts a JS RegExp to a QRegExp.
// The conversion is not 100% exact since ECMA regexp and QRegExp
// have different semantics/flags, but we try to do our best.
QRegExp RegExpObject::toQRegExp() const
{
    Qt::CaseSensitivity caseSensitivity = value()->ignoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
    return QRegExp(*value()->pattern, caseSensitivity, QRegExp::RegExp2);
}

QString RegExpObject::toString() const
{
    QString result = QLatin1Char('/') + source() + QLatin1Char('/');
    if (global())
        result += QLatin1Char('g');
    if (value()->ignoreCase)
        result += QLatin1Char('i');
    if (value()->multiLine)
        result += QLatin1Char('m');
    return result;
}

QString RegExpObject::source() const
{
    Scope scope(engine());
    ScopedString source(scope, scope.engine->newIdentifier(QStringLiteral("source")));
    ScopedValue s(scope, const_cast<RegExpObject *>(this)->get(source));
    return s->toQString();
}

uint RegExpObject::flags() const
{
    uint f = 0;
    if (global())
        f |= QV4::RegExpObject::RegExp_Global;
    if (value()->ignoreCase)
        f |= QV4::RegExpObject::RegExp_IgnoreCase;
    if (value()->multiLine)
        f |= QV4::RegExpObject::RegExp_Multiline;
    return f;
}

DEFINE_OBJECT_VTABLE(RegExpCtor);

void Heap::RegExpCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("RegExp"));
    clearLastMatch();
}

void Heap::RegExpCtor::clearLastMatch()
{
    lastMatch.set(internalClass->engine, Primitive::nullValue());
    lastInput.set(internalClass->engine, internalClass->engine->id_empty()->d());
    lastMatchStart = 0;
    lastMatchEnd = 0;
}

ReturnedValue RegExpCtor::callAsConstructor(const FunctionObject *fo, const Value *argv, int argc)
{
    Scope scope(fo->engine());
    ScopedValue r(scope, argc ? argv[0] : Primitive::undefinedValue());
    ScopedValue f(scope, argc > 1 ? argv[1] : Primitive::undefinedValue());
    Scoped<RegExpObject> re(scope, r);
    if (re) {
        if (!f->isUndefined())
            return scope.engine->throwTypeError();

        Scoped<RegExp> regexp(scope, re->value());
        return Encode(scope.engine->newRegExpObject(regexp));
    }

    QString pattern;
    if (!r->isUndefined())
        pattern = r->toQString();
    if (scope.hasException())
        return Encode::undefined();

    bool global = false;
    bool ignoreCase = false;
    bool multiLine = false;
    if (!f->isUndefined()) {
        ScopedString s(scope, f->toString(scope.engine));
        if (scope.hasException())
            return Encode::undefined();
        QString str = s->toQString();
        for (int i = 0; i < str.length(); ++i) {
            if (str.at(i) == QLatin1Char('g') && !global) {
                global = true;
            } else if (str.at(i) == QLatin1Char('i') && !ignoreCase) {
                ignoreCase = true;
            } else if (str.at(i) == QLatin1Char('m') && !multiLine) {
                multiLine = true;
            } else {
                return scope.engine->throwSyntaxError(QStringLiteral("Invalid flags supplied to RegExp constructor"));
            }
        }
    }

    Scoped<RegExp> regexp(scope, RegExp::create(scope.engine, pattern, ignoreCase, multiLine, global));
    if (!regexp->isValid()) {
        return scope.engine->throwSyntaxError(QStringLiteral("Invalid regular expression"));
    }

    return Encode(scope.engine->newRegExpObject(regexp));
}

ReturnedValue RegExpCtor::call(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    if (argc > 0 && argv[0].as<RegExpObject>()) {
        if (argc == 1 || argv[1].isUndefined())
            return Encode(argv[0]);
    }

    return callAsConstructor(f, argv, argc);
}

void RegExpPrototype::init(ExecutionEngine *engine, Object *constructor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ScopedObject ctor(scope, constructor);

    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    ctor->defineReadonlyProperty(engine->id_length(), Primitive::fromInt32(2));

    // Properties deprecated in the spec but required by "the web" :(
    ctor->defineAccessorProperty(QStringLiteral("lastMatch"), method_get_lastMatch_n<0>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$&"), method_get_lastMatch_n<0>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$1"), method_get_lastMatch_n<1>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$2"), method_get_lastMatch_n<2>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$3"), method_get_lastMatch_n<3>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$4"), method_get_lastMatch_n<4>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$5"), method_get_lastMatch_n<5>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$6"), method_get_lastMatch_n<6>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$7"), method_get_lastMatch_n<7>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$8"), method_get_lastMatch_n<8>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$9"), method_get_lastMatch_n<9>, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("lastParen"), method_get_lastParen, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$+"), method_get_lastParen, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("input"), method_get_input, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$_"), method_get_input, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("leftContext"), method_get_leftContext, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$`"), method_get_leftContext, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("rightContext"), method_get_rightContext, nullptr);
    ctor->defineAccessorProperty(QStringLiteral("$'"), method_get_rightContext, nullptr);

    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(QStringLiteral("exec"), method_exec, 1);
    defineDefaultProperty(QStringLiteral("test"), method_test, 1);
    defineDefaultProperty(engine->id_toString(), method_toString, 0);
    defineDefaultProperty(QStringLiteral("compile"), method_compile, 2);
}

/* used by String.match */
ReturnedValue RegExpPrototype::execFirstMatch(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<RegExpObject> r(scope, thisObject->as<RegExpObject>());
    Q_ASSERT(r && r->global());

    ScopedString str(scope, argc ? argv[0] : Primitive::undefinedValue());
    Q_ASSERT(str);
    QString s = str->toQString();

    int offset = r->lastIndex();
    if (offset < 0 || offset > s.length()) {
        r->setLastIndex(0);
        RETURN_RESULT(Encode::null());
    }

    Q_ALLOCA_VAR(uint, matchOffsets, r->value()->captureCount() * 2 * sizeof(uint));
    const int result = Scoped<RegExp>(scope, r->value())->match(s, offset, matchOffsets);

    RegExpCtor *regExpCtor = static_cast<RegExpCtor *>(scope.engine->regExpCtor());
    regExpCtor->d()->clearLastMatch();

    if (result == -1) {
        r->setLastIndex(0);
        RETURN_RESULT(Encode::null());
    }

    ReturnedValue retVal = Encode::undefined();
    // return first match
    if (r->value()->captureCount()) {
        int start = matchOffsets[0];
        int end = matchOffsets[1];
        retVal = (start != -1) ? scope.engine->memoryManager->alloc<ComplexString>(str->d(), start, end - start)->asReturnedValue() : Encode::undefined();
    }

    RegExpCtor::Data *dd = regExpCtor->d();
    dd->lastInput.set(scope.engine, str->d());
    dd->lastMatchStart = matchOffsets[0];
    dd->lastMatchEnd = matchOffsets[1];

    r->setLastIndex(matchOffsets[1]);

    return retVal;
}

ReturnedValue RegExpPrototype::method_exec(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<RegExpObject> r(scope, thisObject->as<RegExpObject>());
    if (!r)
        return scope.engine->throwTypeError();

    ScopedValue arg(scope, argc ? argv[0]: Primitive::undefinedValue());
    ScopedString str(scope, arg->toString(scope.engine));
    if (scope.hasException())
        RETURN_UNDEFINED();
    QString s = str->toQString();

    int offset = r->global() ? r->lastIndex() : 0;
    if (offset < 0 || offset > s.length()) {
        r->setLastIndex(0);
        RETURN_RESULT(Encode::null());
    }

    Q_ALLOCA_VAR(uint, matchOffsets, r->value()->captureCount() * 2 * sizeof(uint));
    const int result = Scoped<RegExp>(scope, r->value())->match(s, offset, matchOffsets);

    RegExpCtor *regExpCtor = static_cast<RegExpCtor *>(scope.engine->regExpCtor());
    regExpCtor->d()->clearLastMatch();

    if (result == -1) {
        r->setLastIndex(0);
        RETURN_RESULT(Encode::null());
    }

    // fill in result data
    ScopedArrayObject array(scope, scope.engine->newArrayObject(scope.engine->internalClasses[EngineBase::Class_RegExpExecArray], scope.engine->arrayPrototype()));
    int len = r->value()->captureCount();
    array->arrayReserve(len);
    ScopedValue v(scope);
    for (int i = 0; i < len; ++i) {
        int start = matchOffsets[i * 2];
        int end = matchOffsets[i * 2 + 1];
        v = (start != -1) ? scope.engine->memoryManager->alloc<ComplexString>(str->d(), start, end - start)->asReturnedValue() : Encode::undefined();
        array->arrayPut(i, v);
    }
    array->setArrayLengthUnchecked(len);
    array->setProperty(Index_ArrayIndex, Primitive::fromInt32(result));
    array->setProperty(Index_ArrayInput, str);

    RegExpCtor::Data *dd = regExpCtor->d();
    dd->lastMatch.set(scope.engine, array);
    dd->lastInput.set(scope.engine, str->d());
    dd->lastMatchStart = matchOffsets[0];
    dd->lastMatchEnd = matchOffsets[1];

    if (r->global())
        r->setLastIndex(matchOffsets[1]);

    return array.asReturnedValue();
}

ReturnedValue RegExpPrototype::method_test(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Value res = Value::fromReturnedValue(method_exec(b, thisObject, argv, argc));
    return Encode(!res.isNull());
}

ReturnedValue RegExpPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const RegExpObject *r = thisObject->as<RegExpObject>();
    if (!r)
        return v4->throwTypeError();

    return Encode(v4->newString(r->toString()));
}

ReturnedValue RegExpPrototype::method_compile(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<RegExpObject> r(scope, thisObject->as<RegExpObject>());
    if (!r)
        return scope.engine->throwTypeError();

    Scoped<RegExpObject> re(scope, scope.engine->regExpCtor()->callAsConstructor(argv, argc));

    r->d()->value.set(scope.engine, re->value());
    return Encode::undefined();
}

template <int index>
ReturnedValue RegExpPrototype::method_get_lastMatch_n(const FunctionObject *b, const Value *, const Value *, int)
{
    Scope scope(b);
    ScopedArrayObject lastMatch(scope, static_cast<RegExpCtor*>(scope.engine->regExpCtor())->lastMatch());
    ScopedValue res(scope, lastMatch ? lastMatch->getIndexed(index) : Encode::undefined());
    if (res->isUndefined())
        res = scope.engine->newString();
    return res->asReturnedValue();
}

ReturnedValue RegExpPrototype::method_get_lastParen(const FunctionObject *b, const Value *, const Value *, int)
{
    Scope scope(b);
    ScopedArrayObject lastMatch(scope, static_cast<RegExpCtor*>(scope.engine->regExpCtor())->lastMatch());
    ScopedValue res(scope, lastMatch ? lastMatch->getIndexed(lastMatch->getLength() - 1) : Encode::undefined());
    if (res->isUndefined())
        res = scope.engine->newString();
    return res->asReturnedValue();
}

ReturnedValue RegExpPrototype::method_get_input(const FunctionObject *b, const Value *, const Value *, int)
{
    return static_cast<RegExpCtor*>(b->engine()->regExpCtor())->lastInput()->asReturnedValue();
}

ReturnedValue  RegExpPrototype::method_get_leftContext(const FunctionObject *b, const Value *, const Value *, int)
{
    Scope scope(b);
    Scoped<RegExpCtor> regExpCtor(scope, scope.engine->regExpCtor());
    QString lastInput = regExpCtor->lastInput()->toQString();
    return Encode(scope.engine->newString(lastInput.left(regExpCtor->lastMatchStart())));
}

ReturnedValue  RegExpPrototype::method_get_rightContext(const FunctionObject *b, const Value *, const Value *, int)
{
    Scope scope(b);
    Scoped<RegExpCtor> regExpCtor(scope, scope.engine->regExpCtor());
    QString lastInput = regExpCtor->lastInput()->toQString();
    return Encode(scope.engine->newString(lastInput.mid(regExpCtor->lastMatchEnd())));
}

QT_END_NAMESPACE
