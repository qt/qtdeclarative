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
#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#ifndef QMLJS_LLVM_RUNTIME
#  include <QtCore/QString>
#  include <QtCore/qnumeric.h>
#  include <QtCore/QDebug>
#endif

#include <cmath>
#include <cassert>

namespace QQmlJS {

namespace IR {
struct Function;
}

namespace VM {

enum TypeHint {
    PREFERREDTYPE_HINT,
    NUMBER_HINT,
    STRING_HINT
};

enum PropertyAttributes {
    NoAttributes          = 0,
    ValueAttribute        = 1,
    WritableAttribute     = 2,
    EnumerableAttribute   = 4,
    ConfigurableAttribute = 8
};

struct Value;
struct Object;
struct String;
struct Context;
struct FunctionObject;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct DateObject;
struct ArrayObject;
struct ErrorObject;
struct ActivationObject;
struct ExecutionEngine;

typedef uint Bool;

extern "C" {

// context
Value __qmljs_call_activation_property(Context *, String *name, Value *args, int argc);
Value __qmljs_call_property(Context *context, Value base, String *name, Value *args, int argc);
Value __qmljs_call_value(Context *context, Value thisObject, Value func, Value *args, int argc);

Value __qmljs_construct_activation_property(Context *, String *name, Value *args, int argc);
Value __qmljs_construct_property(Context *context, Value base, String *name, Value *args, int argc);
Value __qmljs_construct_value(Context *context, Value func, Value *args, int argc);

Value __qmljs_builtin_typeof(Value val, Context *context);
void __qmljs_builtin_throw(Value val, Context *context);
Value __qmljs_builtin_rethrow(Context *context);

// constructors
Value __qmljs_init_closure(IR::Function *clos, Context *ctx);
Value __qmljs_init_native_function(void (*code)(Context *), Context *ctx);

Bool __qmljs_is_function(Value value);

// string literals
Value __qmljs_string_literal_undefined(Context *ctx);
Value __qmljs_string_literal_null(Context *ctx);
Value __qmljs_string_literal_true(Context *ctx);
Value __qmljs_string_literal_false(Context *ctx);
Value __qmljs_string_literal_object(Context *ctx);
Value __qmljs_string_literal_boolean(Context *ctx);
Value __qmljs_string_literal_number(Context *ctx);
Value __qmljs_string_literal_string(Context *ctx);
Value __qmljs_string_literal_function(Context *ctx);

// strings
String *__qmljs_string_from_utf8(Context *ctx, const char *s);
int __qmljs_string_length(Context *ctx, String *string);
double __qmljs_string_to_number(Context *ctx, String *string);
Value __qmljs_string_from_number(Context *ctx, double number);
Bool __qmljs_string_compare(Context *ctx, String *left, String *right);
Bool __qmljs_string_equal(Context *ctx, String *left, String *right);
String *__qmljs_string_concat(Context *ctx, String *first, String *second);
String *__qmljs_identifier_from_utf8(Context *ctx, const char *s);

// objects
Value __qmljs_object_default_value(Context *ctx, Value object, int typeHint);
Value __qmljs_throw_type_error(Context *ctx);
Value __qmljs_new_object(Context *ctx);
Value __qmljs_new_boolean_object(Context *ctx, bool boolean);
Value __qmljs_new_number_object(Context *ctx, double n);
Value __qmljs_new_string_object(Context *ctx, String *string);
void __qmljs_set_activation_property(Context *ctx, String *name, Value value);
void __qmljs_set_property(Context *ctx, Value object, String *name, Value value);
Value __qmljs_get_property(Context *ctx, Value object, String *name);
Value __qmljs_get_activation_property(Context *ctx, String *name);

Value __qmljs_get_element(Context *ctx, Value object, Value index);
void __qmljs_set_element(Context *ctx, Value object, Value index, Value value);

// context
Value __qmljs_get_thisObject(Context *ctx);

// type conversion and testing
Value __qmljs_to_primitive(Value value, Context *ctx, int typeHint);
Bool __qmljs_to_boolean(Value value, Context *ctx);
double __qmljs_to_number(Value value, Context *ctx);
double __qmljs_to_integer(Value value, Context *ctx);
int __qmljs_to_int32(Value value, Context *ctx);
unsigned __qmljs_to_uint32(Value value, Context *ctx);
unsigned short __qmljs_to_uint16(Value value, Context *ctx);
Value __qmljs_to_string(Value value, Context *ctx);
Value __qmljs_to_object(Value value, Context *ctx);
//uint __qmljs_check_object_coercible(Context *ctx, Value *result, Value *value);
Bool __qmljs_is_callable(Value value, Context *ctx);
Value __qmljs_default_value(Value value, Context *ctx, int typeHint);

Bool __qmljs_equal(Value x, Value y, Context *ctx);
Bool __qmljs_strict_equal(Value x, Value y, Context *ctx);

// unary operators
Value __qmljs_uplus(Value value, Context *ctx);
Value __qmljs_uminus(Value value, Context *ctx);
Value __qmljs_compl(Value value, Context *ctx);
Value __qmljs_not(Value value, Context *ctx);

/* ### these 4 methods are apparently unused right now */
Value __qmljs_delete_subscript(Context *ctx, Value base, Value index);
Value __qmljs_delete_member(Context *ctx, Value base, String *name);
Value __qmljs_delete_property(Context *ctx, String *name);
Value __qmljs_delete_value(Context *ctx, Value value);

Value __qmljs_typeof(Value value, Context *ctx);
void __qmljs_throw(Value value, Context *context);
Value __qmljs_rethrow(Context *context);

// binary operators
Value __qmljs_instanceof(Value left, Value right, Context *ctx);
Value __qmljs_in(Value left, Value right, Context *ctx);
Value __qmljs_bit_or(Value left, Value right, Context *ctx);
Value __qmljs_bit_xor(Value left, Value right, Context *ctx);
Value __qmljs_bit_and(Value left, Value right, Context *ctx);
Value __qmljs_add(Value left, Value right, Context *ctx);
Value __qmljs_sub(Value left, Value right, Context *ctx);
Value __qmljs_mul(Value left, Value right, Context *ctx);
Value __qmljs_div(Value left, Value right, Context *ctx);
Value __qmljs_mod(Value left, Value right, Context *ctx);
Value __qmljs_shl(Value left, Value right, Context *ctx);
Value __qmljs_shr(Value left, Value right, Context *ctx);
Value __qmljs_ushr(Value left, Value right, Context *ctx);
Value __qmljs_gt(Value left, Value right, Context *ctx);
Value __qmljs_lt(Value left, Value right, Context *ctx);
Value __qmljs_ge(Value left, Value right, Context *ctx);
Value __qmljs_le(Value left, Value right, Context *ctx);
Value __qmljs_eq(Value left, Value right, Context *ctx);
Value __qmljs_ne(Value left, Value right, Context *ctx);
Value __qmljs_se(Value left, Value right, Context *ctx);
Value __qmljs_sne(Value left, Value right, Context *ctx);

Value __qmljs_add_helper(Value left, Value right, Context *ctx);

/*
 unused and probably don't make sense with the new calling convention
void __qmljs_inplace_bit_and(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_bit_or(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_bit_xor(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_add(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_sub(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_mul(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_div(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_mod(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_shl(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_shr(Context *ctx, Value *result, Value *value);
void __qmljs_inplace_ushr(Context *ctx, Value *result, Value *value);
*/

void __qmljs_inplace_bit_and_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_bit_or_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_bit_xor_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_add_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_sub_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_mul_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_div_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_mod_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_shl_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_shr_name(Value value, String *name, Context *ctx);
void __qmljs_inplace_ushr_name(Value value, String *name, Context *ctx);

void __qmljs_inplace_bit_and_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_bit_or_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_bit_xor_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_add_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_sub_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_mul_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_div_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_mod_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_shl_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_shr_element(Value base, Value index, Value value, Context *ctx);
void __qmljs_inplace_ushr_element(Value base, Value index, Value value, Context *ctx);

void __qmljs_inplace_bit_and_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_bit_or_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_bit_xor_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_add_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_sub_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_mul_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_div_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_mod_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_shl_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_shr_member(Value value, Value base, String *name, Context *ctx);
void __qmljs_inplace_ushr_member(Value value, Value base, String *name, Context *ctx);

Bool __qmljs_cmp_gt(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_lt(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_ge(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_le(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_eq(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_ne(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_se(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_sne(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_instanceof(Value left, Value right, Context *ctx);
Bool __qmljs_cmp_in(Value left, Value right, Context *ctx);


} // extern "C"

struct ValueData {
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
            };
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            uint tag;
#endif
        };
    };
};

struct Value;
template <int> struct ValueBase;
template <> struct ValueBase<4> : public ValueData
{
    // we have all 4 bytes on 32 bit to specify the type
    enum Masks {
        NaN_Mask = 0xfff80000,
        Type_Mask = 0xffffffff
    };

    enum ValueType {
        Undefined_Type = NaN_Mask | 0x7ffff, // all 1's
        Null_Type = NaN_Mask | 0x0,
        Boolean_Type = NaN_Mask | 0x1,
        Integer_Type = NaN_Mask | 0x2,
        Object_Type = NaN_Mask | 0x2d59b, // give it randomness to avoid accidental collisions (for gc)
        String_Type = NaN_Mask | 0x2d5ba,
        Double_Type = 0
    };

    inline bool is(ValueType type) const {
        if (type == Double_Type)
            return (tag & NaN_Mask) != NaN_Mask;
        return tag == type;
    }
    inline ValueType type() const {
        return (ValueType)tag;
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
        if (tag == Integer_Type)
            return int_32;
        return dbl;
    }
    int integerValue() const {
        return int_32;
    }

    String *stringValue() const {
        return (String *)(quintptr) uint_32;
    }
    Object *objectValue() const {
        return (Object *)(quintptr) uint_32;
    }
    quint64 rawValue() const {
        return val;
    }

    static inline Value undefinedValue();
    static inline Value nullValue();
    static inline Value fromBoolean(Bool b);
    static inline Value fromDouble(double d);
    static inline Value fromInt32(int i);
    static inline Value fromString(String *s);
    static inline Value fromObject(Object *o);
};

template <> struct ValueBase<8> : public ValueData
{
    enum Masks {
        NaN_Mask = 0x7ff80000,
        Type_Mask = 0x7fff0000,
        Tag_Shift = 32
    };
    enum ValueType {
        Undefined_Type = NaN_Mask | 0x70000,
        Null_Type = NaN_Mask | 0x00000,
        Boolean_Type = NaN_Mask | 0x10000,
        Integer_Type = NaN_Mask | 0x20000,
        Object_Type = NaN_Mask | 0x30000,
        String_Type = NaN_Mask | 0x40000,
        Double_Type = 0
    };

    inline bool is(ValueType type) const {
        if (type == Double_Type)
            return (tag & NaN_Mask) != NaN_Mask;
        return (tag & Type_Mask) == type;
    }
    inline bool isNot(ValueType type) {
        return !is(type);
    }
    inline ValueType type() const {
        return (ValueType)(tag & Type_Mask);
    }

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
        if (tag == Integer_Type)
            return int_32;
        return dbl;
    }
    int integerValue() const {
        return int_32;
    }

    String *stringValue() const {
        return (String *)(val & ~(quint64(Type_Mask) << Tag_Shift));
    }
    Object *objectValue() const {
        return (Object *)(val & ~(quint64(Type_Mask) << Tag_Shift));
    }
    quint64 rawValue() const {
        return val;
    }

    static Value undefinedValue();
    static Value nullValue();
    static Value fromBoolean(Bool b);
    static Value fromDouble(double d);
    static Value fromInt32(int i);
    static Value fromString(String *s);
    static Value fromObject(Object *o);
};


struct Value : public ValueBase<sizeof(void *)>
{

#ifndef QMLJS_LLVM_RUNTIME
    static Value fromString(Context *ctx, const QString &fromString);
    using ValueBase<sizeof(void *)>::fromString;
#endif

    static double toInteger(double fromNumber);
    static int toInt32(double value);
    static unsigned int toUInt32(double value);

    inline int toUInt16(Context *ctx);
    inline int toInt32(Context *ctx);
    inline unsigned int toUInt32(Context *ctx);
    inline Bool toBoolean(Context *ctx) const;
    inline double toInteger(Context *ctx) const;
    double toNumber(Context *ctx) const;
    inline String *toString(Context *ctx) const;
    inline Value toObject(Context *ctx) const;

    inline bool isUndefined() const { return is(Value::Undefined_Type); }
    inline bool isNull() const { return is(Value::Null_Type); }
    inline bool isString() const { return is(Value::String_Type); }
    inline bool isBoolean() const { return type() == Value::Boolean_Type; }
    inline bool isNumber() const { return is(Value::Integer_Type) || is(Value::Double_Type); }
    inline bool isDouble() const { return is(Value::Double_Type); }
    inline bool isInteger() const { return type() == Value::Integer_Type; }
    inline bool isObject() const { return type() == Value::Object_Type; }

    inline bool isPrimitive() const { return type() != Value::Object_Type; }
    bool isFunctionObject() const;
    bool isBooleanObject() const;
    bool isNumberObject() const;
    bool isStringObject() const;
    bool isDateObject() const;
    bool isArrayObject() const;
    bool isErrorObject() const;
    bool isArgumentsObject() const;

    Object *asObject() const;
    FunctionObject *asFunctionObject() const;
    BooleanObject *asBooleanObject() const;
    NumberObject *asNumberObject() const;
    StringObject *asStringObject() const;
    DateObject *asDateObject() const;
    ArrayObject *asArrayObject() const;
    ErrorObject *asErrorObject() const;
    ActivationObject *asArgumentsObject() const;

    Value property(Context *ctx, String *name) const;
    Value *getPropertyDescriptor(Context *ctx, String *name) const;
};

inline int Value::toUInt16(Context *ctx)
{
    return __qmljs_to_uint16(*this, ctx);
}

inline int Value::toInt32(Context *ctx)
{
    return __qmljs_to_int32(*this, ctx);
}

inline unsigned int Value::toUInt32(Context *ctx)
{
    return __qmljs_to_uint32(*this, ctx);
}

inline Bool Value::toBoolean(Context *ctx) const
{
    return __qmljs_to_boolean(*this, ctx);
}

inline double Value::toInteger(Context *ctx) const
{
    return __qmljs_to_integer(*this, ctx);
}

inline double Value::toNumber(Context *ctx) const
{
    return __qmljs_to_number(*this, ctx);
}

inline String *Value::toString(Context *ctx) const
{
    Value v = __qmljs_to_string(*this, ctx);
    assert(v.is(Value::String_Type));
    return v.stringValue();
}

inline Value Value::toObject(Context *ctx) const
{
    return __qmljs_to_object(*this, ctx);
}


inline Value ValueBase<4>::undefinedValue()
{
    Value v;
    v.tag = Undefined_Type;
    v.uint_32 = 0;
    return v;
}

inline Value ValueBase<4>::nullValue()
{
    Value v;
    v.tag = Null_Type;
    v.uint_32 = 0;
    return v;
}

inline Value ValueBase<4>::fromBoolean(Bool b)
{
    Value v;
    v.tag = Boolean_Type;
    v.int_32 = (bool)b;
    return v;
}

inline Value ValueBase<4>::fromDouble(double d)
{
    Value v;
    v.dbl = d;
    return v;
}

inline Value ValueBase<4>::fromInt32(int i)
{
    Value v;
    v.tag = Integer_Type;
    v.int_32 = i;
    return v;
}

inline Value ValueBase<4>::fromString(String *s)
{
    Value v;
    v.tag = String_Type;
    v.uint_32 = (quint32)(quintptr) s;
    return v;
}

inline Value ValueBase<4>::fromObject(Object *o)
{
    Value v;
    v.tag = Object_Type;
    v.uint_32 = (quint32)(quintptr) o;
    return v;
}


inline Value ValueBase<8>::undefinedValue()
{
    Value v;
    v.val = quint64(Undefined_Type) << Tag_Shift;
    return v;
}

inline Value ValueBase<8>::nullValue()
{
    Value v;
    v.val = quint64(Null_Type) << Tag_Shift;
    return v;
}

inline Value ValueBase<8>::fromBoolean(Bool b)
{
    Value v;
    v.tag = Boolean_Type;
    v.int_32 = (bool)b;
    return v;
}

inline Value ValueBase<8>::fromDouble(double d)
{
    Value v;
    v.dbl = d;
    return v;
}

inline Value ValueBase<8>::fromInt32(int i)
{
    Value v;
    v.tag = Integer_Type;
    v.int_32 = i;
    return v;
}

inline Value ValueBase<8>::fromString(String *s)
{
    Value v;
    v.val = (quint64)s;
    v.val |= quint64(String_Type) << Tag_Shift;
    return v;
}

inline Value ValueBase<8>::fromObject(Object *o)
{
    Value v;
    v.val = (quint64)o;
    v.val |= quint64(Object_Type) << Tag_Shift;
    return v;
}

#include <qmljs_math.h>

struct Context {
    ExecutionEngine *engine;
    Context *parent;
    Value activation;
    Value thisObject;
    Value *arguments;
    unsigned int argumentCount;
    Value *locals;
    Value result;
    String **formals;
    unsigned int formalCount;
    String **vars;
    unsigned int varCount;
    int calledAsConstructor;
    int hasUncaughtException;

    Value *lookupPropertyDescriptor(String *name);

    inline Value argument(unsigned int index = 0)
    {
        Value arg;
        getArgument(&arg, index);
        return arg;
    }

    inline void getArgument(Value *result, unsigned int index)
    {
        if (index < argumentCount)
            *result = arguments[index];
        else
            *result = Value::undefinedValue();
    }

    void init(ExecutionEngine *eng);

    void throwError(Value value);
    void throwTypeError();
    void throwReferenceError(Value value);

#ifndef QMLJS_LLVM_RUNTIME
    void throwError(const QString &message);
    void throwUnimplemented(const QString &message);
#endif

    void initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, unsigned argc);
    void leaveCallContext(FunctionObject *f);

    void initConstructorContext(ExecutionEngine *e, Value *object, FunctionObject *f, Value *args, unsigned argc);
    void leaveConstructorContext(FunctionObject *f);
};



extern "C" {

// type conversion and testing
inline Value __qmljs_to_primitive(Value value, Context *ctx, int typeHint)
{
    if (!value.isObject())
        return value;
    return __qmljs_default_value(value, ctx, typeHint);
}

inline Bool __qmljs_to_boolean(Value value, Context *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return false;
    case Value::Boolean_Type:
    case Value::Integer_Type:
        return (bool)value.int_32;
    case Value::String_Type:
        return __qmljs_string_length(ctx, value.stringValue()) > 0;
    case Value::Object_Type:
        return true;
    default: // double
        if (! value.doubleValue() || std::isnan(value.doubleValue()))
            return false;
        return true;
    }
}

inline double __qmljs_to_number(Value value, Context *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return nan("");
    case Value::Null_Type:
        return 0;
    case Value::Boolean_Type:
        return (value.booleanValue() ? 1. : 0.);
    case Value::Integer_Type:
        return value.int_32;
    case Value::String_Type:
        return __qmljs_string_to_number(ctx, value.stringValue());
    case Value::Object_Type: {
        Value prim = __qmljs_to_primitive(value, ctx, NUMBER_HINT);
        return __qmljs_to_number(prim, ctx);
    }
    default: // double
        return value.doubleValue();
    }
}

inline double __qmljs_to_integer(Value value, Context *ctx)
{
    if (value.isInteger())
        return value.int_32;

    return Value::toInteger(__qmljs_to_number(value, ctx));
}

inline int __qmljs_to_int32(Value value, Context *ctx)
{
    if (value.isInteger())
        return value.int_32;

    return Value::toInt32(__qmljs_to_number(value, ctx));
}

inline unsigned __qmljs_to_uint32(Value value, Context *ctx)
{
    if (value.isInteger())
        return (unsigned) value.int_32;

    return Value::toUInt32(__qmljs_to_number(value, ctx));
}

inline unsigned short __qmljs_to_uint16(Value value, Context *ctx)
{
    if (value.isInteger())
        return (ushort)(uint)value.integerValue();

    double number = __qmljs_to_number(value, ctx);

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

inline Value __qmljs_to_string(Value value, Context *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return __qmljs_string_literal_undefined(ctx);
        break;
    case Value::Null_Type:
        return __qmljs_string_literal_null(ctx);
        break;
    case Value::Boolean_Type:
        if (value.booleanValue())
            return __qmljs_string_literal_true(ctx);
        else
            return __qmljs_string_literal_false(ctx);
        break;
    case Value::String_Type:
        return value;
        break;
    case Value::Object_Type: {
        Value prim = __qmljs_to_primitive(value, ctx, STRING_HINT);
        if (prim.isPrimitive())
            return __qmljs_to_string(prim, ctx);
        else
            return __qmljs_throw_type_error(ctx);
        break;
    }
    case Value::Integer_Type:
        return __qmljs_string_from_number(ctx, value.int_32);
        break;
    default: // double
        return __qmljs_string_from_number(ctx, value.doubleValue());
        break;

    } // switch
}

inline Value __qmljs_to_object(Value value, Context *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return __qmljs_throw_type_error(ctx);
        break;
    case Value::Boolean_Type:
        return __qmljs_new_boolean_object(ctx, value.booleanValue());
        break;
    case Value::String_Type:
        return __qmljs_new_string_object(ctx, value.stringValue());
        break;
    case Value::Object_Type:
        return value;
        break;
    case Value::Integer_Type:
        return __qmljs_new_number_object(ctx, value.int_32);
        break;
    default: // double
        return __qmljs_new_number_object(ctx, value.doubleValue());
        break;
    }
}

/*
inline uint __qmljs_check_object_coercible(Context *ctx, Value *result, Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        *result = __qmljs_throw_type_error(ctx);
        return false;
    default:
        return true;
    }
}
*/

inline Bool __qmljs_is_callable(Value value, Context * /*ctx*/)
{
    if (value.isObject())
        return __qmljs_is_function(value);
    else
        return false;
}

inline Value __qmljs_default_value(Value value, Context *ctx, int typeHint)
{
    if (value.isObject())
        return __qmljs_object_default_value(ctx, value, typeHint);
    return Value::undefinedValue();
}


// unary operators
inline Value __qmljs_typeof(Value value, Context *ctx)
{
    switch (value.type()) {
    case Value::Undefined_Type:
        return __qmljs_string_literal_undefined(ctx);
        break;
    case Value::Null_Type:
        return __qmljs_string_literal_object(ctx);
        break;
    case Value::Boolean_Type:
        return __qmljs_string_literal_boolean(ctx);
        break;
    case Value::String_Type:
        return __qmljs_string_literal_string(ctx);
        break;
    case Value::Object_Type:
        if (__qmljs_is_callable(value, ctx))
            return __qmljs_string_literal_function(ctx);
        else
            return __qmljs_string_literal_object(ctx); // ### implementation-defined
        break;
    default:
        return __qmljs_string_literal_number(ctx);
        break;
    }
}

inline Value __qmljs_uplus(Value value, Context *ctx)
{
    if (value.isInteger())
        return value;
    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(n);
}

inline Value __qmljs_uminus(Value value, Context *ctx)
{
    if (value.isInteger())
        return Value::fromInt32(-value.integerValue());
    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(-n);
}

inline Value __qmljs_compl(Value value, Context *ctx)
{
    int n = __qmljs_to_int32(value, ctx);
    return Value::fromInt32(~n);
}

inline Value __qmljs_not(Value value, Context *ctx)
{
    bool b = __qmljs_to_boolean(value, ctx);
    return Value::fromBoolean(!b);
}

// binary operators
inline Value __qmljs_bit_or(Value left, Value right, Context *ctx)
{
    int lval = __qmljs_to_int32(left, ctx);
    int rval = __qmljs_to_int32(right, ctx);
    return Value::fromInt32(lval | rval);
}

inline Value __qmljs_bit_xor(Value left, Value right, Context *ctx)
{
    int lval = __qmljs_to_int32(left, ctx);
    int rval = __qmljs_to_int32(right, ctx);
    return Value::fromInt32(lval ^ rval);
}

inline Value __qmljs_bit_and(Value left, Value right, Context *ctx)
{
    int lval = __qmljs_to_int32(left, ctx);
    int rval = __qmljs_to_int32(right, ctx);
    return Value::fromInt32(lval & rval);
}

/*
inline void __qmljs_inplace_bit_and(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_bit_and(*result, *value, ctx);
}

inline void __qmljs_inplace_bit_or(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_bit_or(*result, *value, ctx);
}

inline void __qmljs_inplace_bit_xor(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_bit_xor(*result, *value, ctx);
}

inline void __qmljs_inplace_add(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_add(*result, *value, ctx);
}

inline void __qmljs_inplace_sub(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_sub(*result, *value, ctx);
}

inline void __qmljs_inplace_mul(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_mul(*result, *value, ctx);
}

inline void __qmljs_inplace_div(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_div(*result, *value, ctx);
}

inline void __qmljs_inplace_mod(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_mod(*result, *value, ctx);
}

inline void __qmljs_inplace_shl(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_shl(*result, *value, ctx);
}

inline void __qmljs_inplace_shr(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_shr(*result, *value, ctx);
}

inline void __qmljs_inplace_ushr(Context *ctx, Value *result, Value *value)
{
    *result = __qmljs_ushr(*result, *value, ctx);
}
*/

inline Value __qmljs_add(Value left, Value right, Context *ctx)
{
    if (left.isInteger() & right.isInteger())
        return add_int32(left.integerValue(), right.integerValue());

    if (left.isNumber() & right.isNumber())
        return Value::fromDouble(left.asDouble() + right.asDouble());
    else
        return __qmljs_add_helper(left, right, ctx);
}

inline Value __qmljs_sub(Value left, Value right, Context *ctx)
{
    if (left.isInteger() && right.isInteger())
        return sub_int32(left.integerValue(), right.integerValue());

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval - rval);
}

inline Value __qmljs_mul(Value left, Value right, Context *ctx)
{
    if (left.isInteger() && right.isInteger())
        return mul_int32(left.integerValue(), right.integerValue());

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval * rval);
}

inline Value __qmljs_div(Value left, Value right, Context *ctx)
{
    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval / rval);
}

inline Value __qmljs_mod(Value left, Value right, Context *ctx)
{
    if (left.isInteger() && right.isInteger())
        return Value::fromInt32(left.integerValue() % right.integerValue());

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(fmod(lval, rval));
}

// ### unsigned shl missing?

inline Value __qmljs_shl(Value left, Value right, Context *ctx)
{
    int lval = __qmljs_to_int32(left, ctx);
    unsigned rval = __qmljs_to_uint32(right, ctx) & 0x1f;
    return Value::fromInt32(lval << rval);
}

inline Value __qmljs_shr(Value left, Value right, Context *ctx)
{
    int lval = __qmljs_to_int32(left, ctx);
    unsigned rval = __qmljs_to_uint32(right, ctx) & 0x1f;
    return Value::fromInt32(lval >> rval);
}

inline Value __qmljs_ushr(Value left, Value right, Context *ctx)
{
    unsigned lval = __qmljs_to_uint32(left, ctx);
    unsigned rval = __qmljs_to_uint32(right, ctx) & 0x1f;
    return Value::fromInt32(lval >> rval);
}

inline Value __qmljs_gt(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(__qmljs_cmp_gt(left, right, ctx));
}

inline Value __qmljs_lt(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(__qmljs_cmp_lt(left, right, ctx));
}

inline Value __qmljs_ge(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(__qmljs_cmp_ge(left, right, ctx));
}

inline Value __qmljs_le(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(__qmljs_cmp_le(left, right, ctx));
}

inline Value __qmljs_eq(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_ne(Value left, Value right, Context *ctx)
{
    return Value::fromBoolean(!__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_se(Value left, Value right, Context *ctx)
{
    bool r = __qmljs_strict_equal(left, right, ctx);
    return Value::fromBoolean(r);
}

inline Value __qmljs_sne(Value left, Value right, Context *ctx)
{
    bool r = ! __qmljs_strict_equal(left, right, ctx);
    return Value::fromBoolean(r);
}

inline Bool __qmljs_cmp_gt(Value left, Value right, Context *ctx)
{
    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (left.isInteger() && right.isInteger())
        return left.integerValue() > right.integerValue();
    if (left.isNumber() && right.isNumber()) {
        return left.asDouble() > right.asDouble();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l > r;
    }
}

inline Bool __qmljs_cmp_lt(Value left, Value right, Context *ctx)
{
    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (left.isInteger() && right.isInteger())
        return left.integerValue() < right.integerValue();
    if (left.isNumber() && right.isNumber()) {
        return left.asDouble() < right.asDouble();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l < r;
    }
}

inline Bool __qmljs_cmp_ge(Value left, Value right, Context *ctx)
{
    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (left.isInteger() && right.isInteger())
        return left.integerValue() >= right.integerValue();
    if (left.isNumber() && right.isNumber()) {
        return left.asDouble() >= right.asDouble();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l >= r;
    }
}

inline Bool __qmljs_cmp_le(Value left, Value right, Context *ctx)
{
    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (left.isInteger() && right.isInteger())
        return left.integerValue() <= right.integerValue();
    if (left.isNumber() && right.isNumber()) {
        return left.asDouble() <= right.asDouble();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l <= r;
    }
}

inline Bool __qmljs_cmp_eq(Value left, Value right, Context *ctx)
{
    // need to test for doubles first as NaN != NaN
    if (left.isDouble() && right.isDouble())
        return left.doubleValue() == right.doubleValue();
    if (left.val == right.val)
        return true;
    if (left.isString() && right.isString())
        return __qmljs_string_equal(ctx, left.stringValue(), right.stringValue());

    return __qmljs_equal(left, right, ctx);
}

inline Bool __qmljs_cmp_ne(Value left, Value right, Context *ctx)
{
    return !__qmljs_cmp_eq(left, right, ctx);
}

inline Bool __qmljs_cmp_se(Value left, Value right, Context *ctx)
{
    return __qmljs_strict_equal(left, right, ctx);
}

inline Bool __qmljs_cmp_sne(Value left, Value right, Context *ctx)
{
    return ! __qmljs_strict_equal(left, right, ctx);
}

inline Bool __qmljs_cmp_instanceof(Value left, Value right, Context *ctx)
{
    Value v = __qmljs_instanceof(left, right, ctx);
    return v.booleanValue();
}

inline uint __qmljs_cmp_in(Value left, Value right, Context *ctx)
{
    Value v = __qmljs_in(left, right, ctx);
    return v.booleanValue();
}

inline Bool __qmljs_strict_equal(Value x, Value y, Context *ctx)
{
    if (x.rawValue() == y.rawValue())
        return true;
    if (x.isString() && y.isString())
        return __qmljs_string_equal(ctx, x.stringValue(), y.stringValue());
    return false;
}

} // extern "C"

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_RUNTIME_H
