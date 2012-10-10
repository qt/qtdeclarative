#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#ifndef QMLJS_LLVM_RUNTIME
#  include <QtCore/QString>
#  include <QtCore/QDebug>
#endif

#include <math.h>
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

extern "C" {

// context
void __qmljs_call_activation_property(Context *, Value *result, String *name, Value *args, int argc);
void __qmljs_call_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc);
void __qmljs_call_value(Context *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc);

void __qmljs_construct_activation_property(Context *, Value *result, String *name, Value *args, int argc);
void __qmljs_construct_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc);
void __qmljs_construct_value(Context *context, Value *result, const Value *func, Value *args, int argc);

void __qmljs_builtin_typeof(Context *context, Value *result, Value *args, int argc);
void __qmljs_builtin_throw(Context *context, Value *result, Value *args, int argc);
void __qmljs_builtin_rethrow(Context *context, Value *result, Value *args, int argc);

// constructors
void __qmljs_init_undefined(Value *result);
void __qmljs_init_null(Value *result);
void __qmljs_init_boolean(Value *result, bool value);
void __qmljs_init_number(Value *result, double number);
void __qmljs_init_string(Value *result, String *string);
void __qmljs_init_object(Value *result, Object *object);
void __qmljs_init_closure(Context *ctx, Value *result, IR::Function *clos);
void __qmljs_init_native_function(Context *ctx, Value *result, void (*code)(Context *));

bool __qmljs_is_function(Context *ctx, const Value *value);

// string literals
void __qmljs_string_literal_undefined(Context *ctx, Value *result);
void __qmljs_string_literal_null(Context *ctx, Value *result);
void __qmljs_string_literal_true(Context *ctx, Value *result);
void __qmljs_string_literal_false(Context *ctx, Value *result);
void __qmljs_string_literal_object(Context *ctx, Value *result);
void __qmljs_string_literal_boolean(Context *ctx, Value *result);
void __qmljs_string_literal_number(Context *ctx, Value *result);
void __qmljs_string_literal_string(Context *ctx, Value *result);
void __qmljs_string_literal_function(Context *ctx, Value *result);

// strings
String *__qmljs_string_from_utf8(Context *ctx, const char *s);
int __qmljs_string_length(Context *ctx, String *string);
double __qmljs_string_to_number(Context *ctx, String *string);
void __qmljs_string_from_number(Context *ctx, Value *result, double number);
bool __qmljs_string_compare(Context *ctx, String *left, String *right);
bool __qmljs_string_equal(Context *ctx, String *left, String *right);
String *__qmljs_string_concat(Context *ctx, String *first, String *second);
String *__qmljs_identifier_from_utf8(Context *ctx, const char *s);

// objects
void __qmljs_object_default_value(Context *ctx, Value *result, const Value *object, int typeHint);
void __qmljs_throw_type_error(Context *ctx, Value *result);
void __qmljs_new_object(Context *ctx, Value *result);
void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean);
void __qmljs_new_number_object(Context *ctx, Value *result, double n);
void __qmljs_new_string_object(Context *ctx, Value *result, String *string);
void __qmljs_set_activation_property(Context *ctx, String *name, Value *value);
void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value);
void __qmljs_get_property(Context *ctx, Value *result, Value *object, String *name);
void __qmljs_get_activation_property(Context *ctx, Value *result, String *name);
void __qmljs_copy_activation_property(Context *ctx, String *name, String *other);

void __qmljs_get_element(Context *ctx, Value *result, Value *object, Value *index);
void __qmljs_set_element(Context *ctx, Value *object, Value *index, Value *value);

void __qmljs_set_activation_element(Context *ctx, String *name, Value *index, Value *value);

// context
void __qmljs_get_activation(Context *ctx, Value *result);
void __qmljs_get_thisObject(Context *ctx, Value *result);

// type conversion and testing
void __qmljs_to_primitive(Context *ctx, Value *result, const Value *value, int typeHint);
bool __qmljs_to_boolean(Context *ctx, const Value *value);
double __qmljs_to_number(Context *ctx, const Value *value);
double __qmljs_to_integer(Context *ctx, const Value *value);
int __qmljs_to_int32(Context *ctx, const Value *value);
unsigned __qmljs_to_uint32(Context *ctx, const Value *value);
unsigned short __qmljs_to_uint16(Context *ctx, const Value *value);
void __qmljs_to_string(Context *ctx, Value *result, const Value *value);
void __qmljs_to_object(Context *ctx, Value *result, const Value *value);
bool __qmljs_check_object_coercible(Context *ctx, Value *result, const Value *value);
bool __qmljs_is_callable(Context *ctx, const Value *value);
void __qmljs_default_value(Context *ctx, Value *result, const Value *value, int typeHint);

void __qmljs_compare(Context *ctx, Value *result, const Value *left, const Value *right, bool leftFlag);
bool __qmljs_equal(Context *ctx, const Value *x, const Value *y);
bool __qmljs_strict_equal(Context *ctx, const Value *x, const Value *y);

// unary operators
void __qmljs_postincr(Context *ctx, Value *result, const Value *value);
void __qmljs_postdecr(Context *ctx, Value *result, const Value *value);
void __qmljs_uplus(Context *ctx, Value *result, const Value *value);
void __qmljs_uminus(Context *ctx, Value *result, const Value *value);
void __qmljs_compl(Context *ctx, Value *result, const Value *value);
void __qmljs_not(Context *ctx, Value *result, const Value *value);
void __qmljs_preincr(Context *ctx, Value *result, const Value *value);
void __qmljs_predecr(Context *ctx, Value *result, const Value *value);

void __qmljs_delete_subscript(Context *ctx, Value *result, Value *base, Value *index);
void __qmljs_delete_member(Context *ctx, Value *result, Value *base, String *name);
void __qmljs_delete_property(Context *ctx, Value *result, String *name);
void __qmljs_delete_value(Context *ctx, Value *result, Value *value);

void __qmljs_typeof(Context *ctx, Value *result, const Value *value);
void __qmljs_throw(Context *context, Value *value);
void __qmljs_rethrow(Context *context, Value *result);

// binary operators
void __qmljs_instanceof(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_in(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_bit_or(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_bit_xor(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_bit_and(Context *ctx, Value *result, const Value *left,const Value *right);
void __qmljs_add(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_sub(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_mul(Context *ctx, Value *result, const Value *left,const Value *right);
void __qmljs_div(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_mod(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_shl(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_shr(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_ushr(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_gt(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_lt(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_ge(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_le(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_eq(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_ne(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_se(Context *ctx, Value *result, const Value *left, const Value *right);
void __qmljs_sne(Context *ctx, Value *result, const Value *left, const Value *right);

void __qmljs_add_helper(Context *ctx, Value *result, const Value *left, const Value *right);

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

void __qmljs_inplace_bit_and_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_bit_or_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_bit_xor_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_add_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_sub_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_mul_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_div_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_mod_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_shl_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_shr_name(Context *ctx, String *name, Value *value);
void __qmljs_inplace_ushr_name(Context *ctx, String *name, Value *value);

void __qmljs_inplace_bit_and_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_bit_or_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_bit_xor_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_add_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_sub_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_mul_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_div_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_mod_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_shl_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_shr_element(Context *ctx, Value *base, Value *index, Value *value);
void __qmljs_inplace_ushr_element(Context *ctx, Value *base, Value *index, Value *value);

void __qmljs_inplace_bit_and_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_bit_or_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_bit_xor_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_add_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_sub_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_mul_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_div_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_mod_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_shl_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_shr_member(Context *ctx, Value *base, String *name, Value *value);
void __qmljs_inplace_ushr_member(Context *ctx, Value *base, String *name, Value *value);

bool __qmljs_cmp_gt(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_lt(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_ge(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_le(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_eq(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_ne(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_se(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_sne(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_instanceof(Context *ctx, const Value *left, const Value *right);
bool __qmljs_cmp_in(Context *ctx, const Value *left, const Value *right);


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
                bool b;
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
//        Integer_Type = NaN_Mask | 0x2,
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
        return b;
    }
    double doubleValue() const {
        return dbl;
    }
    void setDouble(double d) {
        dbl = d;
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

    static Value undefinedValue();
    static Value nullValue();
    static Value fromBoolean(bool b);
    static Value fromDouble(double d);
    static Value fromInt32(int i);
    static Value fromString(String *s);
    static Value fromObject(Object *o);
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
//        Integer_Type = NaN_Mask | 0x20000,
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

    bool booleanValue() const {
        return b;
    }
    double doubleValue() const {
        return dbl;
    }
    void setDouble(double d) {
        dbl = d;
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
    static Value fromBoolean(bool b);
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

    static int toInteger(double fromNumber);
    static int toInt32(double value);
    static unsigned int toUInt32(double value);

    int toUInt16(Context *ctx);
    int toInt32(Context *ctx);
    unsigned int toUInt32(Context *ctx);
    bool toBoolean(Context *ctx) const;
    double toInteger(Context *ctx) const;
    double toNumber(Context *ctx) const;
    String *toString(Context *ctx) const;
    Value toObject(Context *ctx) const;

    inline bool isUndefined() const { return is(Value::Undefined_Type); }
    inline bool isNull() const { return is(Value::Null_Type); }
    inline bool isString() const { return is(Value::String_Type); }
    inline bool isBoolean() const { return type() == Value::Boolean_Type; }
    inline bool isNumber() const { return is(Value::Double_Type) /*|| is(Value::Integer_Type)*/; }
    inline bool isObject() const { return type() == Value::Object_Type; }

    inline bool isPrimitive() const { ValueType t = type(); return t != Value::Object_Type; }
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

inline Value ValueBase<4>::fromBoolean(bool b)
{
    Value v;
    v.tag = Boolean_Type;
    v.b = b;
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
    // ###
    v.dbl = i;
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

inline Value ValueBase<8>::fromBoolean(bool b)
{
    Value v;
    v.tag = Boolean_Type;
    v.b = b;
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
    // ###
    v.dbl = i;
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

template <Value::ValueType> struct ValueOffsetHelper;
template <> struct ValueOffsetHelper<Value::Boolean_Type>
{
    enum { DataOffset = offsetof(ValueData, b) };
};

template <> struct ValueOffsetHelper<Value::Undefined_Type>
{
    enum { DataOffset = offsetof(ValueData, uint_32) };
};

template <> struct ValueOffsetHelper<Value::Null_Type>
{
    enum { DataOffset = offsetof(ValueData, uint_32) };
};

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
            __qmljs_init_undefined(result);
    }

    void init(ExecutionEngine *eng);

    void throwError(const Value &value);
    void throwTypeError();
    void throwReferenceError(const Value &value);

#ifndef QMLJS_LLVM_RUNTIME
    void throwError(const QString &message);
    void throwUnimplemented(const QString &message);
#endif

    void initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, unsigned argc);
    void leaveCallContext(FunctionObject *f, Value *r);

    void initConstructorContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, unsigned argc);
    void leaveConstructorContext(FunctionObject *f, Value *returnValue);
};



extern "C" {

// constructors
inline void __qmljs_init_undefined(Value *result)
{
    *result = Value::undefinedValue();
}

inline void __qmljs_init_null(Value *result)
{
    *result = Value::nullValue();
}

inline void __qmljs_init_boolean(Value *result, bool value)
{
    *result = Value::fromBoolean(value);
}

inline void __qmljs_init_number(Value *result, double value)
{
    *result = Value::fromDouble(value);
}

inline void __qmljs_init_string(Value *result, String *value)
{
    *result = Value::fromString(value);
}

inline void __qmljs_init_object(Value *result, Object *object)
{
    *result = Value::fromObject(object);
}

// type conversion and testing
inline void __qmljs_to_primitive(Context *ctx, Value *result, const Value *value, int typeHint)
{
    if (!value->isObject())
        *result = *value;
    else
        __qmljs_default_value(ctx, result, value, typeHint);
}

inline bool __qmljs_to_boolean(Context *ctx, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        return false;
    case Value::Boolean_Type:
        return value->booleanValue();
//    case Value::Integer_Type:
//        return value->data;
    case Value::String_Type:
        return __qmljs_string_length(ctx, value->stringValue()) > 0;
    case Value::Object_Type:
        return true;
    default: // double
        if (! value->doubleValue() || isnan(value->doubleValue()))
            return false;
        return true;
    }
}

inline double __qmljs_to_number(Context *ctx, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
        return nan("");
    case Value::Null_Type:
        return 0;
    case Value::Boolean_Type:
        return (value->booleanValue() ? 1. : 0.);
//    case Value::Integer_Type:
//        return value->data;
    case Value::String_Type:
        return __qmljs_string_to_number(ctx, value->stringValue());
    case Value::Object_Type: {
        Value prim;
        __qmljs_to_primitive(ctx, &prim, value, NUMBER_HINT);
        return __qmljs_to_number(ctx, &prim);
    }
    default: // double
        return value->doubleValue();
    }
}

inline double __qmljs_to_integer(Context *ctx, const Value *value)
{
    const double number = __qmljs_to_number(ctx, value);
    if (isnan(number))
        return +0;
    else if (! number || isinf(number))
        return number;
    const double v = floor(fabs(number));
    return signbit(number) ? -v : v;
}

inline int __qmljs_to_int32(Context *ctx, const Value *value)
{
    double number = __qmljs_to_number(ctx, value);

    if ((number >= -2147483648.0 && number < 2147483648.0)) {
        return static_cast<int>(number);
    }

    if (! number || isnan(number) || isinf(number))
        return 0;

    double D32 = 4294967296.0;
    double sign = (number < 0) ? -1.0 : 1.0;
    double abs_n = fabs(number);

    number = ::fmod(sign * ::floor(abs_n), D32);
    const double D31 = D32 / 2.0;

    if (sign == -1 && number < -D31)
        number += D32;

    else if (sign != -1 && number >= D31)
        number -= D32;

    return int(number);
}

inline unsigned __qmljs_to_uint32(Context *ctx, const Value *value)
{
    double number = __qmljs_to_number(ctx, value);
    if (! number || isnan(number) || isinf(number))
        return +0;

    double sign = (number < 0) ? -1.0 : 1.0;
    double abs_n = ::fabs(number);

    const double D32 = 4294967296.0;
    number = ::fmod(sign * ::floor(abs_n), D32);

    if (number < 0)
        number += D32;

    return unsigned(number);
}

inline unsigned short __qmljs_to_uint16(Context *ctx, const Value *value)
{
    double number = __qmljs_to_number(ctx, value);
    if (! number || isnan(number) || isinf(number))
        return +0;

    double sign = (number < 0) ? -1.0 : 1.0;
    double abs_n = ::fabs(number);

    double D16 = 65536.0;
    number = ::fmod(sign * ::floor(abs_n), D16);

    if (number < 0)
        number += D16;

    return (unsigned short)number;
}

inline void __qmljs_to_string(Context *ctx, Value *result, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
        __qmljs_string_literal_undefined(ctx, result);
        break;
    case Value::Null_Type:
        __qmljs_string_literal_null(ctx, result);
        break;
    case Value::Boolean_Type:
        if (value->booleanValue())
            __qmljs_string_literal_true(ctx, result);
        else
            __qmljs_string_literal_false(ctx, result);
        break;
    case Value::String_Type:
        *result = *value;
        break;
    case Value::Object_Type: {
        Value prim;
        __qmljs_to_primitive(ctx, &prim, value, STRING_HINT);
        if (prim.isPrimitive())
            __qmljs_to_string(ctx, result, &prim);
        else
            __qmljs_throw_type_error(ctx, result);
        break;
    }
//    case Value::Integer_Type:
//        __qmljs_string_from_number(ctx, result, value->data);
//        break;
    default: // double
        __qmljs_string_from_number(ctx, result, value->doubleValue());
        break;

    } // switch
}

inline void __qmljs_to_object(Context *ctx, Value *result, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        __qmljs_throw_type_error(ctx, result);
        break;
    case Value::Boolean_Type:
        __qmljs_new_boolean_object(ctx, result, value->booleanValue());
        break;
    case Value::String_Type:
        __qmljs_new_string_object(ctx, result, value->stringValue());
        break;
    case Value::Object_Type:
        *result = *value;
        break;
//    case Value::Integer_Type:
//        __qmljs_new_number_object(ctx, result, value->data);
//        break;
    default:
        __qmljs_new_number_object(ctx, result, value->doubleValue());
        break;
    }
}

inline bool __qmljs_check_object_coercible(Context *ctx, Value *result, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
    case Value::Null_Type:
        __qmljs_throw_type_error(ctx, result);
        return false;
    default:
        return true;
    }
}

inline bool __qmljs_is_callable(Context *ctx, const Value *value)
{
    if (value->isObject())
        return __qmljs_is_function(ctx, value);
    else
        return false;
}

inline void __qmljs_default_value(Context *ctx, Value *result, const Value *value, int typeHint)
{
    if (value->isObject())
        __qmljs_object_default_value(ctx, result, value, typeHint);
    else
        __qmljs_init_undefined(result);
}


// unary operators
inline void __qmljs_typeof(Context *ctx, Value *result, const Value *value)
{
    switch (value->type()) {
    case Value::Undefined_Type:
        __qmljs_string_literal_undefined(ctx, result);
        break;
    case Value::Null_Type:
        __qmljs_string_literal_object(ctx, result);
        break;
    case Value::Boolean_Type:
        __qmljs_string_literal_boolean(ctx, result);
        break;
    case Value::String_Type:
        __qmljs_string_literal_string(ctx, result);
        break;
    case Value::Object_Type:
        if (__qmljs_is_callable(ctx, value))
            __qmljs_string_literal_function(ctx, result);
        else
            __qmljs_string_literal_object(ctx, result); // ### implementation-defined
        break;
//    case Value::Integer_Type:
    case Value::Double_Type:
        __qmljs_string_literal_number(ctx, result);
        break;
    }
}

inline void __qmljs_uplus(Context *ctx, Value *result, const Value *value)
{
    double n = __qmljs_to_number(ctx, value);
    __qmljs_init_number(result, n);
}

inline void __qmljs_uminus(Context *ctx, Value *result, const Value *value)
{
    double n = __qmljs_to_number(ctx, value);
    __qmljs_init_number(result, -n);
}

inline void __qmljs_compl(Context *ctx, Value *result, const Value *value)
{
    int n = __qmljs_to_int32(ctx, value);
    __qmljs_init_number(result, ~n);
}

inline void __qmljs_not(Context *ctx, Value *result, const Value *value)
{
    bool b = __qmljs_to_boolean(ctx, value);
    __qmljs_init_number(result, !b);
}

// binary operators
inline void __qmljs_bit_or(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(result, lval | rval);
}

inline void __qmljs_bit_xor(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(result, lval ^ rval);
}

inline void __qmljs_bit_and(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(result, lval & rval);
}

inline void __qmljs_inplace_bit_and(Context *ctx, Value *result, Value *value)
{
    __qmljs_bit_xor(ctx, result, result, value);
}

inline void __qmljs_inplace_bit_or(Context *ctx, Value *result, Value *value)
{
    __qmljs_bit_or(ctx, result, result, value);
}

inline void __qmljs_inplace_bit_xor(Context *ctx, Value *result, Value *value)
{
    __qmljs_bit_xor(ctx, result, result, value);
}

inline void __qmljs_inplace_add(Context *ctx, Value *result, Value *value)
{
    __qmljs_add(ctx, result, result, value);
}

inline void __qmljs_inplace_sub(Context *ctx, Value *result, Value *value)
{
    __qmljs_sub(ctx, result, result, value);
}

inline void __qmljs_inplace_mul(Context *ctx, Value *result, Value *value)
{
    __qmljs_mul(ctx, result, result, value);
}

inline void __qmljs_inplace_div(Context *ctx, Value *result, Value *value)
{
    __qmljs_div(ctx, result, result, value);
}

inline void __qmljs_inplace_mod(Context *ctx, Value *result, Value *value)
{
    __qmljs_mod(ctx, result, result, value);
}

inline void __qmljs_inplace_shl(Context *ctx, Value *result, Value *value)
{
    __qmljs_shl(ctx, result, result, value);
}

inline void __qmljs_inplace_shr(Context *ctx, Value *result, Value *value)
{
    __qmljs_shr(ctx, result, result, value);
}

inline void __qmljs_inplace_ushr(Context *ctx, Value *result, Value *value)
{
    __qmljs_ushr(ctx, result, result, value);
}

inline void __qmljs_add(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber())
        __qmljs_init_number(result, left->doubleValue() + right->doubleValue());
    else
        __qmljs_add_helper(ctx, result, left, right);
}

inline void __qmljs_sub(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(result, lval - rval);
}

inline void __qmljs_mul(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(result, lval * rval);
}

inline void __qmljs_div(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(result, lval / rval);
}

inline void __qmljs_mod(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(result, fmod(lval, rval));
}

inline void __qmljs_shl(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(result, lval << rval);
}

inline void __qmljs_shr(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(result, lval >> rval);
}

inline void __qmljs_ushr(Context *ctx, Value *result, const Value *left, const Value *right)
{
    unsigned lval = __qmljs_to_uint32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(result, lval >> rval);
}

inline void __qmljs_gt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() > right->doubleValue());
    } else {
        __qmljs_compare(ctx, result, left, right, false);

        if (result->isUndefined())
            __qmljs_init_boolean(result, false);
    }
}

inline void __qmljs_lt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() < right->doubleValue());
    } else {
        __qmljs_compare(ctx, result, left, right, true);

        if (result->isUndefined())
            __qmljs_init_boolean(result, false);
    }
}

inline void __qmljs_ge(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() >= right->doubleValue());
    } else {
        __qmljs_compare(ctx, result, right, left, false);

        bool r = ! (result->isUndefined() || (result->isBoolean() && result->booleanValue()));
        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_le(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() <= right->doubleValue());
    } else {
        __qmljs_compare(ctx, result, right, left, true);

        bool r = ! (result->isUndefined() || (result->isBoolean() && result->booleanValue()));
        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_eq(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() == right->doubleValue());
    } else if (left->isString() && right->isString()) {
        __qmljs_init_boolean(result, __qmljs_string_equal(ctx, left->stringValue(), right->stringValue()));
    } else {
        bool r = __qmljs_equal(ctx, left, right);
        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_ne(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->isNumber() && right->isNumber()) {
        __qmljs_init_boolean(result, left->doubleValue() != right->doubleValue());
    } else if (left->isString() && right->isString()) {
        __qmljs_init_boolean(result, !__qmljs_string_equal(ctx, left->stringValue(), right->stringValue()));
    } else {
        bool r = ! __qmljs_equal(ctx, left, right);
        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_se(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = __qmljs_strict_equal(ctx, left, right);
    __qmljs_init_boolean(result, r);
}

inline void __qmljs_sne(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = ! __qmljs_strict_equal(ctx, left, right);
    __qmljs_init_boolean(result, r);
}

inline bool __qmljs_cmp_gt(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_gt(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_lt(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_lt(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_ge(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_ge(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_le(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_le(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_eq(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_eq(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_ne(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_ne(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_se(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_se(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_sne(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_sne(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_instanceof(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_instanceof(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_cmp_in(Context *ctx, const Value *left, const Value *right)
{
    Value v;
    __qmljs_in(ctx, &v, left, right);
    return __qmljs_to_boolean(ctx, &v);
}

inline bool __qmljs_strict_equal(Context *ctx, const Value *x, const Value *y)
{
    if (x->rawValue() == y->rawValue())
        return true;
    if (x->isString() && y->isString())
        return __qmljs_string_equal(ctx, x->stringValue(), y->stringValue());
    return false;
}

} // extern "C"

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_RUNTIME_H
