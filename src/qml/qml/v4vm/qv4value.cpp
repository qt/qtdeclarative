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
#include <qv4engine_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include "qv4mm_p.h"

#include <wtf/MathExtras.h>

namespace QQmlJS {
namespace VM {


int Value::toUInt16() const
{
    if (isConvertibleToInt())
        return (ushort)(uint)integerValue();

    double number = __qmljs_to_number(*this);

    double D16 = 65536.0;
    if ((number >= 0 && number < D16))
        return static_cast<ushort>(number);

    if (!std::isfinite(number))
        return +0;

    double d = ::floor(::fabs(number));
    if (std::signbit(number))
        d = -d;

    number = ::fmod(d , D16);

    if (number < 0)
        number += D16;

    return (unsigned short)number;
}

double Value::toInteger() const
{
    if (isConvertibleToInt())
        return int_32;

    return Value::toInteger(__qmljs_to_number(*this));
}

double Value::toNumber() const
{
    return __qmljs_to_number(*this);
}

bool Value::sameValue(Value other) const {
    if (val == other.val)
        return true;
    if (isString() && other.isString())
        return stringValue()->isEqualTo(other.stringValue());
    if (isInteger())
        return int_32 ? (double(int_32) == other.dbl) : (other.val == 0);
    if (other.isInteger())
        return other.int_32 ? (dbl == double(other.int_32)) : (val == 0);
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
    if (isnan(number))
        return +0;
    else if (! number || isinf(number))
        return number;
    const double v = floor(fabs(number));
    return std::signbit(number) ? -v : v;
}

Value Value::property(ExecutionContext *ctx, String *name) const
{
    return isObject() ? objectValue()->get(ctx, name) : undefinedValue();
}

PersistentValue::PersistentValue()
    : m_memoryManager(0)
    , m_value(Value::undefinedValue())
{
}

PersistentValue::PersistentValue(MemoryManager *mm, const Value &val)
    : m_memoryManager(mm)
    , m_value(val)
{
    assert(mm);
    if (Managed *m = asManaged())
        m_memoryManager->protect(m);
}

PersistentValue::PersistentValue(const PersistentValue &other)
    : m_memoryManager(other.m_memoryManager)
    , m_value(other.m_value)
{
    if (Managed *m = asManaged())
        m_memoryManager->protect(m);
}

PersistentValue &PersistentValue::operator=(const PersistentValue &other)
{
    if (this == &other)
        return *this;
    if (Managed *m = asManaged())
        m_memoryManager->unprotect(m);
    m_memoryManager = other.m_memoryManager;
    m_value = other.m_value;
    if (Managed *m = asManaged())
        m_memoryManager->protect(m);
}

PersistentValue::~PersistentValue()
{
    if (Managed *m = asManaged())
        m_memoryManager->unprotect(m);
}



} // namespace VM
} // namespace QQmlJS
