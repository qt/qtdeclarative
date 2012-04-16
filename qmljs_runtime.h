#ifndef QMLJS_RUNTIME_H
#define QMLJS_RUNTIME_H

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <math.h>

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

extern "C" {

// constructors
void __qmljs_init_undefined(Context *ctx, Value *result);
void __qmljs_init_null(Context *ctx, Value *result);
void __qmljs_init_boolean(Context *ctx, Value *result, bool value);
void __qmljs_init_number(Context *ctx, Value *result, double number);
void __qmljs_init_string(Context *ctx, Value *result, String *string);
void __qmljs_init_object(Context *ctx, Value *result, Object *object);

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
int __qmljs_string_length(Context *ctx, String *string);
double __qmljs_string_to_number(Context *ctx, String *string);
void __qmljs_string_from_number(Context *ctx, Value *result, double number);
bool __qmljs_string_compare(Context *ctx, String *left, String *right);
bool __qmljs_string_equal(Context *ctx, String *left, String *right);
String *__qmljs_string_concat(Context *ctx, String *first, String *second);

// objects
void __qmljs_object_default_value(Context *ctx, Value *result, Object *object, int typeHint);
void __qmljs_throw_type_error(Context *ctx, Value *result);
void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean);
void __qmljs_new_number_object(Context *ctx, Value *result, double n);
void __qmljs_new_string_object(Context *ctx, Value *result, String *string);
void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value);
void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double value);
void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *value);
void __qmljs_set_activation_property(Context *ctx, String *name, Value *value);
void __qmljs_set_activation_property_number(Context *ctx, String *name, double value);
void __qmljs_set_activation_property_string(Context *ctx, String *name, String *value);
void __qmljs_get_property(Context *ctx, Value *result, Value *object, String *name);
void __qmljs_get_activation_property(Context *ctx, Value *result, String *name);

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
void __qmljs_delete(Context *ctx, Value *result, const Value *value);
void __qmljs_typeof(Context *ctx, Value *result, const Value *value);
void __qmljs_postincr(Context *ctx, Value *result, const Value *value);
void __qmljs_postdecr(Context *ctx, Value *result, const Value *value);
void __qmljs_uplus(Context *ctx, Value *result, const Value *value);
void __qmljs_uminus(Context *ctx, Value *result, const Value *value);
void __qmljs_compl(Context *ctx, Value *result, const Value *value);
void __qmljs_not(Context *ctx, Value *result, const Value *value);
void __qmljs_preincr(Context *ctx, Value *result, const Value *value);
void __qmljs_predecr(Context *ctx, Value *result, const Value *value);

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

void __qmljs_call(Context *ctx, Value *result, const Value *function, const Value *thisObject, const Value *arguments, int argc);
void __qmjs_construct(Context *ctx, Value *result, const Value *function, const Value *arguments, int argc);

} // extern "C"

struct Value {
    int type;
    union {
        bool booleanValue;
        double numberValue;
        Object *objectValue;
        String *stringValue;
    };

    static inline Value boolean(Context *ctx, bool value) {
        Value v;
        __qmljs_init_boolean(ctx, &v, value);
        return v;
    }

    static inline Value number(Context *ctx, double value) {
        Value v;
        __qmljs_init_number(ctx, &v, value);
        return v;
    }

    static inline Value object(Context *ctx, Object *value) {
        Value v;
        __qmljs_init_object(ctx, &v, value);
        return v;
    }

    static inline Value string(Context *ctx, String *value) {
        Value v;
        __qmljs_init_string(ctx, &v, value);
        return v;
    }

    static Value string(Context *ctx, const QString &string);
};

extern "C" {

// constructors
inline void __qmljs_init_undefined(Context *, Value *result)
{
    result->type = UNDEFINED_TYPE;
}

inline void __qmljs_init_null(Context *, Value *result)
{
    result->type = NULL_TYPE;
}

inline void __qmljs_init_boolean(Context *, Value *result, bool value)
{
    result->type = BOOLEAN_TYPE;
    result->booleanValue = value;
}

inline void __qmljs_init_number(Context *, Value *result, double value)
{
    result->type = NUMBER_TYPE;
    result->numberValue = value;
}

inline void __qmljs_init_string(Context *, Value *result, String *value)
{
    result->type = STRING_TYPE;
    result->stringValue = value;
}

inline void __qmljs_init_object(Context *, Value *result, Object *object)
{
    result->type = OBJECT_TYPE;
    result->objectValue = object;
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
    Q_ASSERT(!"unreachable");
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
    return signbit(number) * floor(fabs(number));
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
        __qmljs_to_string(ctx, result, &prim);
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
        __qmljs_object_default_value(ctx, result, value->objectValue, typeHint);
    else
        __qmljs_init_undefined(ctx, result);
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
    __qmljs_init_number(ctx, result, n);
}

inline void __qmljs_uminus(Context *ctx, Value *result, const Value *value)
{
    double n = __qmljs_to_number(ctx, value);
    __qmljs_init_number(ctx, result, -n);
}

inline void __qmljs_compl(Context *ctx, Value *result, const Value *value)
{
    int n = __qmljs_to_int32(ctx, value);
    __qmljs_init_number(ctx, result, ~n);
}

inline void __qmljs_not(Context *ctx, Value *result, const Value *value)
{
    bool b = __qmljs_to_boolean(ctx, value);
    __qmljs_init_number(ctx, result, !b);
}

// binary operators
inline void __qmljs_bit_or(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(ctx, result, lval | rval);
}

inline void __qmljs_bit_xor(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(ctx, result, lval ^ rval);
}

inline void __qmljs_bit_and(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    int rval = __qmljs_to_int32(ctx, right);
    __qmljs_init_number(ctx, result, lval & rval);
}

inline void __qmljs_add(Context *ctx, Value *result, const Value *left, const Value *right)
{
    Value pleft, pright;
    __qmljs_to_primitive(ctx, &pleft, left, PREFERREDTYPE_HINT);
    __qmljs_to_primitive(ctx, &pright, right, PREFERREDTYPE_HINT);
    if (pleft.type == STRING_TYPE || pright.type == STRING_TYPE) {
        if (pleft.type != STRING_TYPE)
            __qmljs_to_string(ctx, &pleft, &pleft);
        if (pright.type != STRING_TYPE)
            __qmljs_to_string(ctx, &pright, &pright);
        String *string = __qmljs_string_concat(ctx, pleft.stringValue, pright.stringValue);
        __qmljs_init_string(ctx, result, string);
    } else {
        double x = __qmljs_to_number(ctx, &pleft);
        double y = __qmljs_to_number(ctx, &pright);
        __qmljs_init_number(ctx, result, x + y);
    }
}

inline void __qmljs_sub(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(ctx, result, lval - rval);
}

inline void __qmljs_mul(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(ctx, result, lval * rval);
}

inline void __qmljs_div(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(ctx, result, lval * rval);
}

inline void __qmljs_mod(Context *ctx, Value *result, const Value *left, const Value *right)
{
    double lval = __qmljs_to_number(ctx, left);
    double rval = __qmljs_to_number(ctx, right);
    __qmljs_init_number(ctx, result, fmod(lval, rval));
}

inline void __qmljs_shl(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(ctx, result, lval << rval);
}

inline void __qmljs_shr(Context *ctx, Value *result, const Value *left, const Value *right)
{
    int lval = __qmljs_to_int32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(ctx, result, lval >> rval);
}

inline void __qmljs_ushr(Context *ctx, Value *result, const Value *left, const Value *right)
{
    unsigned lval = __qmljs_to_uint32(ctx, left);
    unsigned rval = __qmljs_to_uint32(ctx, right);
    __qmljs_init_number(ctx, result, lval << rval);
}

inline void __qmljs_gt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    __qmljs_compare(ctx, result, right, left, false);

    if (result->type == UNDEFINED_TYPE)
        __qmljs_init_boolean(ctx, result, false);
}

inline void __qmljs_lt(Context *ctx, Value *result, const Value *left, const Value *right)
{
    __qmljs_compare(ctx, result, left, right, true);

    if (result->type == UNDEFINED_TYPE)
        __qmljs_init_boolean(ctx, result,false);
}

inline void __qmljs_ge(Context *ctx, Value *result, const Value *left, const Value *right)
{
    __qmljs_compare(ctx, result, right, left, false);

    bool r = ! (result->type == UNDEFINED_TYPE ||
                (result->type == BOOLEAN_TYPE && result->booleanValue == true));

    __qmljs_init_boolean(ctx, result, r);
}

inline void __qmljs_le(Context *ctx, Value *result, const Value *left, const Value *right)
{
    __qmljs_compare(ctx, result, left, right, true);

    bool r = ! (result->type == UNDEFINED_TYPE ||
                (result->type == BOOLEAN_TYPE && result->booleanValue == true));

    __qmljs_init_boolean(ctx, result, r);
}

inline void __qmljs_eq(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = __qmljs_equal(ctx, left, right);
    __qmljs_init_boolean(ctx, result, r);
}

inline void __qmljs_ne(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = ! __qmljs_equal(ctx, left, right);
    __qmljs_init_boolean(ctx, result, r);
}

inline void __qmljs_se(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = __qmljs_strict_equal(ctx, left, right);
    __qmljs_init_boolean(ctx, result, r);
}

inline void __qmljs_sne(Context *ctx, Value *result, const Value *left, const Value *right)
{
    bool r = ! __qmljs_strict_equal(ctx, left, right);
    __qmljs_init_boolean(ctx, result, r);
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
    Q_ASSERT(!"unreachable");
    return false;
}

} // extern "C"

#endif // QMLJS_RUNTIME_H
