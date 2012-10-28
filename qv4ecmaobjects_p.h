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
    ObjectCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
    virtual Value getProperty(Context *ctx, String *name);
};

struct ObjectPrototype: Object
{
    void init(Context *ctx, const Value &ctor);

    static void method_getPrototypeOf(Context *ctx);
    static void method_getOwnPropertyDescriptor(Context *ctx);
    static void method_getOwnPropertyNames(Context *ctx);
    static void method_create(Context *ctx);
    static void method_defineProperty(Context *ctx);
    static void method_defineProperties(Context *ctx);
    static void method_seal(Context *ctx);
    static void method_freeze(Context *ctx);
    static void method_preventExtensions(Context *ctx);
    static void method_isSealed(Context *ctx);
    static void method_isFrozen(Context *ctx);
    static void method_isExtensible(Context *ctx);
    static void method_keys(Context *ctx);

    static void method_toString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_hasOwnProperty(Context *ctx);
    static void method_isPrototypeOf(Context *ctx);
    static void method_propertyIsEnumerable(Context *ctx);
};

struct StringCtor: FunctionObject
{
    StringCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct StringPrototype: StringObject
{
    StringPrototype(Context *ctx): StringObject(Value::fromString(ctx, QString())) {}
    void init(Context *ctx, const Value &ctor);

    static QString getThisString(Context *ctx);

    static void method_toString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_charAt(Context *ctx);
    static void method_charCodeAt(Context *ctx);
    static void method_concat(Context *ctx);
    static void method_indexOf(Context *ctx);
    static void method_lastIndexOf(Context *ctx);
    static void method_localeCompare(Context *ctx);
    static void method_match(Context *ctx);
    static void method_replace(Context *ctx);
    static void method_search(Context *ctx);
    static void method_slice(Context *ctx);
    static void method_split(Context *ctx);
    static void method_substr(Context *ctx);
    static void method_substring(Context *ctx);
    static void method_toLowerCase(Context *ctx);
    static void method_toLocaleLowerCase(Context *ctx);
    static void method_toUpperCase(Context *ctx);
    static void method_toLocaleUpperCase(Context *ctx);
    static void method_fromCharCode(Context *ctx);
};

struct NumberCtor: FunctionObject
{
    NumberCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct NumberPrototype: NumberObject
{
    NumberPrototype(): NumberObject(Value::fromDouble(0)) {}
    void init(Context *ctx, const Value &ctor);

    static void method_toString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_toFixed(Context *ctx);
    static void method_toExponential(Context *ctx);
    static void method_toPrecision(Context *ctx);
};

struct BooleanCtor: FunctionObject
{
    BooleanCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct BooleanPrototype: BooleanObject
{
    BooleanPrototype(): BooleanObject(Value::fromBoolean(false)) {}
    void init(Context *ctx, const Value &ctor);

    static void method_toString(Context *ctx);
    static void method_valueOf(Context *ctx);
};

struct ArrayCtor: FunctionObject
{
    ArrayCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct ArrayPrototype: ArrayObject
{
    void init(Context *ctx, const Value &ctor);

    static void method_toString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_concat(Context *ctx);
    static void method_join(Context *ctx);
    static void method_pop(Context *ctx);
    static void method_push(Context *ctx);
    static void method_reverse(Context *ctx);
    static void method_shift(Context *ctx);
    static void method_slice(Context *ctx);
    static void method_sort(Context *ctx);
    static void method_splice(Context *ctx);
    static void method_unshift(Context *ctx);
    static void method_indexOf(Context *ctx);
    static void method_lastIndexOf(Context *ctx);
    static void method_every(Context *ctx);
    static void method_some(Context *ctx);
    static void method_forEach(Context *ctx);
    static void method_map(Context *ctx);
    static void method_filter(Context *ctx);
    static void method_reduce(Context *ctx);
    static void method_reduceRight(Context *ctx);
};

struct FunctionCtor: FunctionObject
{
    FunctionCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(Context *ctx): FunctionObject(ctx) {}
    void init(Context *ctx, const Value &ctor);

    static void method_toString(Context *ctx);
    static void method_apply(Context *ctx);
    static void method_call(Context *ctx);
    static void method_bind(Context *ctx);
};

struct DateCtor: FunctionObject
{
    DateCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct DatePrototype: DateObject
{
    DatePrototype(): DateObject(Value::fromDouble(qSNaN())) {}
    void init(Context *ctx, const Value &ctor);

    static double getThisDate(Context *ctx);

    static void method_MakeTime(Context *ctx);
    static void method_MakeDate(Context *ctx);
    static void method_TimeClip(Context *ctx);
    static void method_parse(Context *ctx);
    static void method_UTC(Context *ctx);
    static void method_toString(Context *ctx);
    static void method_toDateString(Context *ctx);
    static void method_toTimeString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_toLocaleDateString(Context *ctx);
    static void method_toLocaleTimeString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_getTime(Context *ctx);
    static void method_getYear(Context *ctx);
    static void method_getFullYear(Context *ctx);
    static void method_getUTCFullYear(Context *ctx);
    static void method_getMonth(Context *ctx);
    static void method_getUTCMonth(Context *ctx);
    static void method_getDate(Context *ctx);
    static void method_getUTCDate(Context *ctx);
    static void method_getDay(Context *ctx);
    static void method_getUTCDay(Context *ctx);
    static void method_getHours(Context *ctx);
    static void method_getUTCHours(Context *ctx);
    static void method_getMinutes(Context *ctx);
    static void method_getUTCMinutes(Context *ctx);
    static void method_getSeconds(Context *ctx);
    static void method_getUTCSeconds(Context *ctx);
    static void method_getMilliseconds(Context *ctx);
    static void method_getUTCMilliseconds(Context *ctx);
    static void method_getTimezoneOffset(Context *ctx);
    static void method_setTime(Context *ctx);
    static void method_setMilliseconds(Context *ctx);
    static void method_setUTCMilliseconds(Context *ctx);
    static void method_setSeconds(Context *ctx);
    static void method_setUTCSeconds(Context *ctx);
    static void method_setMinutes(Context *ctx);
    static void method_setUTCMinutes(Context *ctx);
    static void method_setHours(Context *ctx);
    static void method_setUTCHours(Context *ctx);
    static void method_setDate(Context *ctx);
    static void method_setUTCDate(Context *ctx);
    static void method_setMonth(Context *ctx);
    static void method_setUTCMonth(Context *ctx);
    static void method_setYear(Context *ctx);
    static void method_setFullYear(Context *ctx);
    static void method_setUTCFullYear(Context *ctx);
    static void method_toUTCString(Context *ctx);
};

struct RegExpCtor: FunctionObject
{
    RegExpCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct RegExpPrototype: RegExpObject
{
    RegExpPrototype(): RegExpObject(QRegularExpression(), false) {}
    void init(Context *ctx, const Value &ctor);

    static void method_exec(Context *ctx);
    static void method_test(Context *ctx);
    static void method_toString(Context *ctx);
};

struct MathObject: Object
{
    MathObject(Context *ctx);

    static void method_abs(Context *ctx);
    static void method_acos(Context *ctx);
    static void method_asin(Context *ctx);
    static void method_atan(Context *ctx);
    static void method_atan2(Context *ctx);
    static void method_ceil(Context *ctx);
    static void method_cos(Context *ctx);
    static void method_exp(Context *ctx);
    static void method_floor(Context *ctx);
    static void method_log(Context *ctx);
    static void method_max(Context *ctx);
    static void method_min(Context *ctx);
    static void method_pow(Context *ctx);
    static void method_random(Context *ctx);
    static void method_round(Context *ctx);
    static void method_sin(Context *ctx);
    static void method_sqrt(Context *ctx);
    static void method_tan(Context *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
