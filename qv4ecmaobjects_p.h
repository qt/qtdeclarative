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
#ifndef QV4ECMAOBJECTS_P_H
#define QV4ECMAOBJECTS_P_H

#include "qmljs_objects.h"
#include <QtCore/qnumeric.h>

namespace QQmlJS {
namespace VM {

struct ObjectCtor: FunctionObject
{
    ObjectCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
    virtual Value __get__(ExecutionContext *ctx, String *name);
};

struct ObjectPrototype: Object
{
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_getPrototypeOf(ExecutionContext *ctx);
    static void method_getOwnPropertyDescriptor(ExecutionContext *ctx);
    static void method_getOwnPropertyNames(ExecutionContext *ctx);
    static void method_create(ExecutionContext *ctx);
    static void method_defineProperty(ExecutionContext *ctx);
    static void method_defineProperties(ExecutionContext *ctx);
    static void method_seal(ExecutionContext *ctx);
    static void method_freeze(ExecutionContext *ctx);
    static void method_preventExtensions(ExecutionContext *ctx);
    static void method_isSealed(ExecutionContext *ctx);
    static void method_isFrozen(ExecutionContext *ctx);
    static void method_isExtensible(ExecutionContext *ctx);
    static void method_keys(ExecutionContext *ctx);

    static void method_toString(ExecutionContext *ctx);
    static void method_toLocaleString(ExecutionContext *ctx);
    static void method_valueOf(ExecutionContext *ctx);
    static void method_hasOwnProperty(ExecutionContext *ctx);
    static void method_isPrototypeOf(ExecutionContext *ctx);
    static void method_propertyIsEnumerable(ExecutionContext *ctx);
};

struct StringCtor: FunctionObject
{
    StringCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct StringPrototype: StringObject
{
    StringPrototype(ExecutionContext *ctx): StringObject(Value::fromString(ctx, QString())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static QString getThisString(ExecutionContext *ctx);

    static void method_toString(ExecutionContext *ctx);
    static void method_valueOf(ExecutionContext *ctx);
    static void method_charAt(ExecutionContext *ctx);
    static void method_charCodeAt(ExecutionContext *ctx);
    static void method_concat(ExecutionContext *ctx);
    static void method_indexOf(ExecutionContext *ctx);
    static void method_lastIndexOf(ExecutionContext *ctx);
    static void method_localeCompare(ExecutionContext *ctx);
    static void method_match(ExecutionContext *ctx);
    static void method_replace(ExecutionContext *ctx);
    static void method_search(ExecutionContext *ctx);
    static void method_slice(ExecutionContext *ctx);
    static void method_split(ExecutionContext *ctx);
    static void method_substr(ExecutionContext *ctx);
    static void method_substring(ExecutionContext *ctx);
    static void method_toLowerCase(ExecutionContext *ctx);
    static void method_toLocaleLowerCase(ExecutionContext *ctx);
    static void method_toUpperCase(ExecutionContext *ctx);
    static void method_toLocaleUpperCase(ExecutionContext *ctx);
    static void method_fromCharCode(ExecutionContext *ctx);
};

struct NumberCtor: FunctionObject
{
    NumberCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct NumberPrototype: NumberObject
{
    NumberPrototype(): NumberObject(Value::fromDouble(0)) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_toString(ExecutionContext *ctx);
    static void method_toLocaleString(ExecutionContext *ctx);
    static void method_valueOf(ExecutionContext *ctx);
    static void method_toFixed(ExecutionContext *ctx);
    static void method_toExponential(ExecutionContext *ctx);
    static void method_toPrecision(ExecutionContext *ctx);
};

struct BooleanCtor: FunctionObject
{
    BooleanCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct BooleanPrototype: BooleanObject
{
    BooleanPrototype(): BooleanObject(Value::fromBoolean(false)) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_toString(ExecutionContext *ctx);
    static void method_valueOf(ExecutionContext *ctx);
};

struct ArrayCtor: FunctionObject
{
    ArrayCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct ArrayPrototype: ArrayObject
{
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_toString(ExecutionContext *ctx);
    static void method_toLocaleString(ExecutionContext *ctx);
    static void method_concat(ExecutionContext *ctx);
    static void method_join(ExecutionContext *ctx);
    static void method_pop(ExecutionContext *ctx);
    static void method_push(ExecutionContext *ctx);
    static void method_reverse(ExecutionContext *ctx);
    static void method_shift(ExecutionContext *ctx);
    static void method_slice(ExecutionContext *ctx);
    static void method_sort(ExecutionContext *ctx);
    static void method_splice(ExecutionContext *ctx);
    static void method_unshift(ExecutionContext *ctx);
    static void method_indexOf(ExecutionContext *ctx);
    static void method_lastIndexOf(ExecutionContext *ctx);
    static void method_every(ExecutionContext *ctx);
    static void method_some(ExecutionContext *ctx);
    static void method_forEach(ExecutionContext *ctx);
    static void method_map(ExecutionContext *ctx);
    static void method_filter(ExecutionContext *ctx);
    static void method_reduce(ExecutionContext *ctx);
    static void method_reduceRight(ExecutionContext *ctx);
};

struct FunctionCtor: FunctionObject
{
    FunctionCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(ExecutionContext *ctx): FunctionObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_toString(ExecutionContext *ctx);
    static void method_apply(ExecutionContext *ctx);
    static void method_call(ExecutionContext *ctx);
    static void method_bind(ExecutionContext *ctx);
};

struct DateCtor: FunctionObject
{
    DateCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct DatePrototype: DateObject
{
    DatePrototype(): DateObject(Value::fromDouble(qSNaN())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static double getThisDate(ExecutionContext *ctx);

    static void method_MakeTime(ExecutionContext *ctx);
    static void method_MakeDate(ExecutionContext *ctx);
    static void method_TimeClip(ExecutionContext *ctx);
    static void method_parse(ExecutionContext *ctx);
    static void method_UTC(ExecutionContext *ctx);
    static void method_toString(ExecutionContext *ctx);
    static void method_toDateString(ExecutionContext *ctx);
    static void method_toTimeString(ExecutionContext *ctx);
    static void method_toLocaleString(ExecutionContext *ctx);
    static void method_toLocaleDateString(ExecutionContext *ctx);
    static void method_toLocaleTimeString(ExecutionContext *ctx);
    static void method_valueOf(ExecutionContext *ctx);
    static void method_getTime(ExecutionContext *ctx);
    static void method_getYear(ExecutionContext *ctx);
    static void method_getFullYear(ExecutionContext *ctx);
    static void method_getUTCFullYear(ExecutionContext *ctx);
    static void method_getMonth(ExecutionContext *ctx);
    static void method_getUTCMonth(ExecutionContext *ctx);
    static void method_getDate(ExecutionContext *ctx);
    static void method_getUTCDate(ExecutionContext *ctx);
    static void method_getDay(ExecutionContext *ctx);
    static void method_getUTCDay(ExecutionContext *ctx);
    static void method_getHours(ExecutionContext *ctx);
    static void method_getUTCHours(ExecutionContext *ctx);
    static void method_getMinutes(ExecutionContext *ctx);
    static void method_getUTCMinutes(ExecutionContext *ctx);
    static void method_getSeconds(ExecutionContext *ctx);
    static void method_getUTCSeconds(ExecutionContext *ctx);
    static void method_getMilliseconds(ExecutionContext *ctx);
    static void method_getUTCMilliseconds(ExecutionContext *ctx);
    static void method_getTimezoneOffset(ExecutionContext *ctx);
    static void method_setTime(ExecutionContext *ctx);
    static void method_setMilliseconds(ExecutionContext *ctx);
    static void method_setUTCMilliseconds(ExecutionContext *ctx);
    static void method_setSeconds(ExecutionContext *ctx);
    static void method_setUTCSeconds(ExecutionContext *ctx);
    static void method_setMinutes(ExecutionContext *ctx);
    static void method_setUTCMinutes(ExecutionContext *ctx);
    static void method_setHours(ExecutionContext *ctx);
    static void method_setUTCHours(ExecutionContext *ctx);
    static void method_setDate(ExecutionContext *ctx);
    static void method_setUTCDate(ExecutionContext *ctx);
    static void method_setMonth(ExecutionContext *ctx);
    static void method_setUTCMonth(ExecutionContext *ctx);
    static void method_setYear(ExecutionContext *ctx);
    static void method_setFullYear(ExecutionContext *ctx);
    static void method_setUTCFullYear(ExecutionContext *ctx);
    static void method_toUTCString(ExecutionContext *ctx);
};

struct RegExpCtor: FunctionObject
{
    RegExpCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct RegExpPrototype: RegExpObject
{
    RegExpPrototype(): RegExpObject(QRegularExpression(), false) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static void method_exec(ExecutionContext *ctx);
    static void method_test(ExecutionContext *ctx);
    static void method_toString(ExecutionContext *ctx);
};

struct ErrorCtor: FunctionObject
{
    ErrorCtor(ExecutionContext *scope);

    virtual void construct(ExecutionContext *ctx);
    virtual void call(ExecutionContext *ctx);
};

struct EvalErrorCtor: ErrorCtor
{
    EvalErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};

struct RangeErrorCtor: ErrorCtor
{
    RangeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};

struct ReferenceErrorCtor: ErrorCtor
{
    ReferenceErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};

struct SyntaxErrorCtor: ErrorCtor
{
    SyntaxErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};

struct TypeErrorCtor: ErrorCtor
{
    TypeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};

struct URIErrorCtor: ErrorCtor
{
    URIErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual void construct(ExecutionContext *ctx);
};


struct ErrorPrototype: ErrorObject
{
    // ### shouldn't be undefined
    ErrorPrototype(): ErrorObject(Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { init(ctx, ctor, this); }

    static void init(ExecutionContext *ctx, const Value &ctor, Object *obj);
    static void method_toString(ExecutionContext *ctx);
};

struct EvalErrorPrototype: EvalErrorObject
{
    EvalErrorPrototype(ExecutionContext *ctx): EvalErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct RangeErrorPrototype: RangeErrorObject
{
    RangeErrorPrototype(ExecutionContext *ctx): RangeErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct ReferenceErrorPrototype: ReferenceErrorObject
{
    ReferenceErrorPrototype(ExecutionContext *ctx): ReferenceErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct SyntaxErrorPrototype: SyntaxErrorObject
{
    SyntaxErrorPrototype(ExecutionContext *ctx): SyntaxErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct TypeErrorPrototype: TypeErrorObject
{
    TypeErrorPrototype(ExecutionContext *ctx): TypeErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct URIErrorPrototype: URIErrorObject
{
    URIErrorPrototype(ExecutionContext *ctx): URIErrorObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};


struct MathObject: Object
{
    MathObject(ExecutionContext *ctx);

    static void method_abs(ExecutionContext *ctx);
    static void method_acos(ExecutionContext *ctx);
    static void method_asin(ExecutionContext *ctx);
    static void method_atan(ExecutionContext *ctx);
    static void method_atan2(ExecutionContext *ctx);
    static void method_ceil(ExecutionContext *ctx);
    static void method_cos(ExecutionContext *ctx);
    static void method_exp(ExecutionContext *ctx);
    static void method_floor(ExecutionContext *ctx);
    static void method_log(ExecutionContext *ctx);
    static void method_max(ExecutionContext *ctx);
    static void method_min(ExecutionContext *ctx);
    static void method_pow(ExecutionContext *ctx);
    static void method_random(ExecutionContext *ctx);
    static void method_round(ExecutionContext *ctx);
    static void method_sin(ExecutionContext *ctx);
    static void method_sqrt(ExecutionContext *ctx);
    static void method_tan(ExecutionContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
