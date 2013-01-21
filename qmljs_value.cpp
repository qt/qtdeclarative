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
#include <qmljs_engine.h>
#include <qv4object.h>
#include <qv4ecmaobjects_p.h>

namespace QQmlJS {
namespace VM {


int Value::toUInt16(ExecutionContext *ctx)
{
    return __qmljs_to_uint16(*this, ctx);
}

Bool Value::toBoolean(ExecutionContext *ctx) const
{
    return __qmljs_to_boolean(*this, ctx);
}

double Value::toInteger(ExecutionContext *ctx) const
{
    return __qmljs_to_integer(*this, ctx);
}

double Value::toNumber(ExecutionContext *ctx) const
{
    return __qmljs_to_number(*this, ctx);
}

String *Value::toString(ExecutionContext *ctx) const
{
    Value v = __qmljs_to_string(*this, ctx);
    assert(v.isString());
    return v.stringValue();
}

Value Value::toObject(ExecutionContext *ctx) const
{
    return __qmljs_to_object(*this, ctx);
}


bool Value::sameValue(Value other) const {
    if (val == other.val)
        return true;
    if (isString() && other.isString())
        return stringValue()->isEqualTo(other.stringValue());
    if (isInteger() && int_32 == 0 && other.dbl == 0)
        return true;
    if (dbl == 0 && other.isInteger() && other.int_32 == 0)
        return true;
    return false;
}

Value Value::fromString(ExecutionContext *ctx, const QString &s)
{
    return fromString(ctx->engine->newString(s));
}

int Value::toInt32(double number)
{
    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((number >= -D31 && number < D31))
        return static_cast<int>(number);


    if (!std::isfinite(number))
        return 0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < -D31)
        number += D32;
    else if (number >= D31)
        number -= D32;

    return int(number);
}

unsigned int Value::toUInt32(double number)
{
    const double D32 = 4294967296.0;
    if ((number >= 0 && number < D32))
        return static_cast<uint>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D32);

    if (number < 0)
        number += D32;

    return unsigned(number);
}

double Value::toInteger(double number)
{
    if (std::isnan(number))
        return +0;
    else if (! number || std::isinf(number))
        return number;
    const double v = floor(fabs(number));
    return std::signbit(number) ? -v : v;
}

Object *Value::asObject() const
{
    return isObject() ? objectValue() : 0;
}

FunctionObject *Value::asFunctionObject() const
{
    return isObject() ? objectValue()->asFunctionObject() : 0;
}

BooleanObject *Value::asBooleanObject() const
{
    return isObject() ? objectValue()->asBooleanObject() : 0;
}

NumberObject *Value::asNumberObject() const
{
    return isObject() ? objectValue()->asNumberObject() : 0;
}

StringObject *Value::asStringObject() const
{
    return isObject() ? objectValue()->asStringObject() : 0;
}

DateObject *Value::asDateObject() const
{
    return isObject() ? objectValue()->asDateObject() : 0;
}

RegExpObject *Value::asRegExpObject() const
{
    return isObject() ? objectValue()->asRegExpObject() : 0;
}

ArrayObject *Value::asArrayObject() const
{
    return isObject() ? objectValue()->asArrayObject() : 0;
}

ErrorObject *Value::asErrorObject() const
{
    return isObject() ? objectValue()->asErrorObject() : 0;
}

Value Value::property(ExecutionContext *ctx, String *name) const
{
    return isObject() ? objectValue()->__get__(ctx, name) : undefinedValue();
}



} // namespace VM
} // namespace QQmlJS
