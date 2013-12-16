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
#include <qv4engine_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include "qv4mm_p.h"

#include <wtf/MathExtras.h>

using namespace QV4;

int Value::toUInt16() const
{
    if (integerCompatible())
        return (ushort)(uint)integerValue();

    double number = toNumber();

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
    if (integerCompatible())
        return int_32;

    return Primitive::toInteger(toNumber());
}

double Value::toNumberImpl() const
{
    switch (type()) {
    case QV4::Value::Undefined_Type:
        return std::numeric_limits<double>::quiet_NaN();
    case QV4::Value::Managed_Type:
        if (isString())
            return __qmljs_string_to_number(stringValue()->toQString());
        {
            ExecutionContext *ctx = objectValue()->internalClass->engine->currentContext();
            Scope scope(ctx);
            ScopedValue prim(scope, __qmljs_to_primitive(ValueRef::fromRawValue(this), NUMBER_HINT));
            return prim->toNumber();
        }
    case QV4::Value::Null_Type:
    case QV4::Value::Boolean_Type:
    case QV4::Value::Integer_Type:
    default: // double
        Q_UNREACHABLE();
    }
}

QString Value::toQStringNoThrow() const
{
    switch (type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return QStringLiteral("undefined");
    case Value::Null_Type:
        return QStringLiteral("null");
    case Value::Boolean_Type:
        if (booleanValue())
            return QStringLiteral("true");
        else
            return QStringLiteral("false");
    case Value::Managed_Type:
        if (isString())
            return stringValue()->toQString();
        {
            ExecutionContext *ctx = objectValue()->internalClass->engine->currentContext();
            Scope scope(ctx);
            ScopedValue ex(scope);
            bool caughtException = false;
            ScopedValue prim(scope, __qmljs_to_primitive(ValueRef::fromRawValue(this), STRING_HINT));
            if (scope.hasException()) {
                ex = ctx->catchException();
                caughtException = true;
            } else if (prim->isPrimitive()) {
                    return prim->toQStringNoThrow();
            }
            // Can't nest try/catch due to CXX ABI limitations for foreign exception nesting.
            if (caughtException) {
                ScopedValue prim(scope, __qmljs_to_primitive(ex, STRING_HINT));
                if (scope.hasException()) {
                    ex = ctx->catchException();
                } else if (prim->isPrimitive()) {
                    return prim->toQStringNoThrow();
                }
            }
            return QString();
        }
    case Value::Integer_Type: {
        QString str;
        __qmljs_numberToString(&str, (double)int_32, 10);
        return str;
    }
    default: { // double
        QString str;
        __qmljs_numberToString(&str, doubleValue(), 10);
        return str;
    }
    } // switch
}

QString Value::toQString() const
{
    switch (type()) {
    case Value::Empty_Type:
        Q_ASSERT(!"empty Value encountered");
    case Value::Undefined_Type:
        return QStringLiteral("undefined");
    case Value::Null_Type:
        return QStringLiteral("null");
    case Value::Boolean_Type:
        if (booleanValue())
            return QStringLiteral("true");
        else
            return QStringLiteral("false");
    case Value::Managed_Type:
        if (isString())
            return stringValue()->toQString();
        {
            ExecutionContext *ctx = objectValue()->internalClass->engine->currentContext();
            Scope scope(ctx);
            ScopedValue prim(scope, __qmljs_to_primitive(ValueRef::fromRawValue(this), STRING_HINT));
            return prim->toQString();
        }
    case Value::Integer_Type: {
        QString str;
        __qmljs_numberToString(&str, (double)int_32, 10);
        return str;
    }
    default: { // double
        QString str;
        __qmljs_numberToString(&str, doubleValue(), 10);
        return str;
    }
    } // switch
}

bool Value::sameValue(Value other) const {
    if (val == other.val)
        return true;
    if (isString() && other.isString())
        return stringValue()->isEqualTo(other.stringValue());
    if (isInteger() && other.isDouble())
        return int_32 ? (double(int_32) == other.doubleValue()) : (other.val == 0);
    if (isDouble() && other.isInteger())
        return other.int_32 ? (doubleValue() == double(other.int_32)) : (val == 0);
    return false;
}


int Primitive::toInt32(double number)
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

unsigned int Primitive::toUInt32(double number)
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

double Primitive::toInteger(double number)
{
    if (std::isnan(number))
        return +0;
    else if (! number || std::isinf(number))
        return number;
    const double v = floor(fabs(number));
    return std::signbit(number) ? -v : v;
}

String *Value::toString(ExecutionContext *ctx) const
{
    if (isString())
        return stringValue();
    return __qmljs_convert_to_string(ctx, ValueRef::fromRawValue(this))->getPointer();
}

Object *Value::toObject(ExecutionContext *ctx) const
{
    if (isObject())
        return objectValue();
    return __qmljs_convert_to_object(ctx, ValueRef::fromRawValue(this))->getPointer();
}


PersistentValue::PersistentValue(const ValueRef val)
    : d(new PersistentValuePrivate(val.asReturnedValue()))
{
}

PersistentValue::PersistentValue(ReturnedValue val)
    : d(new PersistentValuePrivate(val))
{
}

PersistentValue::PersistentValue(const PersistentValue &other)
    : d(other.d)
{
    if (d)
        d->ref();
}

PersistentValue &PersistentValue::operator=(const PersistentValue &other)
{
    if (d == other.d)
        return *this;

    // the memory manager cleans up those with a refcount of 0

    if (d)
        d->deref();
    d = other.d;
    if (d)
        d->ref();

    return *this;
}

PersistentValue &PersistentValue::operator =(const ValueRef other)
{
    if (!d) {
        d = new PersistentValuePrivate(other.asReturnedValue());
        return *this;
    }
    d = d->detach(other.asReturnedValue());
    return *this;
}

PersistentValue &PersistentValue::operator =(ReturnedValue other)
{
    if (!d) {
        d = new PersistentValuePrivate(other);
        return *this;
    }
    d = d->detach(other);
    return *this;
}

PersistentValue::~PersistentValue()
{
    if (d)
        d->deref();
}

WeakValue::WeakValue(const ValueRef val)
    : d(new PersistentValuePrivate(val.asReturnedValue(), /*engine*/0, /*weak*/true))
{
}

WeakValue::WeakValue(const WeakValue &other)
    : d(other.d)
{
    if (d)
        d->ref();
}

WeakValue::WeakValue(ReturnedValue val)
    : d(new PersistentValuePrivate(val, /*engine*/0, /*weak*/true))
{
}

WeakValue &WeakValue::operator=(const WeakValue &other)
{
    if (d == other.d)
        return *this;

    // the memory manager cleans up those with a refcount of 0

    if (d)
        d->deref();
    d = other.d;
    if (d)
        d->ref();

    return *this;
}

WeakValue &WeakValue::operator =(const ValueRef other)
{
    if (!d) {
        d = new PersistentValuePrivate(other.asReturnedValue(), /*engine*/0, /*weak*/true);
        return *this;
    }
    d = d->detach(other.asReturnedValue(), /*weak*/true);
    return *this;
}

WeakValue &WeakValue::operator =(const ReturnedValue &other)
{
    if (!d) {
        d = new PersistentValuePrivate(other, /*engine*/0, /*weak*/true);
        return *this;
    }
    d = d->detach(other, /*weak*/true);
    return *this;
}


WeakValue::~WeakValue()
{
    if (d)
        d->deref();
}

void WeakValue::markOnce(ExecutionEngine *e)
{
    if (!d)
        return;
    d->value.mark(e);
}

PersistentValuePrivate::PersistentValuePrivate(ReturnedValue v, ExecutionEngine *e, bool weak)
    : refcount(1)
    , weak(weak)
    , engine(e)
    , prev(0)
    , next(0)
{
    value.val = v;
    init();
}

void PersistentValuePrivate::init()
{
    if (!engine) {
        Managed *m = value.asManaged();
        if (!m)
            return;

        engine = m->engine();
    }
    if (engine && !prev) {
        PersistentValuePrivate **listRoot = weak ? &engine->memoryManager->m_weakValues : &engine->memoryManager->m_persistentValues;

        prev = listRoot;
        next = *listRoot;
        *prev = this;
        if (next)
            next->prev = &this->next;
    }
}

PersistentValuePrivate::~PersistentValuePrivate()
{
}

void PersistentValuePrivate::removeFromList()
{
    if (prev) {
        if (next)
            next->prev = prev;
        *prev = next;
        next = 0;
        prev = 0;
    }
}

void PersistentValuePrivate::deref()
{
    // if engine is not 0, they are registered with the memory manager
    // and will get cleaned up in the next gc run
    if (!--refcount) {
        removeFromList();
        delete this;
    }
}

PersistentValuePrivate *PersistentValuePrivate::detach(const QV4::ReturnedValue val, bool weak)
{
    if (refcount == 1) {
        value.val = val;

        Managed *m = value.asManaged();
        if (!prev) {
            if (m) {
                ExecutionEngine *engine = m->engine();
                if (engine) {
                    PersistentValuePrivate **listRoot = weak ? &engine->memoryManager->m_weakValues : &engine->memoryManager->m_persistentValues;
                    prev = listRoot;
                    next = *listRoot;
                    *prev = this;
                    if (next)
                        next->prev = &this->next;
                }
            }
        } else if (!m)
            removeFromList();

        return this;
    }
    --refcount;
    return new PersistentValuePrivate(val, engine, weak);
}

