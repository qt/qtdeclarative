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
#ifndef QMLJS_VALUE_H
#define QMLJS_VALUE_H

#include <cmath> // this HAS to come

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include "qv4global_p.h"
#include "qv4string_p.h"
#include <QtCore/QDebug>
#include "qv4managed_p.h"

//#include <wtf/MathExtras.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct String;
struct ExecutionContext;
struct ExecutionEngine;
struct Value;

}

extern "C" {
double __qmljs_to_number(const QV4::Value &value);
Q_QML_EXPORT QV4::String *__qmljs_convert_to_string(QV4::ExecutionContext *ctx, const QV4::Value &value);
QV4::Object *__qmljs_convert_to_object(QV4::ExecutionContext *ctx, const QV4::Value &value);
}

namespace QV4 {

typedef uint Bool;


struct Q_QML_EXPORT Value
{
    union {
        quint64 val;
        double dbl;
        struct {
#if Q_BYTE_ORDER != Q_LITTLE_ENDIAN
            uint tag;
#endif
            union {
                uint uint_32;
                int int_32;
#if QT_POINTER_SIZE == 4
                Managed *m;
                Object *o;
                String *s;
#endif
            };
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            uint tag;
#endif
        };
    };

    enum Masks {
        NotDouble_Mask = 0xfffc0000,
        Type_Mask = 0xffff8000,
        Immediate_Mask = NotDouble_Mask | 0x00008000,
        Special_Mask = Immediate_Mask | 0x20000,
        Tag_Shift = 32
    };
    enum ValueType {
        Undefined_Type = Immediate_Mask | 0x00000,
        Null_Type = Immediate_Mask | 0x10000,
        Boolean_Type = Immediate_Mask | 0x20000,
        Integer_Type = Immediate_Mask | 0x30000,
        Object_Type = NotDouble_Mask | 0x00000,
        String_Type = NotDouble_Mask | 0x10000,
        Deleted_Type = NotDouble_Mask | 0x30000
    };

    enum ImmediateFlags {
        ConvertibleToInt = Immediate_Mask | 0x1
    };

    enum ValueTypeInternal {
        _Undefined_Type = Undefined_Type,
        _Empty_Type = Deleted_Type,
        _Null_Type = Null_Type | ConvertibleToInt,
        _Boolean_Type = Boolean_Type | ConvertibleToInt,
        _Integer_Type = Integer_Type | ConvertibleToInt,
        _Object_Type = Object_Type,
        _String_Type = String_Type

    };

    inline unsigned type() const {
        return tag & Type_Mask;
    }

    // used internally in property
    inline bool isEmpty() const { return tag == _Empty_Type; }

    inline bool isUndefined() const { return tag == _Undefined_Type; }
    inline bool isNull() const { return tag == _Null_Type; }
    inline bool isBoolean() const { return tag == _Boolean_Type; }
    inline bool isInteger() const { return tag == _Integer_Type; }
    inline bool isDouble() const { return (tag & NotDouble_Mask) != NotDouble_Mask; }
    inline bool isNumber() const { return tag == _Integer_Type || (tag & NotDouble_Mask) != NotDouble_Mask; }
#if QT_POINTER_SIZE == 8
    inline bool isString() const { return (tag & Type_Mask) == String_Type; }
    inline bool isObject() const { return (tag & Type_Mask) == Object_Type; }
#else
    inline bool isString() const { return tag == String_Type; }
    inline bool isObject() const { return tag == Object_Type; }
#endif
    inline bool isConvertibleToInt() const { return (tag & ConvertibleToInt) == ConvertibleToInt; }

    Bool booleanValue() const {
        return int_32;
    }
    double doubleValue() const {
        return dbl;
    }
    void setDouble(double d) {
        dbl = d;
    }
    double asDouble() const {
        if (tag == _Integer_Type)
            return int_32;
        return dbl;
    }
    int integerValue() const {
        return int_32;
    }

#if QT_POINTER_SIZE == 8
    String *stringValue() const {
        return (String *)(val & ~(quint64(Type_Mask) << Tag_Shift));
    }
    Object *objectValue() const {
        return (Object *)(val & ~(quint64(Type_Mask) << Tag_Shift));
    }
    Managed *managed() const {
        return (Managed *)(val & ~(quint64(Type_Mask) << Tag_Shift));
    }
#else
    String *stringValue() const {
        return s;
    }
    Object *objectValue() const {
        return o;
    }
    Managed *managed() const {
        return m;
    }
#endif

    quint64 rawValue() const {
        return val;
    }

    static Value emptyValue();
    static Value undefinedValue();
    static Value nullValue();
    static Value fromBoolean(Bool b);
    static Value fromDouble(double d);
    static Value fromInt32(int i);
    static Value fromUInt32(uint i);
    static Value fromString(String *s);
    static Value fromObject(Object *o);

#ifndef QMLJS_LLVM_RUNTIME
    static Value fromString(ExecutionContext *ctx, const QString &fromString);
    static Value fromString(ExecutionEngine *engine, const QString &s);
#endif

    static double toInteger(double fromNumber);
    static int toInt32(double value);
    static unsigned int toUInt32(double value);

    int toUInt16() const;
    int toInt32() const;
    unsigned int toUInt32() const;

    Bool toBoolean() const;
    double toInteger() const;
    double toNumber() const;
    QString toQString() const;
    String *toString(ExecutionContext *ctx) const;
    Object *toObject(ExecutionContext *ctx) const;

    inline bool isPrimitive() const { return !isObject(); }
#if QT_POINTER_SIZE == 8
    inline bool integerCompatible() const {
        const quint64 mask = quint64(ConvertibleToInt) << 32;
        return (val & mask) == mask;
    }
    static inline bool integerCompatible(Value a, Value b) {
        const quint64 mask = quint64(ConvertibleToInt) << 32;
        return ((a.val & b.val) & mask) == mask;
    }
    static inline bool bothDouble(Value a, Value b) {
        const quint64 mask = quint64(NotDouble_Mask) << 32;
        return ((a.val | b.val) & mask) != mask;
    }
#else
    inline bool integerCompatible() const {
        return (tag & ConvertibleToInt) == ConvertibleToInt;
    }
    static inline bool integerCompatible(Value a, Value b) {
        return ((a.tag & b.tag) & ConvertibleToInt) == ConvertibleToInt;
    }
    static inline bool bothDouble(Value a, Value b) {
        return ((a.tag | b.tag) & NotDouble_Mask) != NotDouble_Mask;
    }
#endif
    inline bool tryIntegerConversion() {
        bool b = isConvertibleToInt();
        if (b)
            tag = _Integer_Type;
        return b;
    }

    String *asString() const;
    Managed *asManaged() const;
    Object *asObject() const;
    FunctionObject *asFunctionObject() const;
    BooleanObject *asBooleanObject() const;
    NumberObject *asNumberObject() const;
    StringObject *asStringObject() const;
    DateObject *asDateObject() const;
    RegExpObject *asRegExpObject() const;
    ArrayObject *asArrayObject() const;
    ErrorObject *asErrorObject() const;
    VariantObject *asVariantObject() const;
    QObjectWrapper *asQObjectWrapper() const;
    uint asArrayIndex() const;
    uint asArrayLength(bool *ok) const;

    Value property(ExecutionContext *ctx, String *name) const;

    ExecutionEngine *engine() const {
        Managed *m = asManaged();
        return m ? m->engine() : 0;
    }

    // Section 9.12
    bool sameValue(Value other) const;

    void mark() const {
        Managed *m = asManaged();
        if (m)
            m->mark();
    }
};

inline Value Value::undefinedValue()
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.val = quint64(_Undefined_Type) << Tag_Shift;
#else
    v.tag = _Undefined_Type;
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
    v.tag = Value::_Empty_Type;
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
    v.dbl = d;
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
        v.dbl = i;
    }
    return v;
}

inline Value Value::fromString(String *s)
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.val = (quint64)s;
    v.val |= quint64(_String_Type) << Tag_Shift;
#else
    v.tag = _String_Type;
    v.s = s;
#endif
    return v;
}

inline Value Value::fromObject(Object *o)
{
    Value v;
#if QT_POINTER_SIZE == 8
    v.val = (quint64)o;
    v.val |= quint64(_Object_Type) << Tag_Shift;
#else
    v.tag = _Object_Type;
    v.o = o;
#endif
    return v;
}

inline Bool Value::toBoolean() const
{
    switch (type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return false;
    case Value::Boolean_Type:
    case Value::Integer_Type:
        return (bool)int_32;
    case Value::String_Type:
        return stringValue()->toQString().length() > 0;
    case Value::Object_Type:
        return true;
    default: // double
        if (! doubleValue() || std::isnan(doubleValue()))
            return false;
        return true;
    }
}

inline Object *Value::toObject(ExecutionContext *ctx) const
{
    if (isObject())
        return objectValue();
    return __qmljs_convert_to_object(ctx, *this);
}

inline int Value::toInt32() const
{
    if (isConvertibleToInt())
        return int_32;
    double d;
    if (isDouble())
        d = dbl;
    else
        d = __qmljs_to_number(*this);

    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((d >= -D31 && d < D31))
        return static_cast<int>(d);

    return Value::toInt32(__qmljs_to_number(*this));
}

inline unsigned int Value::toUInt32() const
{
    if (isConvertibleToInt())
        return (unsigned) int_32;
    double d;
    if (isDouble())
        d = dbl;
    else
        d = __qmljs_to_number(*this);

    const double D32 = 4294967296.0;
    if (dbl >= 0 && dbl < D32)
        return static_cast<uint>(dbl);
    return toUInt32(d);
}

inline uint Value::asArrayIndex() const
{
    if (isInteger() && int_32 >= 0)
        return (uint)int_32;
    if (!isDouble())
        return UINT_MAX;
    uint idx = (uint)dbl;
    if (idx != dbl)
        return UINT_MAX;
    return idx;
}

inline uint Value::asArrayLength(bool *ok) const
{
    *ok = true;
    if (isConvertibleToInt() && int_32 >= 0)
        return (uint)int_32;
    if (isDouble()) {
        uint idx = (uint)dbl;
        if ((double)idx != dbl) {
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

inline Managed *Value::asManaged() const
{
    if (isObject() || isString())
        return managed();
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

inline RegExpObject *Value::asRegExpObject() const
{
    return isObject() ? managed()->asRegExpObject() : 0;
}

inline ArrayObject *Value::asArrayObject() const
{
    return isObject() ? managed()->asArrayObject() : 0;
}

inline ErrorObject *Value::asErrorObject() const
{
    return isObject() ? managed()->asErrorObject() : 0;
}

inline VariantObject *Value::asVariantObject() const
{
    return isObject() ? managed()->asVariantObject() : 0;
}

inline QObjectWrapper *Value::asQObjectWrapper() const
{
    return isObject() ? managed()->asQObjectWrapper() : 0;
}

// ###
inline Value Managed::construct(ExecutionContext *context, Value *args, int argc) {
    return vtbl->construct(this, context, args, argc);
}
inline Value Managed::call(ExecutionContext *context, const Value &thisObject, Value *args, int argc) {
    return vtbl->call(this, context, thisObject, args, argc);
}

struct PersistentValuePrivate
{
    PersistentValuePrivate(const Value &v, bool weak = false);
    Value value;
    uint refcount;
    PersistentValuePrivate **prev;
    PersistentValuePrivate *next;

    void removeFromList();
    void ref() { ++refcount; }
    void deref();
};

class Q_QML_EXPORT PersistentValue
{
public:
    PersistentValue() : d(0) {}
    PersistentValue(const Value &val);
    PersistentValue(const PersistentValue &other);
    PersistentValue &operator=(const PersistentValue &other);
    PersistentValue &operator=(const Value &other);
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
    PersistentValuePrivate *d;
};

class Q_QML_EXPORT WeakValue
{
public:
    WeakValue() : d(0) {}
    WeakValue(const Value &val);
    WeakValue(const WeakValue &other);
    WeakValue &operator=(const WeakValue &other);
    WeakValue &operator=(const Value &other);
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

private:
    PersistentValuePrivate *d;
};

} // namespace QV4

QT_END_NAMESPACE

#endif
