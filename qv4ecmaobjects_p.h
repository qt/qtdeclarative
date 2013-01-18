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

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct ObjectPrototype: Object
{
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_getPrototypeOf(ExecutionContext *ctx);
    static Value method_getOwnPropertyDescriptor(ExecutionContext *ctx);
    static Value method_getOwnPropertyNames(ExecutionContext *ctx);
    static Value method_create(ExecutionContext *ctx);
    static Value method_defineProperty(ExecutionContext *ctx);
    static Value method_defineProperties(ExecutionContext *ctx);
    static Value method_seal(ExecutionContext *ctx);
    static Value method_freeze(ExecutionContext *ctx);
    static Value method_preventExtensions(ExecutionContext *ctx);
    static Value method_isSealed(ExecutionContext *ctx);
    static Value method_isFrozen(ExecutionContext *ctx);
    static Value method_isExtensible(ExecutionContext *ctx);
    static Value method_keys(ExecutionContext *ctx);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_toLocaleString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_hasOwnProperty(ExecutionContext *ctx);
    static Value method_isPrototypeOf(ExecutionContext *ctx);
    static Value method_propertyIsEnumerable(ExecutionContext *ctx);

    static Value method_defineGetter(ExecutionContext *ctx);
    static Value method_defineSetter(ExecutionContext *ctx);

    static void toPropertyDescriptor(ExecutionContext *ctx, Value v, PropertyDescriptor *desc);
    static Value fromPropertyDescriptor(ExecutionContext *ctx, const PropertyDescriptor *desc);
};

struct StringCtor: FunctionObject
{
    StringCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct StringPrototype: StringObject
{
    StringPrototype(ExecutionContext *ctx): StringObject(ctx, Value::fromString(ctx, QString())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static QString getThisString(ExecutionContext *ctx);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_charAt(ExecutionContext *ctx);
    static Value method_charCodeAt(ExecutionContext *ctx);
    static Value method_concat(ExecutionContext *ctx);
    static Value method_indexOf(ExecutionContext *ctx);
    static Value method_lastIndexOf(ExecutionContext *ctx);
    static Value method_localeCompare(ExecutionContext *ctx);
    static Value method_match(ExecutionContext *ctx);
    static Value method_replace(ExecutionContext *ctx);
    static Value method_search(ExecutionContext *ctx);
    static Value method_slice(ExecutionContext *ctx);
    static Value method_split(ExecutionContext *ctx);
    static Value method_substr(ExecutionContext *ctx);
    static Value method_substring(ExecutionContext *ctx);
    static Value method_toLowerCase(ExecutionContext *ctx);
    static Value method_toLocaleLowerCase(ExecutionContext *ctx);
    static Value method_toUpperCase(ExecutionContext *ctx);
    static Value method_toLocaleUpperCase(ExecutionContext *ctx);
    static Value method_fromCharCode(ExecutionContext *ctx);
};

struct NumberCtor: FunctionObject
{
    NumberCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct NumberPrototype: NumberObject
{
    NumberPrototype(): NumberObject(Value::fromDouble(0)) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_toLocaleString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_toFixed(ExecutionContext *ctx);
    static Value method_toExponential(ExecutionContext *ctx);
    static Value method_toPrecision(ExecutionContext *ctx);
};

struct BooleanCtor: FunctionObject
{
    BooleanCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct BooleanPrototype: BooleanObject
{
    BooleanPrototype(): BooleanObject(Value::fromBoolean(false)) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
};

struct ArrayCtor: FunctionObject
{
    ArrayCtor(ExecutionContext *scope);

    virtual Value call(ExecutionContext *ctx);
};

struct ArrayPrototype: ArrayObject
{
    ArrayPrototype(ExecutionContext *context) : ArrayObject(context) {}

    void init(ExecutionContext *ctx, const Value &ctor);

    static uint getLength(ExecutionContext *ctx, Object *o);

    static Value method_isArray(ExecutionContext *ctx);
    static Value method_toString(ExecutionContext *ctx);
    static Value method_toLocaleString(ExecutionContext *ctx);
    static Value method_concat(ExecutionContext *ctx);
    static Value method_join(ExecutionContext *ctx);
    static Value method_pop(ExecutionContext *ctx);
    static Value method_push(ExecutionContext *ctx);
    static Value method_reverse(ExecutionContext *ctx);
    static Value method_shift(ExecutionContext *ctx);
    static Value method_slice(ExecutionContext *ctx);
    static Value method_sort(ExecutionContext *ctx);
    static Value method_splice(ExecutionContext *ctx);
    static Value method_unshift(ExecutionContext *ctx);
    static Value method_indexOf(ExecutionContext *ctx);
    static Value method_lastIndexOf(ExecutionContext *ctx);
    static Value method_every(ExecutionContext *ctx);
    static Value method_some(ExecutionContext *ctx);
    static Value method_forEach(ExecutionContext *ctx);
    static Value method_map(ExecutionContext *ctx);
    static Value method_filter(ExecutionContext *ctx);
    static Value method_reduce(ExecutionContext *ctx);
    static Value method_reduceRight(ExecutionContext *ctx);
};

struct FunctionCtor: FunctionObject
{
    FunctionCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(ExecutionContext *ctx): FunctionObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_apply(ExecutionContext *ctx);
    static Value method_call(ExecutionContext *ctx);
    static Value method_bind(ExecutionContext *ctx);
};

struct DateCtor: FunctionObject
{
    DateCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct DatePrototype: DateObject
{
    DatePrototype(): DateObject(Value::fromDouble(qSNaN())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static double getThisDate(ExecutionContext *ctx);

    static Value method_MakeTime(ExecutionContext *ctx);
    static Value method_MakeDate(ExecutionContext *ctx);
    static Value method_TimeClip(ExecutionContext *ctx);
    static Value method_parse(ExecutionContext *ctx);
    static Value method_UTC(ExecutionContext *ctx);
    static Value method_toString(ExecutionContext *ctx);
    static Value method_toDateString(ExecutionContext *ctx);
    static Value method_toTimeString(ExecutionContext *ctx);
    static Value method_toLocaleString(ExecutionContext *ctx);
    static Value method_toLocaleDateString(ExecutionContext *ctx);
    static Value method_toLocaleTimeString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_getTime(ExecutionContext *ctx);
    static Value method_getYear(ExecutionContext *ctx);
    static Value method_getFullYear(ExecutionContext *ctx);
    static Value method_getUTCFullYear(ExecutionContext *ctx);
    static Value method_getMonth(ExecutionContext *ctx);
    static Value method_getUTCMonth(ExecutionContext *ctx);
    static Value method_getDate(ExecutionContext *ctx);
    static Value method_getUTCDate(ExecutionContext *ctx);
    static Value method_getDay(ExecutionContext *ctx);
    static Value method_getUTCDay(ExecutionContext *ctx);
    static Value method_getHours(ExecutionContext *ctx);
    static Value method_getUTCHours(ExecutionContext *ctx);
    static Value method_getMinutes(ExecutionContext *ctx);
    static Value method_getUTCMinutes(ExecutionContext *ctx);
    static Value method_getSeconds(ExecutionContext *ctx);
    static Value method_getUTCSeconds(ExecutionContext *ctx);
    static Value method_getMilliseconds(ExecutionContext *ctx);
    static Value method_getUTCMilliseconds(ExecutionContext *ctx);
    static Value method_getTimezoneOffset(ExecutionContext *ctx);
    static Value method_setTime(ExecutionContext *ctx);
    static Value method_setMilliseconds(ExecutionContext *ctx);
    static Value method_setUTCMilliseconds(ExecutionContext *ctx);
    static Value method_setSeconds(ExecutionContext *ctx);
    static Value method_setUTCSeconds(ExecutionContext *ctx);
    static Value method_setMinutes(ExecutionContext *ctx);
    static Value method_setUTCMinutes(ExecutionContext *ctx);
    static Value method_setHours(ExecutionContext *ctx);
    static Value method_setUTCHours(ExecutionContext *ctx);
    static Value method_setDate(ExecutionContext *ctx);
    static Value method_setUTCDate(ExecutionContext *ctx);
    static Value method_setMonth(ExecutionContext *ctx);
    static Value method_setUTCMonth(ExecutionContext *ctx);
    static Value method_setYear(ExecutionContext *ctx);
    static Value method_setFullYear(ExecutionContext *ctx);
    static Value method_setUTCFullYear(ExecutionContext *ctx);
    static Value method_toUTCString(ExecutionContext *ctx);
};

struct RegExpCtor: FunctionObject
{
    RegExpCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct RegExpPrototype: RegExpObject
{
    RegExpPrototype(ExecutionEngine* engine): RegExpObject(engine, RegExp::create(0, QString()), false) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_exec(ExecutionContext *ctx);
    static Value method_test(ExecutionContext *ctx);
    static Value method_toString(ExecutionContext *ctx);
};

struct ErrorCtor: FunctionObject
{
    ErrorCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct EvalErrorCtor: ErrorCtor
{
    EvalErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};

struct RangeErrorCtor: ErrorCtor
{
    RangeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};

struct ReferenceErrorCtor: ErrorCtor
{
    ReferenceErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};

struct SyntaxErrorCtor: ErrorCtor
{
    SyntaxErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};

struct TypeErrorCtor: ErrorCtor
{
    TypeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};

struct URIErrorCtor: ErrorCtor
{
    URIErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx);
};


struct ErrorPrototype: ErrorObject
{
    // ### shouldn't be undefined
    ErrorPrototype(ExecutionEngine* engine): ErrorObject(engine, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { init(ctx, ctor, this); }

    static void init(ExecutionContext *ctx, const Value &ctor, Object *obj);
    static Value method_toString(ExecutionContext *ctx);
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
    SyntaxErrorPrototype(ExecutionContext *ctx): SyntaxErrorObject(ctx, 0) {}
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

    static Value method_abs(ExecutionContext *ctx);
    static Value method_acos(ExecutionContext *ctx);
    static Value method_asin(ExecutionContext *ctx);
    static Value method_atan(ExecutionContext *ctx);
    static Value method_atan2(ExecutionContext *ctx);
    static Value method_ceil(ExecutionContext *ctx);
    static Value method_cos(ExecutionContext *ctx);
    static Value method_exp(ExecutionContext *ctx);
    static Value method_floor(ExecutionContext *ctx);
    static Value method_log(ExecutionContext *ctx);
    static Value method_max(ExecutionContext *ctx);
    static Value method_min(ExecutionContext *ctx);
    static Value method_pow(ExecutionContext *ctx);
    static Value method_random(ExecutionContext *ctx);
    static Value method_round(ExecutionContext *ctx);
    static Value method_sin(ExecutionContext *ctx);
    static Value method_sqrt(ExecutionContext *ctx);
    static Value method_tan(ExecutionContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
