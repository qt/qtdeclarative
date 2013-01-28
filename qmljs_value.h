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

#include <wtf/Platform.h>

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <qv4string.h>
#include <QtCore/QDebug>
#include <qv4managed.h>

namespace QQmlJS {
namespace VM {

struct String;
struct ExecutionContext;
struct ExecutionEngine;
struct Value;

extern "C" {
double __qmljs_to_number(Value value, ExecutionContext *ctx);
}

typedef uint Bool;


struct Value
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
#if CPU(X86_64)
#else
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
        Tag_Shift = 32
    };
    enum ValueType {
        Undefined_Type = Immediate_Mask | 0x00000,
        Null_Type = Immediate_Mask | 0x10000,
        Boolean_Type = Immediate_Mask | 0x20000,
        Integer_Type = Immediate_Mask | 0x30000,
        Object_Type = NotDouble_Mask | 0x00000,
        String_Type = NotDouble_Mask | 0x10000
    };

    enum ImmediateFlags {
        ConvertibleToInt = Immediate_Mask | 0x1
    };

    enum ValueTypeInternal {
        _Undefined_Type = Undefined_Type,
        _Null_Type = Null_Type | ConvertibleToInt,
        _Boolean_Type = Boolean_Type | ConvertibleToInt,
        _Integer_Type = Integer_Type | ConvertibleToInt,
        _Object_Type = Object_Type,
        _String_Type = String_Type

    };

    inline unsigned type() const {
        return tag & Type_Mask;
    }

    inline bool isUndefined() const { return tag == _Undefined_Type; }
    inline bool isNull() const { return tag == _Null_Type; }
    inline bool isBoolean() const { return tag == _Boolean_Type; }
    inline bool isInteger() const { return tag == _Integer_Type; }
    inline bool isDouble() const { return (tag & NotDouble_Mask) != NotDouble_Mask; }
    inline bool isNumber() const { return tag == _Integer_Type || (tag & NotDouble_Mask) != NotDouble_Mask; }
#if CPU(X86_64)
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

#if CPU(X86_64)
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
#endif

    static double toInteger(double fromNumber);
    static int toInt32(double value);
    static unsigned int toUInt32(double value);

    int toUInt16(ExecutionContext *ctx) const;
    int toInt32(ExecutionContext *ctx) const;
    unsigned int toUInt32(ExecutionContext *ctx) const;

    Bool toBoolean(ExecutionContext *ctx) const;
    double toInteger(ExecutionContext *ctx) const;
    double toNumber(ExecutionContext *ctx) const;
    String *toString(ExecutionContext *ctx) const;
    Value toObject(ExecutionContext *ctx) const;

    inline bool isPrimitive() const { return !isObject(); }
#if CPU(X86_64)
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

    Object *asObject() const;
    FunctionObject *asFunctionObject() const;
    BooleanObject *asBooleanObject() const;
    NumberObject *asNumberObject() const;
    StringObject *asStringObject() const;
    DateObject *asDateObject() const;
    RegExpObject *asRegExpObject() const;
    ArrayObject *asArrayObject() const;
    ErrorObject *asErrorObject() const;
    uint asArrayIndex() const;
    uint asArrayLength(ExecutionContext *ctx, bool *ok) const;

    Value property(ExecutionContext *ctx, String *name) const;

    // Section 9.12
    bool sameValue(Value other) const;
};

inline Value Value::undefinedValue()
{
    Value v;
#if CPU(X86_64)
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
#if CPU(X86_64)
    v.val = quint64(_Null_Type) << Tag_Shift;
#else
    v.tag = _Null_Type;
    v.int_32 = 0;
#endif
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
#if CPU(X86_64)
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
#if CPU(X86_64)
    v.val = (quint64)o;
    v.val |= quint64(_Object_Type) << Tag_Shift;
#else
    v.tag = _Object_Type;
    v.o = o;
#endif
    return v;
}

inline int Value::toInt32(ExecutionContext *ctx) const
{
    if (isConvertibleToInt())
        return int_32;
    double d;
    if (isDouble())
        d = dbl;
    else
        d = __qmljs_to_number(*this, ctx);

    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((d >= -D31 && d < D31))
        return static_cast<int>(d);

    return Value::toInt32(__qmljs_to_number(*this, ctx));
}

inline unsigned int Value::toUInt32(ExecutionContext *ctx) const {
    if (isConvertibleToInt())
        return (unsigned) int_32;
    double d;
    if (isDouble())
        d = dbl;
    else
        d = __qmljs_to_number(*this, ctx);

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

inline uint Value::asArrayLength(ExecutionContext *ctx, bool *ok) const
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

    uint idx = toUInt32(ctx);
    double d = toNumber(ctx);
    if (d != idx) {
        *ok = false;
        return UINT_MAX;
    }
    return idx;
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


} // namespace VM
} // namespace QQmlJS

#endif
