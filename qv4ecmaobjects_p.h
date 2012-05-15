#ifndef QV4ECMAOBJECTS_P_H
#define QV4ECMAOBJECTS_P_H

#include "qmljs_objects.h"

namespace QQmlJS {
namespace VM {

struct ObjectCtor: FunctionObject
{
    static Value create(ExecutionEngine *engine);

    ObjectCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *);
};

struct ObjectPrototype: Object
{
    ObjectPrototype(Context *ctx, FunctionObject *ctor);
};

struct StringCtor: FunctionObject
{
    static Value create(ExecutionEngine *engine);

    StringCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct StringPrototype: Object
{
    StringPrototype(Context *ctx, FunctionObject *ctor);

protected:
    static QString getThisString(Context *ctx);

    static void method_toString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_charAt(Context *ctx);
    static void method_charCodeAt(Context *ctx);
    static void method_concat(Context *ctx);
    static void method_indexOf(Context *ctx);
    static void method_lastIndexOf(Context *ctx);
    static void method_localeCompare(Context *ctx);
    static void method_match(Context *ctx);
    static void method_replace(Context *ctx);
    static void method_search(Context *ctx);
    static void method_slice(Context *ctx);
    static void method_split(Context *ctx);
    static void method_substr(Context *ctx);
    static void method_substring(Context *ctx);
    static void method_toLowerCase(Context *ctx);
    static void method_toLocaleLowerCase(Context *ctx);
    static void method_toUpperCase(Context *ctx);
    static void method_toLocaleUpperCase(Context *ctx);
    static void method_fromCharCode(Context *ctx);
};

struct NumberCtor: FunctionObject
{
    static Value create(ExecutionEngine *engine);

    NumberCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct NumberPrototype: Object
{
    NumberPrototype(Context *ctx, FunctionObject *ctor);

protected:
    static void method_toString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_toFixed(Context *ctx);
    static void method_toExponential(Context *ctx);
    static void method_toPrecision(Context *ctx);
};

struct BooleanCtor: FunctionObject
{
    static Value create(ExecutionEngine *engine);

    BooleanCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct BooleanPrototype: Object
{
    BooleanPrototype(Context *ctx, FunctionObject *ctor);

protected:
    static void method_toString(Context *ctx);
    static void method_valueOf(Context *ctx);
};

struct MathObject: Object
{
    MathObject(Context *ctx);

protected:
    static void method_abs(Context *ctx);
    static void method_acos(Context *ctx);
    static void method_asin(Context *ctx);
    static void method_atan(Context *ctx);
    static void method_atan2(Context *ctx);
    static void method_ceil(Context *ctx);
    static void method_cos(Context *ctx);
    static void method_exp(Context *ctx);
    static void method_floor(Context *ctx);
    static void method_log(Context *ctx);
    static void method_max(Context *ctx);
    static void method_min(Context *ctx);
    static void method_pow(Context *ctx);
    static void method_random(Context *ctx);
    static void method_round(Context *ctx);
    static void method_sin(Context *ctx);
    static void method_sqrt(Context *ctx);
    static void method_tan(Context *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
