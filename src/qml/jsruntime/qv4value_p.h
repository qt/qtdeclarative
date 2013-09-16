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
#ifndef QMLJS_VALUE_H
#define QMLJS_VALUE_H

#include <cmath> // this HAS to come

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include "qv4global_p.h"
#include "qv4string_p.h"
#include <QtCore/QDebug>
#include "qv4managed_p.h"
#include <private/qtqmlglobal_p.h>

//#include <wtf/MathExtras.h>

#include "qv4value_def_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

inline bool Value::isString() const
{
    if (!isManaged())
        return false;
    return managed() && managed()->type == Managed::Type_String;
}
inline bool Value::isObject() const
{
    if (!isManaged())
        return false;
    return managed() && managed()->type != Managed::Type_String;
}

inline bool Value::isPrimitive() const
{
    return !isObject();
}

inline Managed *Value::asManaged() const
{
    if (isManaged())
        return managed();
    return 0;
}

inline ExecutionEngine *Value::engine() const {
    Managed *m = asManaged();
    return m ? m->engine() : 0;
}

inline void Value::mark() const {
    Managed *m = asManaged();
    if (m)
        m->mark();
}

inline Value Value::undefinedValue()
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.val = quint64(Undefined_Type) << Tag_Shift;
#else
    v.tag = Undefined_Type;
    v.int_32 = 0;
#endif
    return v;
}

inline Value Value::nullValue()
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.val = quint64(_Null_Type) << Tag_Shift;
#else
    v.tag = _Null_Type;
    v.int_32 = 0;
#endif
    return v;
}

inline Value Value::emptyValue()
{
    Value v;
    v.tag = Value::Empty_Type;
    v.uint_32 = 0;
    return v;
}


inline Value Value::fromBoolean(Bool b)
{
    Value v;
    v.tag = _Boolean_Type;
    v.int_32 = (bool)b;
    return v;
}

inline Value Value::fromDouble(double d)
{
    Value v;
    v.setDouble(d);
    return v;
}

inline Value Value::fromInt32(int i)
{
    Value v;
    v.tag = _Integer_Type;
    v.int_32 = i;
    return v;
}

inline Value Value::fromUInt32(uint i)
{
    Value v;
    if (i < INT_MAX) {
        v.tag = _Integer_Type;
        v.int_32 = (int)i;
    } else {
        v.setDouble(i);
    }
    return v;
}

inline Value Value::fromString(String *s)
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.s = s;
#else
    v.tag = Managed_Type;
    v.s = s;
#endif
    return v;
}

inline Value Value::fromObject(Object *o)
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.o = o;
#else
    v.tag = Managed_Type;
    v.o = o;
#endif
    return v;
}

inline Value Value::fromManaged(Managed *m)
{
    if (!m)
        return QV4::Value::undefinedValue();
    Value v;
#if QT_POINTER_SIZE == 8
    v.m = m;
#else
    v.tag = Managed_Type;
    v.m = m;
#endif
    return v;
}

inline double Value::toNumber() const
{
    if (integerCompatible())
        return int_32;
    if (isDouble())
        return doubleValue();
    return toNumberImpl();
}

inline int Value::toInt32() const
{
    if (integerCompatible())
        return int_32;
    double d;
    if (isDouble())
        d = doubleValue();
    else
        d = toNumberImpl();

    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((d >= -D31 && d < D31))
        return static_cast<int>(d);

    return Value::toInt32(d);
}

inline unsigned int Value::toUInt32() const
{
    return (unsigned int)toInt32();
}


inline bool Value::toBoolean() const
{
    switch (type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return false;
    case Value::Boolean_Type:
    case Value::Integer_Type:
        return (bool)int_32;
    case Value::Managed_Type:
        if (isString())
            return stringValue()->toQString().length() > 0;
        return true;
    default: // double
        return doubleValue() && !std::isnan(doubleValue());
    }
}

inline uint Value::asArrayIndex() const
{
    if (isInteger() && int_32 >= 0)
        return (uint)int_32;
    if (!isDouble())
        return UINT_MAX;
    double d = doubleValue();
    uint idx = (uint)d;
    if (idx != d)
        return UINT_MAX;
    return idx;
}

inline uint Value::asArrayLength(bool *ok) const
{
    *ok = true;
    if (integerCompatible() && int_32 >= 0)
        return (uint)int_32;
    if (isDouble()) {
        double d = doubleValue();
        uint idx = (uint)d;
        if (idx != d) {
            *ok = false;
            return UINT_MAX;
        }
        return idx;
    }
    if (isString())
        return stringValue()->toUInt(ok);

    uint idx = toUInt32();
    double d = toNumber();
    if (d != idx) {
        *ok = false;
        return UINT_MAX;
    }
    return idx;
}

inline String *Value::asString() const
{
    if (isString())
        return stringValue();
    return 0;
}

inline Object *Value::asObject() const
{
    return isObject() ? objectValue() : 0;
}

inline FunctionObject *Value::asFunctionObject() const
{
    return isObject() ? managed()->asFunctionObject() : 0;
}

inline BooleanObject *Value::asBooleanObject() const
{
    return isObject() ? managed()->asBooleanObject() : 0;
}

inline NumberObject *Value::asNumberObject() const
{
    return isObject() ? managed()->asNumberObject() : 0;
}

inline StringObject *Value::asStringObject() const
{
    return isObject() ? managed()->asStringObject() : 0;
}

inline DateObject *Value::asDateObject() const
{
    return isObject() ? managed()->asDateObject() : 0;
}

inline ArrayObject *Value::asArrayObject() const
{
    return isObject() ? managed()->asArrayObject() : 0;
}

inline ErrorObject *Value::asErrorObject() const
{
    return isObject() ? managed()->asErrorObject() : 0;
}

template<typename T>
inline T *Value::as() const { Managed *m = isObject() ? managed() : 0; return m ? m->as<T>() : 0; }

struct Q_QML_PRIVATE_EXPORT PersistentValuePrivate
{
    PersistentValuePrivate(const Value &v, ExecutionEngine *engine = 0, bool weak = false);
    virtual ~PersistentValuePrivate();
    Value value;
    uint refcount;
    QV4::ExecutionEngine *engine;
    PersistentValuePrivate **prev;
    PersistentValuePrivate *next;

    void removeFromList();
    void ref() { ++refcount; }
    void deref();
    PersistentValuePrivate *detach(const QV4::Value &value, bool weak = false);

    bool checkEngine(QV4::ExecutionEngine *otherEngine) {
        if (!engine) {
            Q_ASSERT(!value.isObject());
            engine = otherEngine;
        }
        return (engine == otherEngine);
    }
};

class Q_QML_EXPORT PersistentValue
{
public:
    PersistentValue() : d(0) {}

    PersistentValue(const Value &val);
    PersistentValue(const ScopedValue &val);
    PersistentValue(ReturnedValue val);
    template<typename T>
    PersistentValue(Returned<T> *obj);
    template<typename T>
    PersistentValue(const Scoped<T> &obj);
    PersistentValue(const PersistentValue &other);
    PersistentValue &operator=(const PersistentValue &other);
    PersistentValue &operator=(const Value &other);
    PersistentValue &operator=(const ScopedValue &other);
    PersistentValue &operator=(const ValueRef other);
    PersistentValue &operator =(const ReturnedValue &other);
    template<typename T>
    PersistentValue &operator=(Returned<T> *obj);
    template<typename T>
    PersistentValue &operator=(const Scoped<T> &obj);
    ~PersistentValue();

    Value value() const {
        return d ? d->value : Value::emptyValue();
    }

    ExecutionEngine *engine() {
        if (!d)
            return 0;
        Managed *m = d->value.asManaged();
        return m ? m->engine() : 0;
    }

    operator Value() const { return value(); }

    bool isEmpty() const { return !d || d->value.isEmpty(); }
    void clear() {
        *this = PersistentValue();
    }

private:
    friend struct ValueRef;
    PersistentValuePrivate *d;
};

class Q_QML_EXPORT WeakValue
{
public:
    WeakValue() : d(0) {}
    WeakValue(const Value &val);
    WeakValue(const WeakValue &other);
    WeakValue(ReturnedValue val);
    template<typename T>
    WeakValue(Returned<T> *obj);
    WeakValue &operator=(const WeakValue &other);
    WeakValue &operator=(const Value &other);
    WeakValue &operator =(const ReturnedValue &other);
    template<typename T>
    WeakValue &operator=(Returned<T> *obj);

    ~WeakValue();

    Value value() const {
        return d ? d->value : Value::emptyValue();
    }

    ExecutionEngine *engine() {
        if (!d)
            return 0;
        Managed *m = d->value.asManaged();
        return m ? m->engine() : 0;
    }

    operator Value() const { return value(); }

    bool isEmpty() const { return !d || d->value.isEmpty(); }
    void clear() {
        *this = WeakValue();
    }

    void markOnce();

private:
    friend struct ValueRef;
    PersistentValuePrivate *d;
};

} // namespace QV4

QT_END_NAMESPACE

#endif
