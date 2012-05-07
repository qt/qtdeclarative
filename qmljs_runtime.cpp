
#include "qmljs_runtime.h"
#include "qmljs_objects.h"
#include <QtCore/QDebug>
#include <cstdio>
#include <cassert>

namespace QQmlJS {
namespace VM {

Value Value::string(Context *ctx, const QString &s)
{
    return string(ctx, String::get(ctx, s));
}

extern "C" {

void __qmljs_string_literal_undefined(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("undefined")));
}

void __qmljs_string_literal_null(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("null")));
}

void __qmljs_string_literal_true(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("true")));
}

void __qmljs_string_literal_false(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("false")));
}

void __qmljs_string_literal_object(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("object")));
}

void __qmljs_string_literal_boolean(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("boolean")));
}

void __qmljs_string_literal_number(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("number")));
}

void __qmljs_string_literal_string(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("string")));
}

void __qmljs_string_literal_function(Context *ctx, Value *result)
{
    __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("function")));
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
    return string->text().length();
}

double __qmljs_string_to_number(Context *, String *string)
{
    bool ok;
    return string->text().toDouble(&ok); // ### TODO
}

void __qmljs_string_from_number(Context *ctx, Value *result, double number)
{
    String *string = String::get(ctx, QString::number(number, 'g', 16));
    __qmljs_init_string(ctx, result, string);
}

bool __qmljs_string_compare(Context *, String *left, String *right)
{
    return left->text() < right->text();
}

bool __qmljs_string_equal(Context *, String *left, String *right)
{
    return left == right ||
            (left->hashValue() == right->hashValue() &&
             left->text() == right->text());
}

String *__qmljs_string_concat(Context *ctx, String *first, String *second)
{
    return String::get(ctx, first->text() + second->text());
}

bool __qmljs_is_function(Context *, const Value *value)
{
    return value->objectValue->asFunctionObject() != 0;
}

void __qmljs_object_default_value(Context *ctx, Value *result, Object *object, int typeHint)
{
    Q_UNUSED(ctx);
    object->defaultValue(result, typeHint);
}

void __qmljs_throw_type_error(Context *ctx, Value *result)
{
    __qmljs_init_object(ctx, result, new ErrorObject(String::get(ctx, QLatin1String("type error"))));
}

void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean)
{
    Value value;
    __qmljs_init_boolean(ctx, &value, boolean);
    __qmljs_init_object(ctx, result, new BooleanObject(value));
}

void __qmljs_new_number_object(Context *ctx, Value *result, double number)
{
    Value value;
    __qmljs_init_number(ctx, &value, number);
    __qmljs_init_object(ctx, result, new NumberObject(value));
}

void __qmljs_new_string_object(Context *ctx, Value *result, String *string)
{
    Value value;
    __qmljs_init_string(ctx, &value, string);
    __qmljs_init_object(ctx, result, new StringObject(value));
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

void __qmljs_set_activation_property(Context *ctx, String *name, Value *value)
{
    __qmljs_set_property(ctx, &ctx->activation, name, value);
}

void __qmljs_copy_activation_property(Context *ctx, String *name, String *other)
{
    __qmljs_copy_property(ctx, &ctx->activation, name, &ctx->activation, other);
}

void __qmljs_set_activation_property_boolean(Context *ctx, String *name, bool value)
{
    __qmljs_set_property_boolean(ctx, &ctx->activation, name, value);
}

void __qmljs_set_activation_property_number(Context *ctx, String *name, double value)
{
    __qmljs_set_property_number(ctx, &ctx->activation, name, value);
}

void __qmljs_set_activation_property_string(Context *ctx, String *name, String *value)
{
    __qmljs_set_property_string(ctx, &ctx->activation, name, value);
}

void __qmljs_get_property(Context *ctx, Value *result, Value *object, String *name)
{
    Q_UNUSED(ctx);
    Q_ASSERT(object->type == OBJECT_TYPE);
    object->objectValue->get(name, result);
}

void __qmljs_get_activation_property(Context *ctx, Value *result, String *name)
{
    __qmljs_get_property(ctx, result, &ctx->activation, name);
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

Context *__qmljs_new_context(Context *current, Value *thisObject, size_t argc)
{
    Context *ctx = new Context;
    ctx->parent = current;
    ctx->scope = current->activation.objectValue;
    __qmljs_init_object(ctx, &ctx->activation, new ArgumentsObject(ctx));
    if (thisObject)
        ctx->thisObject = *thisObject;
    else
        __qmljs_init_null(ctx, &ctx->thisObject);
    ctx->arguments = new Value[argc];
    ctx->argumentCount = argc;
    return ctx;
}

void __qmljs_dispose_context(Context *ctx)
{
    delete[] ctx->arguments;
    delete ctx;
}

void __qmljs_call_activation_property(Context *context, Value *result, String *name)
{
    Value func;
    context->parent->activation.objectValue->get(name, &func);
    if (func.type == OBJECT_TYPE) {
        if (FunctionObject *f = func.objectValue->asFunctionObject()) {
            f->call(context);
            __qmljs_copy(result, &context->result);
        } else {
            Q_ASSERT(!"not a function");
        }
    } else {
        Q_ASSERT(!"not a callable object");
    }
}

} // extern "C"


} // namespace VM
} // namespace QQmlJS
