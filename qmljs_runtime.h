#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#ifndef QMLJS_LLVM_RUNTIME
#  include <QtCore/QString>
#endif

#include <math.h>
#include <cassert>

namespace QQmlJS {

namespace IR {
struct Function;
}

namespace VM {

enum ValueType {
    UNDEFINED_TYPE,
    NULL_TYPE,
    BOOLEAN_TYPE,
    NUMBER_TYPE,
    STRING_TYPE,
    OBJECT_TYPE
};

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
void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value);
void __qmljs_set_property_boolean(Context *ctx, Value *object, String *name, bool value);
void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double value);
void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *value);
void __qmljs_set_property_closure(Context *ctx, Value *object, String *name, IR::Function *function);
void __qmljs_set_activation_property(Context *ctx, String *name, Value *value);
void __qmljs_set_activation_property_boolean(Context *ctx, String *name, bool value);
void __qmljs_set_activation_property_number(Context *ctx, String *name, double value);
void __qmljs_set_activation_property_string(Context *ctx, String *name, String *value);
void __qmljs_set_activation_property_closure(Context *ctx, String *name, IR::Function *function);
void __qmljs_get_property(Context *ctx, Value *result, Value *object, String *name);
void __qmljs_get_activation_property(Context *ctx, Value *result, String *name);
void __qmljs_copy_activation_property(Context *ctx, String *name, String *other);

void __qmljs_get_element(Context *ctx, Value *result, Value *object, Value *index);

void __qmljs_set_element(Context *ctx, Value *object, Value *index, Value *value);
void __qmljs_set_element_number(Context *ctx, Value *object, Value *index, double number);

void __qmljs_set_activation_element(Context *ctx, String *name, Value *index, Value *value);
void __qmljs_set_activation_element_number(Context *ctx, String *name, Value *index, double number);

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

void __qmljs_inplace_bit_and(Context *ctx, Value *result, double value);
void __qmljs_inplace_bit_or(Context *ctx, Value *result, double value);
void __qmljs_inplace_bit_xor(Context *ctx, Value *result, double value);
void __qmljs_inplace_add(Context *ctx, Value *result, double value);
void __qmljs_inplace_sub(Context *ctx, Value *result, double value);
void __qmljs_inplace_mul(Context *ctx, Value *result, double value);
void __qmljs_inplace_div(Context *ctx, Value *result, double value);
void __qmljs_inplace_mod(Context *ctx, Value *result, double value);
void __qmljs_inplace_shl(Context *ctx, Value *result, double value);
void __qmljs_inplace_shr(Context *ctx, Value *result, double value);
void __qmljs_inplace_ushr(Context *ctx, Value *result, double value);

void __qmljs_inplace_bit_and_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_bit_or_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_bit_xor_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_add_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_sub_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_mul_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_div_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_mod_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_shl_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_shr_element(Context *ctx, Value *base, Value *index, double value);
void __qmljs_inplace_ushr_element(Context *ctx, Value *base, Value *index, double value);

void __qmljs_inplace_bit_and_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_bit_or_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_bit_xor_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_add_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_sub_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_mul_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_div_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_mod_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_shl_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_shr_member(Context *ctx, Value *base, String *name, double value);
void __qmljs_inplace_ushr_member(Context *ctx, Value *base, String *name, double value);

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

struct Value {
    int type;
    union {
        bool booleanValue;
        double numberValue;
        Object *objectValue;
        String *stringValue;
    };

    inline bool is(ValueType t) const { return type == t; }
    inline bool isNot(ValueType t) const { return type != t; }

    static inline Value undefinedValue() {
        Value v;
        v.type = UNDEFINED_TYPE;
        return v;
    }

    static inline Value nullValue() {
        Value v;
        v.type = NULL_TYPE;
        return v;
    }

    static inline Value fromBoolean(bool value) {
        Value v;
        __qmljs_init_boolean(&v, value);
        return v;
    }

    static inline Value fromNumber(double value) {
        Value v;
        __qmljs_init_number(&v, value);
        return v;
    }

    static inline Value fromObject(Object *value) {
        Value v;
        if (value) {
            __qmljs_init_object(&v, value);
        } else {
            __qmljs_init_null(&v);
        }
        return v;
    }

    static inline Value fromString(String *value) {
        Value v;
        __qmljs_init_string(&v, value);
        return v;
    }

#ifndef QMLJS_LLVM_RUNTIME
    static Value fromString(Context *ctx, const QString &fromString);
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

    inline bool isUndefined() const { return is(UNDEFINED_TYPE); }
    inline bool isNull() const { return is(NULL_TYPE); }
    inline bool isString() const { return is(STRING_TYPE); }
    inline bool isBoolean() const { return is(BOOLEAN_TYPE); }
    inline bool isNumber() const { return is(NUMBER_TYPE); }
    inline bool isObject() const { return is(OBJECT_TYPE); }

    inline bool isPrimitive() const { return type != OBJECT_TYPE; }
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

    void initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc);
    void leaveCallContext(FunctionObject *f, Value *r);

    void initConstructorContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc);
    void leaveConstructorContext(FunctionObject *f, Value *returnValue);
};



extern "C" {

// constructors
inline void __qmljs_init_undefined(Value *result)
{
    result->type = UNDEFINED_TYPE;
}

inline void __qmljs_init_null(Value *result)
{
    result->type = NULL_TYPE;
}

inline void __qmljs_init_boolean(Value *result, bool value)
{
    result->type = BOOLEAN_TYPE;
    result->booleanValue = value;
}

inline void __qmljs_init_number(Value *result, double value)
{
    result->type = NUMBER_TYPE;
    result->numberValue = value;
}

inline void __qmljs_init_string(Value *result, String *value)
{
    result->type = STRING_TYPE;
    result->stringValue = value;
}

inline void __qmljs_init_object(Value *result, Object *object)
{
    result->type = OBJECT_TYPE;
    result->objectValue = object;
}

inline void __qmljs_copy(Value *result, Value *source)
{
    result->type = source->type;
    result->numberValue = source->numberValue;
}

// type conversion and testing
inline void __qmljs_to_primitive(Context *ctx, Value *result, const Value *value, int typeHint)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
    case BOOLEAN_TYPE:
    case NUMBER_TYPE:
    case STRING_TYPE:
        *result = *value;
        break;
    case OBJECT_TYPE:
        __qmljs_default_value(ctx, result, value, typeHint);
        break;
    }
}

inline bool __qmljs_to_boolean(Context *ctx, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
        return false;
    case BOOLEAN_TYPE:
        return value->booleanValue;
    case NUMBER_TYPE:
        if (! value->numberValue || isnan(value->numberValue))
            return false;
        return true;
    case STRING_TYPE:
        return __qmljs_string_length(ctx, value->stringValue) > 0;
    case OBJECT_TYPE:
        return true;
    }
    assert(!"unreachable");
    return false;
}

inline double __qmljs_to_number(Context *ctx, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
        return nan("");
    case NULL_TYPE:
        return 0;
    case BOOLEAN_TYPE:
        return (double) value->booleanValue;
    case NUMBER_TYPE:
        return value->numberValue;
    case STRING_TYPE:
        return __qmljs_string_to_number(ctx, value->stringValue);
    case OBJECT_TYPE: {
        Value prim;
        __qmljs_to_primitive(ctx, &prim, value, NUMBER_HINT);
        return __qmljs_to_number(ctx, &prim);
    }
    } // switch
    return 0; // unreachable
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
    const double number = __qmljs_to_number(ctx, value);
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (int) trunc(number); // ###
}

inline unsigned __qmljs_to_uint32(Context *ctx, const Value *value)
{
    const double number = __qmljs_to_number(ctx, value);
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (unsigned) trunc(number); // ###
}

inline unsigned short __qmljs_to_uint16(Context *ctx, const Value *value)
{
    const double number = __qmljs_to_number(ctx, value);
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (unsigned short) trunc(number); // ###
}

inline void __qmljs_to_string(Context *ctx, Value *result, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
        __qmljs_string_literal_undefined(ctx, result);
        break;
    case NULL_TYPE:
        __qmljs_string_literal_null(ctx, result);
        break;
    case BOOLEAN_TYPE:
        if (value->booleanValue)
            __qmljs_string_literal_true(ctx, result);
        else
            __qmljs_string_literal_false(ctx, result);
        break;
    case NUMBER_TYPE:
        __qmljs_string_from_number(ctx, result, value->numberValue);
        break;
    case STRING_TYPE:
        *result = *value;
        break;
    case OBJECT_TYPE: {
        Value prim;
        __qmljs_to_primitive(ctx, &prim, value, STRING_HINT);
        if (prim.isPrimitive())
            __qmljs_to_string(ctx, result, &prim);
        else
            __qmljs_throw_type_error(ctx, result);
        break;
    }

    } // switch
}

inline void __qmljs_to_object(Context *ctx, Value *result, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
        __qmljs_throw_type_error(ctx, result);
        break;
    case BOOLEAN_TYPE:
        __qmljs_new_boolean_object(ctx, result, value->booleanValue);
        break;
    case NUMBER_TYPE:
        __qmljs_new_number_object(ctx, result, value->numberValue);
        break;
    case STRING_TYPE:
        __qmljs_new_string_object(ctx, result, value->stringValue);
        break;
    case OBJECT_TYPE:
        *result = *value;
        break;
    }
}

inline bool __qmljs_check_object_coercible(Context *ctx, Value *result, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
        __qmljs_throw_type_error(ctx, result);
        return false;
    case BOOLEAN_TYPE:
    case NUMBER_TYPE:
    case STRING_TYPE:
    case OBJECT_TYPE:
        return true;
    }
}

inline bool __qmljs_is_callable(Context *ctx, const Value *value)
{
    if (value->type == OBJECT_TYPE)
        return __qmljs_is_function(ctx, value);
    else
        return false;
}

inline void __qmljs_default_value(Context *ctx, Value *result, const Value *value, int typeHint)
{
    if (value->type == OBJECT_TYPE)
        __qmljs_object_default_value(ctx, result, value, typeHint);
    else
        __qmljs_init_undefined(result);
}


// unary operators
inline void __qmljs_typeof(Context *ctx, Value *result, const Value *value)
{
    switch ((ValueType) value->type) {
    case UNDEFINED_TYPE:
        __qmljs_string_literal_undefined(ctx, result);
        break;
    case NULL_TYPE:
        __qmljs_string_literal_object(ctx, result);
        break;
    case BOOLEAN_TYPE:
        __qmljs_string_literal_boolean(ctx, result);
        break;
    case NUMBER_TYPE:
        __qmljs_string_literal_number(ctx, result);
        break;
    case STRING_TYPE:
        __qmljs_string_literal_string(ctx, result);
        break;
    case OBJECT_TYPE:
        if (__qmljs_is_callable(ctx, value))
            __qmljs_string_literal_function(ctx, result);
        else
            __qmljs_string_literal_object(ctx, result); // ### implementation-defined
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

inline void __qmljs_inplace_bit_and(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_bit_xor(ctx, result, result, &v);
}

inline void __qmljs_inplace_bit_or(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_bit_or(ctx, result, result, &v);
}

inline void __qmljs_inplace_bit_xor(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_bit_xor(ctx, result, result, &v);
}

inline void __qmljs_inplace_add(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_add(ctx, result, result, &v);
}

inline void __qmljs_inplace_sub(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_sub(ctx, result, result, &v);
}

inline void __qmljs_inplace_mul(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_mul(ctx, result, result, &v);
}

inline void __qmljs_inplace_div(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_div(ctx, result, result, &v);
}

inline void __qmljs_inplace_mod(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_mod(ctx, result, result, &v);
}

inline void __qmljs_inplace_shl(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_shl(ctx, result, result, &v);
}

inline void __qmljs_inplace_shr(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_shr(ctx, result, result, &v);
}

inline void __qmljs_inplace_ushr(Context *ctx, Value *result, double value)
{
    Value v;
    __qmljs_init_number(&v, value);
    __qmljs_ushr(ctx, result, result, &v);
}

inline void __qmljs_add(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE)
        __qmljs_init_number(result, left->numberValue + right->numberValue);
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
    __qmljs_init_number(result, lval * rval);
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
    __qmljs_init_number(result, lval << rval);
}

inline void __qmljs_gt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue > right->numberValue);
    } else {
        __qmljs_compare(ctx, result, left, right, false);

        if (result->type == UNDEFINED_TYPE)
            __qmljs_init_boolean(result, false);
    }
}

inline void __qmljs_lt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue < right->numberValue);
    } else {
        __qmljs_compare(ctx, result, left, right, true);

        if (result->type == UNDEFINED_TYPE)
            __qmljs_init_boolean(result, false);
    }
}

inline void __qmljs_ge(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue >= right->numberValue);
    } else {
        __qmljs_compare(ctx, result, right, left, false);

        bool r = ! (result->type == UNDEFINED_TYPE ||
                    (result->type == BOOLEAN_TYPE && result->booleanValue == true));

        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_le(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue <= right->numberValue);
    } else {
        __qmljs_compare(ctx, result, right, left, true);

        bool r = ! (result->type == UNDEFINED_TYPE ||
                    (result->type == BOOLEAN_TYPE && result->booleanValue == true));

        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_eq(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue == right->numberValue);
    } else if (left->type == STRING_TYPE && right->type == STRING_TYPE) {
        __qmljs_init_boolean(result, __qmljs_string_equal(ctx, left->stringValue, right->stringValue));
    } else {
        bool r = __qmljs_equal(ctx, left, right);
        __qmljs_init_boolean(result, r);
    }
}

inline void __qmljs_ne(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (left->type == NUMBER_TYPE && right->type == NUMBER_TYPE) {
        __qmljs_init_boolean(result, left->numberValue != right->numberValue);
    } else if (left->type == STRING_TYPE && right->type != STRING_TYPE) {
        __qmljs_init_boolean(result, __qmljs_string_equal(ctx, left->stringValue, right->stringValue));
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
    if (x->type != y->type)
        return false;

    switch ((ValueType) x->type) {
    case UNDEFINED_TYPE:
    case NULL_TYPE:
        return true;
    case BOOLEAN_TYPE:
        return x->booleanValue == y->booleanValue;
    case NUMBER_TYPE:
        return x->numberValue == y->numberValue;
    case STRING_TYPE:
        return __qmljs_string_equal(ctx, x->stringValue, y->stringValue);
    case OBJECT_TYPE:
        return x->objectValue == y->objectValue;
    }
    assert(!"unreachable");
    return false;
}

} // extern "C"

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_RUNTIME_H
