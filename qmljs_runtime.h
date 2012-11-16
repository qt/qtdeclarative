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

#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>

#include <wtf/Platform.h>

#include <cmath>
#include <cassert>

#ifdef DO_TRACE_INSTR
#  define TRACE1(x) fprintf(stderr, "    %s\n", __FUNCTION__);
#  define TRACE2(x, y) fprintf(stderr, "    %s\n", __FUNCTION__);
#else
#  define TRACE1(x)
#  define TRACE2(x, y)
#endif // TRACE1

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

struct Object;
struct String;
struct PropertyDescriptor;
struct ExecutionContext;
struct FunctionObject;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct DateObject;
struct RegExpObject;
struct ArrayObject;
struct ErrorObject;
struct ActivationObject;
struct ExecutionEngine;

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
        NotDouble_Mask = 0xfff80000,
        Type_Mask = 0xffff0000,
        Immediate_Mask = NotDouble_Mask | 0x00040000,
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
        ConvertibleToInt = Immediate_Mask | (0x1 << 15)
    };

    enum ValueTypeInternal {
        _Undefined_Type = Undefined_Type,
        _Null_Type = Null_Type | ConvertibleToInt,
        _Boolean_Type = Boolean_Type | ConvertibleToInt,
        _Integer_Type = Integer_Type | ConvertibleToInt,
        _Object_Type = Object_Type,
        _String_Type = String_Type

    };

    inline ValueType type() const {
        return (ValueType)(tag & Type_Mask);
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
#else
    String *stringValue() const {
        return s;
    }
    Object *objectValue() const {
        return o;
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
    static Value fromString(String *s);
    static Value fromObject(Object *o);

#ifndef QMLJS_LLVM_RUNTIME
    static Value fromString(ExecutionContext *ctx, const QString &fromString);
#endif

    static double toInteger(double fromNumber);
    static int toInt32(double value);
    static unsigned int toUInt32(double value);

    inline int toUInt16(ExecutionContext *ctx);
    inline int toInt32(ExecutionContext *ctx);
    inline unsigned int toUInt32(ExecutionContext *ctx);
    inline Bool toBoolean(ExecutionContext *ctx) const;
    inline double toInteger(ExecutionContext *ctx) const;
    double toNumber(ExecutionContext *ctx) const;
    inline String *toString(ExecutionContext *ctx) const;
    inline Value toObject(ExecutionContext *ctx) const;

    inline bool isPrimitive() const { return !isObject(); }
#if CPU(X86_64)
    static inline bool integerCompatible(Value a, Value b) {
        const quint64 mask = quint64(ConvertibleToInt) << 32;
        return ((a.val & b.val) & mask) == mask;
    }
    static inline bool bothDouble(Value a, Value b) {
        const quint64 mask = quint64(NotDouble_Mask) << 32;
        return ((a.val | b.val) & mask) != mask;
    }
#else
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
    ActivationObject *asArgumentsObject() const;

    Value property(ExecutionContext *ctx, String *name) const;

    // Section 9.12
    bool sameValue(Value other);
};

extern "C" {

// context
Value __qmljs_call_activation_property(ExecutionContext *, String *name, Value *args, int argc);
Value __qmljs_call_property(ExecutionContext *context, Value base, String *name, Value *args, int argc);
Value __qmljs_call_value(ExecutionContext *context, Value thisObject, Value func, Value *args, int argc);

Value __qmljs_construct_activation_property(ExecutionContext *, String *name, Value *args, int argc);
Value __qmljs_construct_property(ExecutionContext *context, Value base, String *name, Value *args, int argc);
Value __qmljs_construct_value(ExecutionContext *context, Value func, Value *args, int argc);

Value __qmljs_builtin_typeof(Value val, ExecutionContext *context);
void __qmljs_builtin_throw(Value val, ExecutionContext *context);

// constructors
Value __qmljs_init_closure(IR::Function *clos, ExecutionContext *ctx);
Value __qmljs_init_native_function(void (*code)(ExecutionContext *), ExecutionContext *ctx);

Bool __qmljs_is_function(Value value);

// string literals
Value __qmljs_string_literal_undefined(ExecutionContext *ctx);
Value __qmljs_string_literal_null(ExecutionContext *ctx);
Value __qmljs_string_literal_true(ExecutionContext *ctx);
Value __qmljs_string_literal_false(ExecutionContext *ctx);
Value __qmljs_string_literal_object(ExecutionContext *ctx);
Value __qmljs_string_literal_boolean(ExecutionContext *ctx);
Value __qmljs_string_literal_number(ExecutionContext *ctx);
Value __qmljs_string_literal_string(ExecutionContext *ctx);
Value __qmljs_string_literal_function(ExecutionContext *ctx);

// strings
String *__qmljs_string_from_utf8(ExecutionContext *ctx, const char *s);
int __qmljs_string_length(ExecutionContext *ctx, String *string);
double __qmljs_string_to_number(ExecutionContext *ctx, String *string);
Value __qmljs_string_from_number(ExecutionContext *ctx, double number);
Bool __qmljs_string_compare(ExecutionContext *ctx, String *left, String *right);
Bool __qmljs_string_equal(String *left, String *right);
String *__qmljs_string_concat(ExecutionContext *ctx, String *first, String *second);
String *__qmljs_identifier_from_utf8(ExecutionContext *ctx, const char *s);

// objects
Value __qmljs_object_default_value(ExecutionContext *ctx, Value object, int typeHint);
Value __qmljs_throw_type_error(ExecutionContext *ctx);
Value __qmljs_new_object(ExecutionContext *ctx);
Value __qmljs_new_boolean_object(ExecutionContext *ctx, bool boolean);
Value __qmljs_new_number_object(ExecutionContext *ctx, double n);
Value __qmljs_new_string_object(ExecutionContext *ctx, String *string);
void __qmljs_set_activation_property(ExecutionContext *ctx, String *name, Value value);
void __qmljs_set_property(ExecutionContext *ctx, Value object, String *name, Value value);
Value __qmljs_get_property(ExecutionContext *ctx, Value object, String *name);
Value __qmljs_get_activation_property(ExecutionContext *ctx, String *name);

Value __qmljs_get_element(ExecutionContext *ctx, Value object, Value index);
void __qmljs_set_element(ExecutionContext *ctx, Value object, Value index, Value value);

// For each
Value __qmljs_foreach_iterator_object(Value in, ExecutionContext *ctx);
Value __qmljs_foreach_next_property_name(Value foreach_iterator);

// context
Value __qmljs_get_thisObject(ExecutionContext *ctx);

// type conversion and testing
Value __qmljs_to_primitive(Value value, ExecutionContext *ctx, int typeHint);
Bool __qmljs_to_boolean(Value value, ExecutionContext *ctx);
double __qmljs_to_number(Value value, ExecutionContext *ctx);
double __qmljs_to_integer(Value value, ExecutionContext *ctx);
int __qmljs_to_int32(Value value, ExecutionContext *ctx);
unsigned short __qmljs_to_uint16(Value value, ExecutionContext *ctx);
Value __qmljs_to_string(Value value, ExecutionContext *ctx);
Value __qmljs_to_object(Value value, ExecutionContext *ctx);
//uint __qmljs_check_object_coercible(Context *ctx, Value *result, Value *value);
Bool __qmljs_is_callable(Value value, ExecutionContext *ctx);
Value __qmljs_default_value(Value value, ExecutionContext *ctx, int typeHint);

Bool __qmljs_equal(Value x, Value y, ExecutionContext *ctx);
Bool __qmljs_strict_equal(Value x, Value y);

// unary operators
Value __qmljs_uplus(Value value, ExecutionContext *ctx);
Value __qmljs_uminus(Value value, ExecutionContext *ctx);
Value __qmljs_compl(Value value, ExecutionContext *ctx);
Value __qmljs_not(Value value, ExecutionContext *ctx);

Value __qmljs_delete_subscript(ExecutionContext *ctx, Value base, Value index);
Value __qmljs_delete_member(ExecutionContext *ctx, Value base, String *name);
Value __qmljs_delete_property(ExecutionContext *ctx, String *name);
Value __qmljs_delete_value(ExecutionContext *ctx, Value value);

Value __qmljs_typeof(Value value, ExecutionContext *ctx);
void __qmljs_throw(Value value, ExecutionContext *context);
// actually returns a jmp_buf *
void *__qmljs_create_exception_handler(ExecutionContext *context);
void __qmljs_delete_exception_handler(ExecutionContext *context);
Value __qmljs_get_exception(ExecutionContext *context);

// binary operators
typedef Value (*BinOp)(Value left, Value right, ExecutionContext *ctx);

Value __qmljs_instanceof(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_in(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_or(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_xor(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_bit_and(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_add(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_sub(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_mul(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_div(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_mod(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_shl(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_shr(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ushr(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_gt(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_lt(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ge(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_le(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_eq(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_ne(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_se(Value left, Value right, ExecutionContext *ctx);
Value __qmljs_sne(Value left, Value right, ExecutionContext *ctx);

Value __qmljs_add_helper(Value left, Value right, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_add_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_sub_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mul_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_div_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mod_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shl_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shr_name(Value value, String *name, ExecutionContext *ctx);
void __qmljs_inplace_ushr_name(Value value, String *name, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_add_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_sub_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_mul_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_div_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_mod_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_shl_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_shr_element(Value base, Value index, Value value, ExecutionContext *ctx);
void __qmljs_inplace_ushr_element(Value base, Value index, Value value, ExecutionContext *ctx);

void __qmljs_inplace_bit_and_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_or_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_bit_xor_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_add_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_sub_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mul_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_div_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_mod_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shl_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_shr_member(Value value, Value base, String *name, ExecutionContext *ctx);
void __qmljs_inplace_ushr_member(Value value, Value base, String *name, ExecutionContext *ctx);

Bool __qmljs_cmp_gt(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_lt(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_ge(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_le(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_eq(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_ne(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_se(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_sne(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_instanceof(Value left, Value right, ExecutionContext *ctx);
Bool __qmljs_cmp_in(Value left, Value right, ExecutionContext *ctx);

} // extern "C"

inline int Value::toUInt16(ExecutionContext *ctx)
{
    return __qmljs_to_uint16(*this, ctx);
}

inline int Value::toInt32(ExecutionContext *ctx)
{
    if (isConvertibleToInt())
        return int_32;

    return Value::toInt32(__qmljs_to_number(*this, ctx));
}

inline unsigned int Value::toUInt32(ExecutionContext *ctx)
{
    if (isConvertibleToInt())
        return (unsigned) int_32;

    return toUInt32(__qmljs_to_number(*this, ctx));
}

inline Bool Value::toBoolean(ExecutionContext *ctx) const
{
    return __qmljs_to_boolean(*this, ctx);
}

inline double Value::toInteger(ExecutionContext *ctx) const
{
    return __qmljs_to_integer(*this, ctx);
}

inline double Value::toNumber(ExecutionContext *ctx) const
{
    return __qmljs_to_number(*this, ctx);
}

inline String *Value::toString(ExecutionContext *ctx) const
{
    Value v = __qmljs_to_string(*this, ctx);
    assert(v.isString());
    return v.stringValue();
}

inline Value Value::toObject(ExecutionContext *ctx) const
{
    return __qmljs_to_object(*this, ctx);
}

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

inline bool Value::sameValue(Value other) {
    if (val == other.val)
        return true;
    if (isString() && other.isString())
        return __qmljs_string_equal(stringValue(), other.stringValue());
    return false;
}

#include <qmljs_math.h>

struct ExecutionContext {
    ExecutionEngine *engine;
    ExecutionContext *parent;
    Object *activation;
    Value thisObject;
    Value *arguments;
    unsigned int argumentCount;
    Value *locals;
    Value result;
    String **formals;
    unsigned int formalCount;
    String **vars;
    unsigned int varCount;

    PropertyDescriptor *lookupPropertyDescriptor(String *name, PropertyDescriptor *tmp);
    void inplaceBitOp(Value value, String *name, BinOp op);

    inline Value argument(unsigned int index = 0)
    {
        if (index < argumentCount)
            return arguments[index];
        return Value::undefinedValue();
    }

    void init(ExecutionEngine *eng);

    void initCallContext(ExecutionContext *parent, const Value that, FunctionObject *f, Value *args, unsigned argc);
    void leaveCallContext();

    void initConstructorContext(ExecutionContext *parent, Value that, FunctionObject *f, Value *args, unsigned argc);
    void leaveConstructorContext(FunctionObject *f);
    void wireUpPrototype(FunctionObject *f);

    void throwError(Value value);
    void throwError(const QString &message);
    void throwTypeError();
    void throwReferenceError(Value value);
    void throwUnimplemented(const QString &message);
};



extern "C" {

// type conversion and testing
inline Value __qmljs_to_primitive(Value value, ExecutionContext *ctx, int typeHint)
{
    if (!value.isObject())
        return value;
    return __qmljs_default_value(value, ctx, typeHint);
}

inline Bool __qmljs_to_boolean(Value value, ExecutionContext *ctx)
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

inline double __qmljs_to_number(Value value, ExecutionContext *ctx)
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

inline double __qmljs_to_integer(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
        return value.int_32;

    return Value::toInteger(__qmljs_to_number(value, ctx));
}

inline int __qmljs_to_int32(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
        return value.int_32;

    return Value::toInt32(__qmljs_to_number(value, ctx));
}

inline unsigned short __qmljs_to_uint16(Value value, ExecutionContext *ctx)
{
    if (value.isConvertibleToInt())
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

inline Value __qmljs_to_string(Value value, ExecutionContext *ctx)
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

inline Value __qmljs_to_object(Value value, ExecutionContext *ctx)
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

inline Bool __qmljs_is_callable(Value value, ExecutionContext * /*ctx*/)
{
    if (value.isObject())
        return __qmljs_is_function(value);
    else
        return false;
}

inline Value __qmljs_default_value(Value value, ExecutionContext *ctx, int typeHint)
{
    if (value.isObject())
        return __qmljs_object_default_value(ctx, value, typeHint);
    return Value::undefinedValue();
}


// unary operators
inline Value __qmljs_typeof(Value value, ExecutionContext *ctx)
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

inline Value __qmljs_uplus(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    if (value.tryIntegerConversion())
        return value;

    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(n);
}

inline Value __qmljs_uminus(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    if (value.isInteger())
        return Value::fromInt32(-value.integerValue());
    double n = __qmljs_to_number(value, ctx);
    return Value::fromDouble(-n);
}

inline Value __qmljs_compl(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    int n;
    if (value.isConvertibleToInt())
        n = ~value.int_32;
    else
        n = Value::toInteger(__qmljs_to_number(value, ctx));

    return Value::fromInt32(~n);
}

inline Value __qmljs_not(Value value, ExecutionContext *ctx)
{
    TRACE1(value);

    bool b = __qmljs_to_boolean(value, ctx);
    return Value::fromBoolean(!b);
}

// binary operators
inline Value __qmljs_bit_or(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() | right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval | rval);
}

inline Value __qmljs_bit_xor(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() ^ right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval ^ rval);
}

inline Value __qmljs_bit_and(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() & right.integerValue());

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    int rval = Value::toInt32(__qmljs_to_number(right, ctx));
    return Value::fromInt32(lval & rval);
}

inline Value __qmljs_add(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return add_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    if (Value::bothDouble(left, right))
        return Value::fromDouble(left.doubleValue() + right.doubleValue());

    return __qmljs_add_helper(left, right, ctx);
}

inline Value __qmljs_sub(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return sub_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval - rval);
}

inline Value __qmljs_mul(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

#ifndef QMLJS_LLVM_RUNTIME
    if (Value::integerCompatible(left, right))
        return mul_int32(left.integerValue(), right.integerValue());
#endif // QMLJS_LLVM_RUNTIME

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval * rval);
}

inline Value __qmljs_div(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(lval / rval);
}

inline Value __qmljs_mod(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() % right.integerValue());

    double lval = __qmljs_to_number(left, ctx);
    double rval = __qmljs_to_number(right, ctx);
    return Value::fromDouble(fmod(lval, rval));
}

// ### unsigned shl missing?

inline Value __qmljs_shl(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() << ((uint(right.integerValue()) & 0x1f)));

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    return Value::fromInt32(lval << rval);
}

inline Value __qmljs_shr(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(left.integerValue() >> ((uint(right.integerValue()) & 0x1f)));

    int lval = Value::toInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    return Value::fromInt32(lval >> rval);
}

inline Value __qmljs_ushr(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    if (Value::integerCompatible(left, right))
        return Value::fromInt32(uint(left.integerValue()) >> ((uint(right.integerValue()) & 0x1f)));

    unsigned lval = Value::toUInt32(__qmljs_to_number(left, ctx));
    unsigned rval = Value::toUInt32(__qmljs_to_number(right, ctx)) & 0x1f;
    return Value::fromInt32(lval >> rval);
}

inline Value __qmljs_gt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_gt(left, right, ctx));
}

inline Value __qmljs_lt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_lt(left, right, ctx));
}

inline Value __qmljs_ge(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_ge(left, right, ctx));
}

inline Value __qmljs_le(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_le(left, right, ctx));
}

inline Value __qmljs_eq(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_ne(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return Value::fromBoolean(!__qmljs_cmp_eq(left, right, ctx));
}

inline Value __qmljs_se(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    bool r = __qmljs_strict_equal(left, right);
    return Value::fromBoolean(r);
}

inline Value __qmljs_sne(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    bool r = ! __qmljs_strict_equal(left, right);
    return Value::fromBoolean(r);
}

inline Bool __qmljs_cmp_gt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() > right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() > right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l > r;
    }
}

inline Bool __qmljs_cmp_lt(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() < right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() < right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return __qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l < r;
    }
}

inline Bool __qmljs_cmp_ge(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() >= right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() >= right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, left.stringValue(), right.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l >= r;
    }
}

inline Bool __qmljs_cmp_le(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    left = __qmljs_to_primitive(left, ctx, NUMBER_HINT);
    right = __qmljs_to_primitive(right, ctx, NUMBER_HINT);

    if (Value::integerCompatible(left, right))
        return left.integerValue() <= right.integerValue();
    if (Value::bothDouble(left, right)) {
        return left.doubleValue() <= right.doubleValue();
    } else if (left.isString() && right.isString()) {
        return !__qmljs_string_compare(ctx, right.stringValue(), left.stringValue());
    } else {
        double l = __qmljs_to_number(left, ctx);
        double r = __qmljs_to_number(right, ctx);
        return l <= r;
    }
}

inline Bool __qmljs_cmp_eq(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    // need to test for doubles first as NaN != NaN
    if (Value::bothDouble(left, right))
        return left.doubleValue() == right.doubleValue();
    if (left.val == right.val)
        return true;
    if (left.isString() && right.isString())
        return __qmljs_string_equal(left.stringValue(), right.stringValue());

    return __qmljs_equal(left, right, ctx);
}

inline Bool __qmljs_cmp_ne(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    return !__qmljs_cmp_eq(left, right, ctx);
}

inline Bool __qmljs_cmp_se(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    return __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_sne(Value left, Value right, ExecutionContext *)
{
    TRACE2(left, right);

    return ! __qmljs_strict_equal(left, right);
}

inline Bool __qmljs_cmp_instanceof(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    Value v = __qmljs_instanceof(left, right, ctx);
    return v.booleanValue();
}

inline uint __qmljs_cmp_in(Value left, Value right, ExecutionContext *ctx)
{
    TRACE2(left, right);

    Value v = __qmljs_in(left, right, ctx);
    return v.booleanValue();
}

inline Bool __qmljs_strict_equal(Value x, Value y)
{
    TRACE2(x, y);

    if (x.isDouble() || y.isDouble())
        return x.asDouble() == y.asDouble();
    if (x.rawValue() == y.rawValue())
        return true;
    if (x.isString() && y.isString())
        return __qmljs_string_equal(x.stringValue(), y.stringValue());
    return false;
}

} // extern "C"

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_RUNTIME_H
