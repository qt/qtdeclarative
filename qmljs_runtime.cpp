
#include "qmljs_runtime.h"
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>

namespace QQmlJS {
namespace VM {

Value Value::string(Context *ctx, const QString &s)
{
    return string(ctx, String::get(ctx, s));
}

int Value::toInt32(double number)
{
    if (! number || isnan(number) || isinf(number))
        return +0;
    return (int) trunc(number); // ###
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


extern "C" {

void __qmljs_init_closure(Context *ctx, Value *result, IR::Function *clos)
{
    __qmljs_init_object(ctx, result, ctx->engine->newScriptFunction(ctx, clos));
}

void __qmljs_string_literal_undefined(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("undefined")));
}

void __qmljs_string_literal_null(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("null")));
}

void __qmljs_string_literal_true(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("true")));
}

void __qmljs_string_literal_false(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("false")));
}

void __qmljs_string_literal_object(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("object")));
}

void __qmljs_string_literal_boolean(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("boolean")));
}

void __qmljs_string_literal_number(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("number")));
}

void __qmljs_string_literal_string(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("string")));
}

void __qmljs_string_literal_function(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("function")));
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
            __qmljs_init_boolean(ctx, result, r);
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
        __qmljs_init_boolean(ctx, result, r);
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
    String *string = String::get(ctx, QString::number(number, 'g', 16));
    __qmljs_init_string(ctx, result, string);
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
    return String::get(ctx, first->toQString() + second->toQString());
}

bool __qmljs_is_function(Context *, const Value *value)
{
    return value->objectValue->asFunctionObject() != 0;
}

void __qmljs_object_default_value(Context *ctx, Value *result, Object *object, int typeHint)
{
    Q_UNUSED(ctx);
    object->defaultValue(ctx, result, typeHint);
}

void __qmljs_throw_type_error(Context *ctx, Value *result)
{
    __qmljs_init_object(ctx, result, ctx->engine->newErrorObject(Value::string(ctx, QLatin1String("type error"))));
}

void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean)
{
    Value value;
    __qmljs_init_boolean(ctx, &value, boolean);
    __qmljs_init_object(ctx, result, ctx->engine->newBooleanObject(value));
}

void __qmljs_new_number_object(Context *ctx, Value *result, double number)
{
    Value value;
    __qmljs_init_number(ctx, &value, number);
    __qmljs_init_object(ctx, result, ctx->engine->newNumberObject(value));
    result->objectValue->prototype = ctx->engine->numberPrototype.objectValue;
}

void __qmljs_new_string_object(Context *ctx, Value *result, String *string)
{
    Value value;
    __qmljs_init_string(ctx, &value, string);
    __qmljs_init_object(ctx, result, ctx->engine->newStringObject(value));
    result->objectValue->prototype = ctx->engine->stringPrototype.objectValue;
}

void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value)
{
    Q_UNUSED(ctx);
    object->objectValue->put(name, *value, /*flags*/ 0);
}

void __qmljs_set_property_boolean(Context *ctx, Value *object, String *name, bool number)
{
    Value value;
    __qmljs_init_boolean(ctx, &value, number);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double number)
{
    Value value;
    __qmljs_init_number(ctx, &value, number);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *s)
{
    Value value;
    __qmljs_init_string(ctx, &value, s);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_property_closure(Context *ctx, Value *object, String *name, IR::Function *function)
{
    Value value;
    __qmljs_init_closure(ctx, &value, function);
    object->objectValue->put(name, value, /*flag*/ 0);
}

void __qmljs_set_activation_property(Context *ctx, String *name, Value *value)
{
    if (Value *prop = ctx->lookup(name))
        __qmljs_copy(prop, value);
    else
        ctx->activation.objectValue->put(name, *value);
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
    __qmljs_init_boolean(ctx, &value, b);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_set_activation_property_number(Context *ctx, String *name, double number)
{
    Value value;
    __qmljs_init_number(ctx, &value, number);
    __qmljs_set_activation_property(ctx, name, &value);
}

void __qmljs_set_activation_property_string(Context *ctx, String *name, String *string)
{
    Value value;
    __qmljs_init_string(ctx, &value, string);
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
        __qmljs_init_boolean(ctx, result, r);
    } else {
        double nx = __qmljs_to_number(ctx, &px);
        double ny = __qmljs_to_number(ctx, &py);
        if (isnan(nx) || isnan(ny)) {
            __qmljs_init_undefined(ctx, result);
        } else {
            __qmljs_init_boolean(ctx, result, nx < ny);
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
            __qmljs_init_number(ctx, &ny, __qmljs_to_number(ctx, y));
            return __qmljs_equal(ctx, x, &ny);
        } else if (x->type == STRING_TYPE && y->type == NUMBER_TYPE) {
            Value nx;
            __qmljs_init_number(ctx, &nx, __qmljs_to_number(ctx, x));
            return __qmljs_equal(ctx, &nx, y);
        } else if (x->type == BOOLEAN_TYPE) {
            Value nx;
            __qmljs_init_number(ctx, &nx, (double) x->booleanValue);
            return __qmljs_equal(ctx, &nx, y);
        } else if (y->type == BOOLEAN_TYPE) {
            Value ny;
            __qmljs_init_number(ctx, &ny, (double) y->booleanValue);
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

    __qmljs_call_value(context, result, func, args, argc);
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
        __qmljs_init_null(context, &thisObject);
    }
    Value func;
    baseObject.objectValue->get(name, &func);
    if (func.type == OBJECT_TYPE) {
        if (FunctionObject *f = func.objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->init(context->engine);
            ctx->parent = f->scope;
            if (f->needsActivation)
                __qmljs_init_object(ctx, &ctx->activation, ctx->engine->newArgumentsObject(ctx));
            else
                __qmljs_init_null(ctx, &ctx->activation);
            ctx->thisObject = thisObject;
            ctx->formals = f->formalParameterList;
            ctx->formalCount = f->formalParameterCount;
            ctx->arguments = args;
            ctx->argumentCount = argc;
            if (argc && f->needsActivation) {
                ctx->arguments = new Value[argc];
                std::copy(args, args + argc, ctx->arguments);
            }
            f->call(ctx);
            if (result)
                __qmljs_copy(result, &ctx->result);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

void __qmljs_call_value(Context *context, Value *result, const Value *func, Value *args, int argc)
{
    Q_UNUSED(context);

    if (func->type == OBJECT_TYPE) {
        if (FunctionObject *f = func->objectValue->asFunctionObject()) {
            Context k;
            Context *ctx = f->needsActivation ? context->engine->newContext() : &k;
            ctx->init(context->engine);
            ctx->parent = f->scope;
            if (f->needsActivation)
                __qmljs_init_object(ctx, &ctx->activation, ctx->engine->newArgumentsObject(ctx));
            else
                __qmljs_init_null(ctx, &ctx->activation);
            __qmljs_init_null(ctx, &ctx->thisObject);
            ctx->formals = f->formalParameterList;
            ctx->formalCount = f->formalParameterCount;
            ctx->arguments = args;
            ctx->argumentCount = argc;
            if (argc && f->needsActivation) {
                ctx->arguments = new Value[argc];
                std::copy(args, args + argc, ctx->arguments);
            }
            f->call(ctx);
            if (result)
                __qmljs_copy(result, &ctx->result);
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
            ctx->init(context->engine);
            ctx->parent = f->scope;
            if (f->needsActivation)
                __qmljs_init_object(ctx, &ctx->activation, ctx->engine->newArgumentsObject(ctx));
            else
                __qmljs_init_null(ctx, &ctx->activation);
            __qmljs_init_null(ctx, &ctx->thisObject);
            ctx->formals = f->formalParameterList;
            ctx->formalCount = f->formalParameterCount;
            ctx->arguments = args;
            ctx->argumentCount = argc;
            if (argc && f->needsActivation) {
                ctx->arguments = new Value[argc];
                std::copy(args, args + argc, ctx->arguments);
            }
            ctx->calledAsConstructor = true;
            f->construct(ctx);
            assert(ctx->thisObject.is(OBJECT_TYPE));
            ctx->result = ctx->thisObject;
            Value proto;
            if (f->get(ctx->engine->identifier(QLatin1String("prototype")), &proto)) { // ### `prototype' should be a unique symbol
                if (proto.type == OBJECT_TYPE)
                    ctx->thisObject.objectValue->prototype = proto.objectValue;
            }

            if (result)
                __qmljs_copy(result, &ctx->thisObject);
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
            ctx->init(context->engine);
            ctx->parent = f->scope;
            if (f->needsActivation)
                __qmljs_init_object(ctx, &ctx->activation, ctx->engine->newArgumentsObject(ctx));
            else
                __qmljs_init_null(ctx, &ctx->activation);
            ctx->thisObject = thisObject;
            ctx->formals = f->formalParameterList;
            ctx->formalCount = f->formalParameterCount;
            ctx->arguments = args;
            ctx->argumentCount = argc;
            if (argc && f->needsActivation) {
                ctx->arguments = new Value[argc];
                std::copy(args, args + argc, ctx->arguments);
            }
            ctx->calledAsConstructor = true;
            f->construct(ctx);
            assert(ctx->thisObject.is(OBJECT_TYPE));

            Value proto;
            if (f->get(ctx->engine->identifier(QLatin1String("prototype")), &proto)) { // ### `prototype' should be a unique symbol
                if (proto.type == OBJECT_TYPE)
                    ctx->thisObject.objectValue->prototype = proto.objectValue;
            }

            if (result)
                __qmljs_copy(result, &ctx->thisObject);
        } else {
            assert(!"not a function");
        }
    } else {
        assert(!"not a callable object");
    }
}

} // extern "C"


} // namespace VM
} // namespace QQmlJS
