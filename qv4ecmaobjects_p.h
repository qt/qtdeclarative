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

struct DateCtor: FunctionObject
{
    static Value create(ExecutionEngine *engine);

    DateCtor(Context *scope);

    virtual void construct(Context *ctx);
    virtual void call(Context *ctx);
};

struct DatePrototype: DateObject
{
    DatePrototype(Context *ctx, FunctionObject *ctor);

protected:
    static DateObject *getThisDateObject(Context *ctx);
    static double getThisDate(Context *ctx);

    static void method_MakeTime(Context *ctx);
    static void method_MakeDate(Context *ctx);
    static void method_TimeClip(Context *ctx);
    static void method_parse(Context *ctx);
    static void method_UTC(Context *ctx);
    static void method_toString(Context *ctx);
    static void method_toDateString(Context *ctx);
    static void method_toTimeString(Context *ctx);
    static void method_toLocaleString(Context *ctx);
    static void method_toLocaleDateString(Context *ctx);
    static void method_toLocaleTimeString(Context *ctx);
    static void method_valueOf(Context *ctx);
    static void method_getTime(Context *ctx);
    static void method_getYear(Context *ctx);
    static void method_getFullYear(Context *ctx);
    static void method_getUTCFullYear(Context *ctx);
    static void method_getMonth(Context *ctx);
    static void method_getUTCMonth(Context *ctx);
    static void method_getDate(Context *ctx);
    static void method_getUTCDate(Context *ctx);
    static void method_getDay(Context *ctx);
    static void method_getUTCDay(Context *ctx);
    static void method_getHours(Context *ctx);
    static void method_getUTCHours(Context *ctx);
    static void method_getMinutes(Context *ctx);
    static void method_getUTCMinutes(Context *ctx);
    static void method_getSeconds(Context *ctx);
    static void method_getUTCSeconds(Context *ctx);
    static void method_getMilliseconds(Context *ctx);
    static void method_getUTCMilliseconds(Context *ctx);
    static void method_getTimezoneOffset(Context *ctx);
    static void method_setTime(Context *ctx);
    static void method_setMilliseconds(Context *ctx);
    static void method_setUTCMilliseconds(Context *ctx);
    static void method_setSeconds(Context *ctx);
    static void method_setUTCSeconds(Context *ctx);
    static void method_setMinutes(Context *ctx);
    static void method_setUTCMinutes(Context *ctx);
    static void method_setHours(Context *ctx);
    static void method_setUTCHours(Context *ctx);
    static void method_setDate(Context *ctx);
    static void method_setUTCDate(Context *ctx);
    static void method_setMonth(Context *ctx);
    static void method_setUTCMonth(Context *ctx);
    static void method_setYear(Context *ctx);
    static void method_setFullYear(Context *ctx);
    static void method_setUTCFullYear(Context *ctx);
    static void method_toUTCString(Context *ctx);
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
