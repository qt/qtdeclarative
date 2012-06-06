#include "qmljs_runtime.h"
#include <stdio.h>

using namespace QQmlJS::VM;

extern "C" {

void __qmljs_llvm_return(Context *ctx, Value *result)
{
    ctx->result = *result;
}

Value *__qmljs_llvm_get_argument(Context *ctx, int index)
{
    return &ctx->arguments[index];
}

void __qmljs_llvm_init_undefined(Value *result)
{
    __qmljs_init_undefined(result);
}

void __qmljs_llvm_init_null(Value *result)
{
    __qmljs_init_null(result);
}

void __qmljs_llvm_init_boolean(Value *result, bool value)
{
    __qmljs_init_boolean(result, value);
}

void __qmljs_llvm_init_number(Value *result, double value)
{
    __qmljs_init_number(result, value);
}

void __qmljs_llvm_init_string(Context *ctx, Value *result, const char *str)
{
    __qmljs_init_string(result, __qmljs_string_from_utf8(ctx, str));
}

bool __qmljs_llvm_to_boolean(Context *ctx, const Value *value)
{
    return __qmljs_to_boolean(ctx, value);
}

void __qmljs_llvm_bit_and(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_and(ctx, result, left, right);
}

void __qmljs_llvm_bit_or(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_or(ctx, result, left, right);
}

void __qmljs_llvm_bit_xor(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_bit_xor(ctx, result, left, right);
}

void __qmljs_llvm_add(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_add(ctx, result, left, right);
}

void __qmljs_llvm_sub(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_sub(ctx, result, left, right);
}

void __qmljs_llvm_mul(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_mul(ctx, result, left, right);
}

void __qmljs_llvm_div(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_div(ctx, result, left, right);
}

void __qmljs_llvm_mod(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_mod(ctx, result, left, right);
}

void __qmljs_llvm_shl(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_shl(ctx, result, left, right);
}

void __qmljs_llvm_shr(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_shr(ctx, result, left, right);
}

void __qmljs_llvm_ushr(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ushr(ctx, result, left, right);
}

void __qmljs_llvm_gt(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_gt(ctx, result, left, right);
}

void __qmljs_llvm_lt(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_lt(ctx, result, left, right);
}

void __qmljs_llvm_ge(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ge(ctx, result, left, right);
}

void __qmljs_llvm_le(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_le(ctx, result, left, right);
}

void __qmljs_llvm_eq(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_eq(ctx, result, left, right);
}

void __qmljs_llvm_ne(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_ne(ctx, result, left, right);
}

void __qmljs_llvm_se(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_se(ctx, result, left, right);
}

void __qmljs_llvm_sne(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_sne(ctx, result, left, right);
}

void __qmljs_llvm_instanceof(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_instanceof(ctx, result, left, right);
}

void __qmljs_llvm_in(Context *ctx, Value *result, Value *left, Value *right)
{
    __qmljs_in(ctx, result, left, right);
}

void __qmljs_llvm_uplus(Context *ctx, Value *result, const Value *value)
{
    __qmljs_uplus(ctx, result, value);
}

void __qmljs_llvm_uminus(Context *ctx, Value *result, const Value *value)
{
    __qmljs_uminus(ctx, result, value);
}

void __qmljs_llvm_compl(Context *ctx, Value *result, const Value *value)
{
    __qmljs_compl(ctx, result, value);
}

void __qmljs_llvm_not(Context *ctx, Value *result, const Value *value)
{
    __qmljs_not(ctx, result, value);
}

String *__qmljs_llvm_get_identifier(Context *ctx, const char *str)
{
    return __qmljs_string_from_utf8(ctx, str); // ### make it unique
}

void __qmljs_llvm_call_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_call_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_construct_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    __qmljs_construct_activation_property(context, result, name, args, argc);
}

void __qmljs_llvm_get_activation_property(Context *ctx, Value *result, String *name)
{
    __qmljs_get_activation_property(ctx, result, name);
}

void __qmljs_llvm_set_activation_property(Context *ctx, String *name, Value *value)
{
    __qmljs_set_activation_property(ctx, name, value);
}

void __qmljs_llvm_get_property(Context *ctx, Value *result, Value *object, String *name)
{
    __qmljs_get_property(ctx, result, object, name);
}

void __qmljs_llvm_call_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    __qmljs_call_property(context, result, base, name, args, argc);
}

void __qmljs_llvm_construct_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    __qmljs_construct_property(context, result, base, name, args, argc);
}

void __qmljs_llvm_get_element(Context *ctx, Value *result, Value *object, Value *index)
{
    __qmljs_get_element(ctx, result, object, index);
}

void __qmljs_llvm_set_element(Context *ctx, Value *object, Value *index, Value *value)
{
    __qmljs_set_element(ctx, object, index, value);
}

void __qmljs_llvm_set_property(Context *ctx, Value *object, String *name, Value *value)
{
    __qmljs_set_property(ctx, object, name, value);
}

} // extern "C"
