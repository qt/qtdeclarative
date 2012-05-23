
#include "qmljs_runtime.h"
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>
#include <typeinfo>

namespace QQmlJS {
namespace VM {

QString numberToString(double num, int radix = 10)
{
    if (qIsNaN(num)) {
        return QLatin1String("NaN");
    } else if (qIsInf(num)) {
        return QLatin1String(num < 0 ? "-Infinity" : "Infinity");
    }

    if (radix == 10)
        return QString::number(num, 'g', 16);

    QString str;
    bool negative = false;

    if (num < 0) {
        negative = true;
        num = -num;
    }

    double frac = num - ::floor(num);
    num = Value::toInteger(num);

    do {
        char c = (char)::fmod(num, radix);
        c = (c < 10) ? (c + '0') : (c - 10 + 'a');
        str.prepend(QLatin1Char(c));
        num = ::floor(num / radix);
    } while (num != 0);

    if (frac != 0) {
        str.append(QLatin1Char('.'));
        do {
            frac = frac * radix;
            char c = (char)::floor(frac);
            c = (c < 10) ? (c + '0') : (c - 10 + 'a');
            str.append(QLatin1Char(c));
            frac = frac - ::floor(frac);
        } while (frac != 0);
    }

    if (negative)
        str.prepend(QLatin1Char('-'));

    return str;
}


Value Value::fromString(Context *ctx, const QString &s)
{
    return fromString(ctx->engine->newString(s));
}

int Value::toInt32(double number)
{
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (int) trunc(number); // ###
}

uint Value::toUInt32(double number)
{
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (uint) trunc(number); // ###
}

int Value::toInteger(double number)
{
    if (isnan(number))
        return +0;
    else if (! number || isinf(number))
        return number;
    const double v = floor(fabs(number));
    return signbit(number) ? -v : v;
}

int Value::toUInt16(Context *ctx)
{
    return __qmljs_to_uint16(ctx, this);
}

int Value::toInt32(Context *ctx)
{
    return __qmljs_to_int32(ctx, this);
}

uint Value::toUInt32(Context *ctx)
{
    return __qmljs_to_int32(ctx, this);
}

bool Value::toBoolean(Context *ctx) const
{
    return __qmljs_to_boolean(ctx, this);
}

double Value::toInteger(Context *ctx) const
{
    return __qmljs_to_integer(ctx, this);
}

double Value::toNumber(Context *ctx) const
{
    return __qmljs_to_number(ctx, this);
}

String *Value::toString(Context *ctx) const
{
    Value v;
    __qmljs_to_string(ctx, &v, this);
    assert(v.is(STRING_TYPE));
    return v.stringValue;
}

bool Value::isFunctionObject() const
{
    return type == OBJECT_TYPE ? objectValue->asFunctionObject() != 0 : false;
}

bool Value::isBooleanObject() const
{
    return type == OBJECT_TYPE ? objectValue->asBooleanObject() != 0 : false;
}

bool Value::isNumberObject() const
{
    return type == OBJECT_TYPE ? objectValue->asNumberObject() != 0 : false;
}

bool Value::isStringObject() const
{
    return type == OBJECT_TYPE ? objectValue->asStringObject() != 0 : false;
}

bool Value::isDateObject() const
{
    return type == OBJECT_TYPE ? objectValue->asDateObject() != 0 : false;
}

bool Value::isArrayObject() const
{
    return type == OBJECT_TYPE ? objectValue->asArrayObject() != 0 : false;
}

bool Value::isErrorObject() const
{
    return type == OBJECT_TYPE ? objectValue->asErrorObject() != 0 : false;
}

bool Value::isArgumentsObject() const
{
    return type == OBJECT_TYPE ? objectValue->asArgumentsObject() != 0 : false;
}

Object *Value::asObject() const
{
    return type == OBJECT_TYPE ? objectValue : 0;
}

FunctionObject *Value::asFunctionObject() const
{
    return type == OBJECT_TYPE ? objectValue->asFunctionObject() : 0;
}

BooleanObject *Value::asBooleanObject() const
{
    return type == OBJECT_TYPE ? objectValue->asBooleanObject() : 0;
}

NumberObject *Value::asNumberObject() const
{
    return type == OBJECT_TYPE ? objectValue->asNumberObject() : 0;
}

StringObject *Value::asStringObject() const
{
    return type == OBJECT_TYPE ? objectValue->asStringObject() : 0;
}

DateObject *Value::asDateObject() const
{
    return type == OBJECT_TYPE ? objectValue->asDateObject() : 0;
}

ArrayObject *Value::asArrayObject() const
{
    return type == OBJECT_TYPE ? objectValue->asArrayObject() : 0;
}

ErrorObject *Value::asErrorObject() const
{
    return type == OBJECT_TYPE ? objectValue->asErrorObject() : 0;
}

ArgumentsObject *Value::asArgumentsObject() const
{
    return type == OBJECT_TYPE ? objectValue->asArgumentsObject() : 0;
}

extern "C" {

void __qmljs_init_closure(Context *ctx, Value *result, IR::Function *clos)
{
    __qmljs_init_object(result, ctx->engine->newScriptFunction(ctx, clos));
}

void __qmljs_string_literal_undefined(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("undefined")));
}

void __qmljs_string_literal_null(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("null")));
}

void __qmljs_string_literal_true(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("true")));
}

void __qmljs_string_literal_false(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("false")));
}

void __qmljs_string_literal_object(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("object")));
}

void __qmljs_string_literal_boolean(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("boolean")));
}

void __qmljs_string_literal_number(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("number")));
}

void __qmljs_string_literal_string(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("string")));
}

void __qmljs_string_literal_function(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QLatin1String("function")));
}

void __qmljs_delete(Context *ctx, Value *result, const Value *value)
{
    Q_UNIMPLEMENTED();
    (void) ctx;
    (void) result;
    (void) value;
    assert(!"TODO");
}

void __qmljs_instanceof(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (right->type == OBJECT_TYPE) {
        if (FunctionObject *function = right->objectValue->asFunctionObject()) {
            bool r = function->hasInstance(*left);
            __qmljs_init_boolean(result, r);
            return;
        }
    }

    __qmljs_throw_type_error(ctx, result);
}

void __qmljs_in(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (right->type == OBJECT_TYPE) {
        Value s;
        __qmljs_to_string(ctx, &s, left);
        bool r = right->objectValue->hasProperty(s.stringValue);
        __qmljs_init_boolean(result, r);
    } else {
        __qmljs_throw_type_error(ctx, result);
    }
}

int __qmljs_string_length(Context *, String *string)
{
    return string->toQString().length();
}

double __qmljs_string_to_number(Context *, String *string)
{
    bool ok;
    return string->toQString().toDouble(&ok); // ### TODO
}

void __qmljs_string_from_number(Context *ctx, Value *result, double number)
{
    String *string = ctx->engine->newString(numberToString(number, 10));
    __qmljs_init_string(result, string);
}

bool __qmljs_string_compare(Context *, String *left, String *right)
{
    return left->toQString() < right->toQString();
}

bool __qmljs_string_equal(Context *, String *left, String *right)
{
    return left == right ||
            (left->hashValue() == right->hashValue() &&
             left->toQString() == right->toQString());
}

String *__qmljs_string_concat(Context *ctx, String *first, String *second)
{
    return ctx->engine->newString(first->toQString() + second->toQString());
}

bool __qmljs_is_function(Context *, const Value *value)
{
    return value->objectValue->asFunctionObject() != 0;
}

void __qmljs_object_default_value(Context *ctx, Value *result, const Value *object, int typeHint)
{
    if (typeHint == PREFERREDTYPE_HINT) {
        if (object->isDateObject())
            typeHint = STRING_HINT;
        else
            typeHint = NUMBER_HINT;
    }

    String *meth1 = ctx->engine->identifier("toString");
    String *meth2 = ctx->engine->identifier("valueOf");

    if (typeHint == NUMBER_HINT)
        qSwap(meth1, meth2);

    Object *oo = object->asObject();
    assert(oo != 0);
    Value *conv = oo->getProperty(meth1);
    if (conv && conv->isFunctionObject()) {
        Value r;
        __qmljs_call_value(ctx, &r, object, conv, 0, 0);
        if (r.isPrimitive()) {
            *result = r;
            return;
        }
    }

    conv = object->asObject()->getProperty(meth2);
    if (conv && conv->isFunctionObject()) {
        Value r;
        __qmljs_call_value(ctx, &r, object, conv, 0, 0);
        if (r.isPrimitive()) {
            *result = r;
            return;
        }
    }

    __qmljs_init_undefined(result);
}

void __qmljs_throw_type_error(Context *ctx, Value *result)
{
    __qmljs_init_object(result, ctx->engine->newErrorObject(Value::fromString(ctx, QLatin1String("type error"))));
}

void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean)
{
    Value value;
    __qmljs_init_boolean(&value, boolean);
    __qmljs_init_object(result, ctx->engine->newBooleanObject(value));
    result->objectValue->prototype = ctx->engine->objectPrototype.objectValue;
}

void __qmljs_new_number_object(Context *ctx, Value *result, double number)
{
    Value value;
    __qmljs_init_number(&value, number);
    __qmljs_init_object(result, ctx->engine->newNumberObject(value));
    result->objectValue->prototype = ctx->engine->numberPrototype.objectValue;
}

void __qmljs_new_string_object(Context *ctx, Value *result, String *string)
{
    Value value;
    __qmljs_init_string(&value, string);
    __qmljs_init_object(result, ctx->engine->newStringObject(value));
    result->objectValue->prototype = ctx->engine->stringPrototype.objectValue;
}

void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value)
{
    Q_UNUSED(ctx);
    object->objectValue->put(name, *value, /*flags*/ 0);
}

void __qmljs_set_property_boolean(Context *ctx, Value *object, String *name, bool number)
{
    Q_UNUSED(ctx);
    Value value;
    __qmljs_init_boolean(&value, number);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double number)
{
    Q_UNUSED(ctx);
    Value value;
    __qmljs_init_number(&value, number);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *s)
{
    Q_UNUSED(ctx);
    Value value;
    __qmljs_init_string(&value, s);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_closure(Context *ctx, Value *object, String *name, IR::Function *function)
{
    Value value;
    __qmljs_init_closure(ctx, &value, function);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_get_element(Context *ctx, Value *result, Value *object, Value *index)
{
    if (object->isString() && index->isNumber()) {
        const QString s = object->stringValue->toQString().mid(Value::toUInt32(index->numberValue), 1);
        if (s.isNull())
            __qmljs_init_undefined(result);
        else
            *result = Value::fromString(ctx, s);
    } else if (object->isArrayObject() && index->isNumber()) {
        *result = object->asArrayObject()->value.at(Value::toUInt32(index->numberValue));
    } else {
        String *name = index->toString(ctx);

        if (! object->isObject())
            __qmljs_to_object(ctx, object, object);

        object->objectValue->get(name, result);
    }
}

void __qmljs_set_element(Context *ctx, Value *object, Value *index, Value *value)
{
    if (object->isArrayObject() && index->isNumber()) {
        object->asArrayObject()->value.assign(Value::toUInt32(index->numberValue), *value);
    } else {
        String *name = index->toString(ctx);

        if (! object->isObject())
            __qmljs_to_object(ctx, object, object);

        object->objectValue->put(name, *value, /*flags*/ 0);
    }
}

void __qmljs_set_element_number(Context *ctx, Value *object, Value *index, double number)
{
    Value v;
    __qmljs_init_number(&v, number);
    __qmljs_set_element(ctx, object, index, &v);
}

void __qmljs_set_activation_element(Context *ctx, String *name, Value *index, Value *value)
{
    if (Value *base = ctx->lookup(name)) {
        __qmljs_set_element(ctx, base, index, value);
    } else {
        assert(!"reference error");
    }
}

void __qmljs_set_activation_element_number(Context *ctx, String *name, Value *index, double number)
{
    Value v;
    __qmljs_init_number(&v, number);
    __qmljs_set_activation_element(ctx, name, index, &v);
}

void __qmljs_set_activation_property(Context *ctx, String *name, Value *value)
{
    if (Value *prop = ctx->lookup(name)) {
        *prop = *value;
    } else
        ctx->engine->globalObject.objectValue->put(name, *value);
}

void __qmljs_copy_activation_property(Context *ctx, String *name, String *other)
{
    if (Value *source = ctx->lookup(other))
        __qmljs_set_activation_property(ctx, name, source);
    else
        assert(!"reference error");
}

void __qmljs_set_activation_property_boolean(Context *ctx, String *name, bool b)
{
    Value value;
    __qmljs_init_boolean(&value, b);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_set_activation_property_number(Context *ctx, String *name, double number)
{
    Value value;
    __qmljs_init_number(&value, number);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_set_activation_property_string(Context *ctx, String *name, String *string)
{
    Value value;
    __qmljs_init_string(&value, string);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_set_activation_property_closure(Context *ctx, String *name, IR::Function *clos)
{
    Value value;
    __qmljs_init_closure(ctx, &value, clos);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_get_property(Context *ctx, Value *result, Value *object, String *name)
{
    if (object->type == OBJECT_TYPE) {
        object->objectValue->get(name, result);
    } else {
        Value o;
        __qmljs_to_object(ctx, &o, object);
        assert(o.type == OBJECT_TYPE);
        __qmljs_get_property(ctx, result, &o, name);
    }
}

void __qmljs_get_activation_property(Context *ctx, Value *result, String *name)
{
    if (Value *prop = ctx->lookup(name))
        *result = *prop;
    else
        assert(!"reference error");
}

void __qmljs_get_activation(Context *ctx, Value *result)
{
    *result = ctx->activation;
}

void __qmljs_get_thisObject(Context *ctx, Value *result)
{
    *result = ctx->thisObject;
}

void __qmljs_copy_property(Context *ctx, Value *target, String *name, Value *source, String *other)
{
    Q_UNUSED(ctx);
    Value v;
    source->objectValue->get(other, &v);
    target->objectValue->put(name, v);
}

void __qmljs_compare(Context *ctx, Value *result, const Value *x, const Value *y, bool leftFirst)
{
    Value px, py;

    if (leftFirst) {
        __qmljs_to_primitive(ctx, &px, x, NUMBER_HINT);
        __qmljs_to_primitive(ctx, &py, y, NUMBER_HINT);
    } else {
        __qmljs_to_primitive(ctx, &py, x, NUMBER_HINT);
        __qmljs_to_primitive(ctx, &px, y, NUMBER_HINT);
    }

    if (px.type == STRING_TYPE && py.type == STRING_TYPE) {
        bool r = __qmljs_string_compare(ctx, px.stringValue, py.stringValue);
        __qmljs_init_boolean(result, r);
    } else {
        double nx = __qmljs_to_number(ctx, &px);
        double ny = __qmljs_to_number(ctx, &py);
        if (isnan(nx) || isnan(ny)) {
            __qmljs_init_undefined(result);
        } else {
            __qmljs_init_boolean(result, nx < ny);
        }
    }
}

bool __qmljs_equal(Context *ctx, const Value *x, const Value *y)
{
    if (x->type == y->type) {
        switch ((ValueType) x->type) {
        case UNDEFINED_TYPE:
            return true;
        case NULL_TYPE:
            return true;
        case BOOLEAN_TYPE:
            return x->booleanValue == y->booleanValue;
            break;
        case NUMBER_TYPE:
            return x->numberValue == y->numberValue;
        case STRING_TYPE:
            return __qmljs_string_equal(ctx, x->stringValue, y->stringValue);
        case OBJECT_TYPE:
            return x->objectValue == y->objectValue;
        }
        // unreachable
    } else {
        if (x->type == NULL_TYPE && y->type == UNDEFINED_TYPE) {
            return true;
        } else if (x->type == UNDEFINED_TYPE && y->type == NULL_TYPE) {
            return true;
        } else if (x->type == NUMBER_TYPE && y->type == STRING_TYPE) {
            Value ny;
            __qmljs_init_number(&ny, __qmljs_to_number(ctx, y));
            return __qmljs_equal(ctx, x, &ny);
        } else if (x->type == STRING_TYPE && y->type == NUMBER_TYPE) {
            Value nx;
            __qmljs_init_number(&nx, __qmljs_to_number(ctx, x));
            return __qmljs_equal(ctx, &nx, y);
        } else if (x->type == BOOLEAN_TYPE) {
            Value nx;
            __qmljs_init_number(&nx, (double) x->booleanValue);
            return __qmljs_equal(ctx, &nx, y);
        } else if (y->type == BOOLEAN_TYPE) {
            Value ny;
            __qmljs_init_number(&ny, (double) y->booleanValue);
            return __qmljs_equal(ctx, x, &ny);
        } else if ((x->type == NUMBER_TYPE || x->type == STRING_TYPE) && y->type == OBJECT_TYPE) {
            Value py;
            __qmljs_to_primitive(ctx, &py, y, PREFERREDTYPE_HINT);
            return __qmljs_equal(ctx, x, &py);
        } else if (x->type == OBJECT_TYPE && (y->type == NUMBER_TYPE || y->type == STRING_TYPE)) {
            Value px;
            __qmljs_to_primitive(ctx, &px, x, PREFERREDTYPE_HINT);
            return __qmljs_equal(ctx, &px, y);
        }
    }

    return false;
}

void __qmljs_call_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    Value *func = context->lookup(name);
    if (! func)
        assert(!"reference error");

    __qmljs_call_value(context, result, /*thisObject=*/ 0, func, args, argc);
}

void __qmljs_call_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    Value baseObject;
    Value thisObject;
    if (base) {
        baseObject = *base;
        if (baseObject.type != OBJECT_TYPE)
            __qmljs_to_object(context, &baseObject, &baseObject);
        assert(baseObject.type == OBJECT_TYPE);
        thisObject = baseObject;
    } else {
        baseObject = context->activation;
        __qmljs_init_null(&thisObject);
    }
    Value func;
    baseObject.objectValue->get(name, &func);
    if (func.type == OBJECT_TYPE) {
        if (FunctionObject *f = func.objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->initCallContext(context->engine, &thisObject, f, args, argc);
            f->call(ctx);
            if (ctx->hasUncaughtException) {
                context->hasUncaughtException = ctx->hasUncaughtException; // propagate the exception
                context->result = ctx->result;
            }
            ctx->leaveCallContext(f, result);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

void __qmljs_call_value(Context *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc)
{
    if (func->type == OBJECT_TYPE) {
        if (FunctionObject *f = func->objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->initCallContext(context->engine, thisObject, f, args, argc);
            f->call(ctx);
            if (ctx->hasUncaughtException) {
                context->hasUncaughtException = ctx->hasUncaughtException; // propagate the exception
                context->result = ctx->result;
            }
            ctx->leaveCallContext(f, result);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

void __qmljs_construct_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    Value *func = context->lookup(name);
    if (! func)
        assert(!"reference error");

    __qmljs_construct_value(context, result, func, args, argc);
}

void __qmljs_construct_value(Context *context, Value *result, const Value *func, Value *args, int argc)
{
    Q_UNUSED(context);
    if (func->type == OBJECT_TYPE) {
        if (FunctionObject *f = func->objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->initConstructorContext(context->engine, 0, f, args, argc);
            f->construct(ctx);
            if (ctx->hasUncaughtException) {
                context->hasUncaughtException = ctx->hasUncaughtException; // propagate the exception
                context->result = ctx->result;
            }
            ctx->leaveConstructorContext(f, result);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

void __qmljs_construct_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    Value func;
    Value thisObject = *base;
    if (thisObject.type != OBJECT_TYPE)
        __qmljs_to_object(context, &thisObject, base);

    assert(thisObject.type == OBJECT_TYPE);
    thisObject.objectValue->get(name, &func);
    if (func.type == OBJECT_TYPE) {
        if (FunctionObject *f = func.objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->initConstructorContext(context->engine, 0, f, args, argc);
            ctx->calledAsConstructor = true;
            f->construct(ctx);
            if (ctx->hasUncaughtException) {
                context->hasUncaughtException = ctx->hasUncaughtException; // propagate the exception
                context->result = ctx->result;
            }
            ctx->leaveConstructorContext(f, result);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

void __qmljs_builtin_typeof(Context *context, Value *result, Value *args, int argc)
{
    Q_UNUSED(argc);
    __qmljs_typeof(context, result, &args[0]);
}

void __qmljs_builtin_throw(Context *context, Value *result, Value *args, int argc)
{
    Q_UNUSED(result);
    Q_UNUSED(argc);
    context->result = args[0];
    context->hasUncaughtException = true;
}


} // extern "C"


} // namespace VM
} // namespace QQmlJS
