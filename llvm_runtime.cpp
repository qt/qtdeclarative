#include "qmljs_runtime.h"

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

} // extern "C"
