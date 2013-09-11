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
#ifndef QV4VALUE_DEF_P_H
#define QV4VALUE_DEF_P_H

#include <QtCore/QString>
#include "qv4global_p.h"

QT_BEGIN_NAMESPACE

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
        NaN_Mask = 0x7ff80000,
        NotDouble_Mask = 0x7ffc0000,
        Type_Mask = 0xffff8000,
        Immediate_Mask = NotDouble_Mask | 0x00008000,
        IsManaged_Mask = Type_Mask & ~0x10000,
        IsNullOrUndefined_Mask = Immediate_Mask | 0x20000,
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
    inline bool isManaged() const { return (tag & IsManaged_Mask) == Object_Type; }
    inline bool isNullOrUndefined() const { return (tag & IsNullOrUndefined_Mask) == Undefined_Type; }
    inline bool isConvertibleToInt() const { return (tag & ConvertibleToInt) == ConvertibleToInt; }
    inline bool isInt32() {
        if (tag == _Integer_Type)
            return true;
        if (isDouble()) {
            int i = (int)dbl;
            if (i == dbl) {
                int_32 = i;
                tag = _Integer_Type;
                return true;
            }
        }
        return false;
    }

    bool booleanValue() const {
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

    bool toBoolean() const;
    double toInteger() const;
    double toNumber() const;
    QString toQStringNoThrow() const;
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
    ArrayObject *asArrayObject() const;
    ErrorObject *asErrorObject() const;

    template<typename T> inline T *as() const;

    uint asArrayIndex() const;
    uint asArrayLength(bool *ok) const;

    Value property(ExecutionContext *ctx, String *name) const;

    inline ExecutionEngine *engine() const;

    ReturnedValue asReturnedValue() const { return val; }
    static Value fromReturnedValue(ReturnedValue val) { Value v; v.val = val; return v; }

    // Section 9.12
    bool sameValue(Value other) const;

    inline void mark() const;
};

}

QT_END_NAMESPACE

#endif // QV4VALUE_DEF_P_H
