
#include "qmljs_runtime.h"
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"

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
        return QStringLiteral("NaN");
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

unsigned int Value::toUInt32(double number)
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

unsigned int Value::toUInt32(Context *ctx)
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

Value Value::toObject(Context *ctx) const
{
    Value v;
    __qmljs_to_object(ctx, &v, this);
    return v;
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
    return type == OBJECT_TYPE ? objectValue->asActivationObject() != 0 : false;
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

ActivationObject *Value::asArgumentsObject() const
{
    return type == OBJECT_TYPE ? objectValue->asActivationObject() : 0;
}

Value Value::property(Context *ctx, String *name) const
{
    return isObject() ? objectValue->getProperty(ctx, name) : undefinedValue();
}

Value *Value::getPropertyDescriptor(Context *ctx, String *name) const
{
    return isObject() ? objectValue->getPropertyDescriptor(ctx, name) : 0;
}

void Context::init(ExecutionEngine *eng)
{
    engine = eng;
    parent = 0;
    arguments = 0;
    argumentCount = 0;
    locals = 0;
    activation.type = NULL_TYPE;
    thisObject.type = NULL_TYPE;
    result.type = UNDEFINED_TYPE;
    formals = 0;
    formalCount = 0;
    vars = 0;
    varCount = 0;
    calledAsConstructor = false;
    hasUncaughtException = false;
}

Value *Context::lookupPropertyDescriptor(String *name)
{
    for (Context *ctx = this; ctx; ctx = ctx->parent) {
        if (ctx->activation.is(OBJECT_TYPE)) {
            if (Value *prop = ctx->activation.objectValue->getPropertyDescriptor(this, name)) {
                return prop;
            }
        }
    }
    return 0;
}

void Context::throwError(const Value &value)
{
    result = value;
    hasUncaughtException = true;
}

void Context::throwError(const QString &message)
{
    Value v = Value::fromString(this, message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwTypeError()
{
    Value v = Value::fromString(this, QStringLiteral("Type error"));
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwUnimplemented(const QString &message)
{
    Value v = Value::fromString(this, QStringLiteral("Unimplemented ") + message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwReferenceError(const Value &value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newErrorObject(Value::fromString(this, msg))));
}

void Context::initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, unsigned argc)
{
    engine = e;
    parent = f->scope;

    if (f->needsActivation)
        __qmljs_init_object(&activation, engine->newActivationObject(this));
    else
        __qmljs_init_null(&activation);

    if (object)
        thisObject = *object;
    else
        __qmljs_init_null(&thisObject);

    formals = f->formalParameterList;
    formalCount = f->formalParameterCount;
    arguments = args;
    argumentCount = argc;
    if (f->needsActivation || argc < formalCount){
        arguments = new Value[qMax(argc, formalCount)];
        if (argc)
            std::copy(args, args + argc, arguments);
        if (argc < formalCount)
            std::fill(arguments + argc, arguments + formalCount, Value::undefinedValue());
    }
    vars = f->varList;
    varCount = f->varCount;
    locals = varCount ? new Value[varCount] : 0;
    hasUncaughtException = false;
    calledAsConstructor = false;
    if (varCount)
        std::fill(locals, locals + varCount, Value::undefinedValue());
}

void Context::leaveCallContext(FunctionObject *f, Value *returnValue)
{
    if (returnValue)
        __qmljs_copy(returnValue, &result);

    if (! f->needsActivation) {
        delete[] locals;
        locals = 0;
    }
}

void Context::initConstructorContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, unsigned argc)
{
    initCallContext(e, object, f, args, argc);
    calledAsConstructor = true;
}

void Context::leaveConstructorContext(FunctionObject *f, Value *returnValue)
{
    assert(thisObject.is(OBJECT_TYPE));
    result = thisObject;

    Value proto = f->getProperty(this, engine->id_prototype);
    thisObject.objectValue->prototype = proto.objectValue;
    if (! thisObject.isObject())
        thisObject.objectValue->prototype = engine->objectPrototype;

    leaveCallContext(f, returnValue);
}

extern "C" {

void __qmljs_init_closure(Context *ctx, Value *result, IR::Function *clos)
{
    __qmljs_init_object(result, ctx->engine->newScriptFunction(ctx, clos));
}

void __qmljs_init_native_function(Context *ctx, Value *result, void (*code)(Context *))
{
    __qmljs_init_object(result, ctx->engine->newNativeFunction(ctx, code));
}

void __qmljs_string_literal_undefined(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("undefined")));
}

void __qmljs_string_literal_null(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("null")));
}

void __qmljs_string_literal_true(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("true")));
}

void __qmljs_string_literal_false(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("false")));
}

void __qmljs_string_literal_object(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("object")));
}

void __qmljs_string_literal_boolean(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("boolean")));
}

void __qmljs_string_literal_number(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("number")));
}

void __qmljs_string_literal_string(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("string")));
}

void __qmljs_string_literal_function(Context *ctx, Value *result)
{
    __qmljs_init_string(result, ctx->engine->identifier(QStringLiteral("function")));
}

void __qmljs_delete_subscript(Context *ctx, Value *result, Value *base, Value *index)
{
    if (ArrayObject *a = base->asArrayObject()) {
        if (index->isNumber()) {
            const quint32 n = index->numberValue;
            if (n < a->value.size()) {
                a->value.assign(n, Value::undefinedValue());
                __qmljs_init_boolean(result, true);
                return;
            }
        }
    }

    String *name = index->toString(ctx);
    __qmljs_delete_member(ctx, result, base, name);
}

void __qmljs_delete_member(Context *ctx, Value *result, Value *base, String *name)
{
    Value obj = base->toObject(ctx);
    __qmljs_init_boolean(result, obj.objectValue->deleteProperty(ctx, name, true));
}

void __qmljs_delete_property(Context *ctx, Value *result, String *name)
{
    Value obj = ctx->activation;
    if (! obj.isObject())
        obj = ctx->engine->globalObject;
    __qmljs_init_boolean(result, obj.objectValue->deleteProperty(ctx, name, true));
}

void __qmljs_delete_value(Context *ctx, Value *result, Value *value)
{
    Q_UNUSED(value);
    __qmljs_throw_type_error(ctx, result); // ### throw syntax error
}

void __qmljs_add_helper(Context *ctx, Value *result, const Value *left, const Value *right)
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
        __qmljs_init_string(result, string);
    } else {
        double x = __qmljs_to_number(ctx, &pleft);
        double y = __qmljs_to_number(ctx, &pright);
        __qmljs_init_number(result, x + y);
    }
}

void __qmljs_instanceof(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (FunctionObject *function = right->asFunctionObject()) {
        bool r = function->hasInstance(ctx, *left);
        __qmljs_init_boolean(result, r);
        return;
    }

    __qmljs_throw_type_error(ctx, result);
}

void __qmljs_in(Context *ctx, Value *result, const Value *left, const Value *right)
{
    if (right->type == OBJECT_TYPE) {
        Value s;
        __qmljs_to_string(ctx, &s, left);
        bool r = right->objectValue->hasProperty(ctx, s.stringValue);
        __qmljs_init_boolean(result, r);
    } else {
        __qmljs_throw_type_error(ctx, result);
    }
}

void __qmljs_inplace_bit_and_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_or_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_xor_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_add_name(Context *ctx, String *name, Value *value)
{
    if (Value *prop = ctx->lookupPropertyDescriptor(name))
        __qmljs_add(ctx, prop, prop, value);
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_inplace_sub_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_mul_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_div_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_mod_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shl_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shr_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_ushr_name(Context *ctx, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_and_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_or_element(Context *ctx, Value *base, Value *index, Value *value)
{
    Object *obj = base->toObject(ctx).objectValue;
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index->isNumber()) {
            const quint32 idx = index->toUInt32(ctx);
            Value v = a->value.at(idx);
            __qmljs_bit_or(ctx, &v, &v, value);
            a->value.assign(idx, v);
            return;
        }
    }
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_xor_element(Context *ctx, Value *base, Value *index, Value *value)
{
    Object *obj = base->toObject(ctx).objectValue;
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index->isNumber()) {
            const quint32 idx = index->toUInt32(ctx);
            Value v = a->value.at(idx);
            __qmljs_bit_xor(ctx, &v, &v, value);
            a->value.assign(idx, v);
            return;
        }
    }
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_add_element(Context *ctx, Value *base, Value *index, Value *value)
{
    Object *obj = base->toObject(ctx).objectValue;
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index->isNumber()) {
            const quint32 idx = index->toUInt32(ctx);
            Value v = a->value.at(idx);
            __qmljs_add(ctx, &v, &v, value);
            a->value.assign(idx, v);
            return;
        }
    }
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_sub_element(Context *ctx, Value *base, Value *index, Value *value)
{
    Object *obj = base->toObject(ctx).objectValue;
    if (ArrayObject *a = obj->asArrayObject()) {
        if (index->isNumber()) {
            const quint32 idx = index->toUInt32(ctx);
            Value v = a->value.at(idx);
            __qmljs_sub(ctx, &v, &v, value);
            a->value.assign(idx, v);
            return;
        }
    }
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_mul_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_div_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_mod_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shl_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shr_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_ushr_element(Context *ctx, Value *base, Value *index, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_and_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_or_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_bit_xor_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_add_member(Context *ctx, Value *base, String *name, Value *value)
{
    Value prop = base->objectValue->getProperty(ctx, name);
    __qmljs_add(ctx, &prop, &prop, value);
    base->objectValue->setProperty(ctx, name, prop);
}

void __qmljs_inplace_sub_member(Context *ctx, Value *base, String *name, Value *value)
{
    Value prop = base->objectValue->getProperty(ctx, name);
    __qmljs_sub(ctx, &prop, &prop, value);
    base->objectValue->setProperty(ctx, name, prop);
}

void __qmljs_inplace_mul_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_div_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_mod_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shl_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_shr_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

void __qmljs_inplace_ushr_member(Context *ctx, Value *base, String *name, Value *value)
{
    ctx->throwUnimplemented(QLatin1String(Q_FUNC_INFO));
}

String *__qmljs_string_from_utf8(Context *ctx, const char *s)
{
    return ctx->engine->newString(QString::fromUtf8(s));
}

String *__qmljs_identifier_from_utf8(Context *ctx, const char *s)
{
    return ctx->engine->identifier(QString::fromUtf8(s));
}

int __qmljs_string_length(Context *, String *string)
{
    return string->toQString().length();
}

double __qmljs_string_to_number(Context *, String *string)
{
    const QString s = string->toQString();
    if (s.startsWith(QLatin1String("0x")) || s.startsWith(QLatin1String("0X")))
        return s.toLong(0, 16);
    bool ok = false;
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
    return left == right || left->isEqualTo(right);
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

    Value conv = oo->getProperty(ctx, meth1);
    if (!conv.isUndefined() && conv.isFunctionObject()) {
        Value r;
        __qmljs_call_value(ctx, &r, object, &conv, 0, 0);
        if (r.isPrimitive()) {
            *result = r;
            return;
        }
    }

    conv = oo->getProperty(ctx, meth2);
    if (!conv.isUndefined() && conv.isFunctionObject()) {
        Value r;
        __qmljs_call_value(ctx, &r, object, &conv, 0, 0);
        if (r.isPrimitive()) {
            *result = r;
            return;
        }
    }

    __qmljs_init_undefined(result);
}

void __qmljs_throw_type_error(Context *ctx, Value *result)
{
    ctx->throwTypeError();
    if (result)
        *result = ctx->result;
}

void __qmljs_new_object(Context *ctx, Value *result)
{
    __qmljs_init_object(result, ctx->engine->newObject());
}

void __qmljs_new_boolean_object(Context *ctx, Value *result, bool boolean)
{
    Value value;
    __qmljs_init_boolean(&value, boolean);
    __qmljs_init_object(result, ctx->engine->newBooleanObject(value));
}

void __qmljs_new_number_object(Context *ctx, Value *result, double number)
{
    Value value;
    __qmljs_init_number(&value, number);
    __qmljs_init_object(result, ctx->engine->newNumberObject(value));
}

void __qmljs_new_string_object(Context *ctx, Value *result, String *string)
{
    Value value;
    __qmljs_init_string(&value, string);
    __qmljs_init_object(result, ctx->engine->newStringObject(value));
}

void __qmljs_set_property(Context *ctx, Value *object, String *name, Value *value)
{
    object->objectValue->setProperty(ctx, name, *value, /*flags*/ 0);
}

void __qmljs_set_property_boolean(Context *ctx, Value *object, String *name, bool number)
{
    Value value;
    __qmljs_init_boolean(&value, number);
    object->objectValue->setProperty(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_number(Context *ctx, Value *object, String *name, double number)
{
    Q_UNUSED(ctx);
    Value value;
    __qmljs_init_number(&value, number);
    object->objectValue->setProperty(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_string(Context *ctx, Value *object, String *name, String *s)
{
    Q_UNUSED(ctx);
    Value value;
    __qmljs_init_string(&value, s);
    object->objectValue->setProperty(ctx, name, value, /*flag*/ 0);
}

void __qmljs_set_property_closure(Context *ctx, Value *object, String *name, IR::Function *function)
{
    Value value;
    __qmljs_init_closure(ctx, &value, function);
    object->objectValue->setProperty(ctx, name, value, /*flag*/ 0);
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

        *result = object->property(ctx, name);
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

        object->objectValue->setProperty(ctx, name, *value, /*flags*/ 0);
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
    if (Value *base = ctx->lookupPropertyDescriptor(name)) {
        __qmljs_set_element(ctx, base, index, value);
    } else {
        ctx->throwReferenceError(Value::fromString(name));
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
    if (Value *prop = ctx->lookupPropertyDescriptor(name)) {
        *prop = *value;
    } else
        ctx->engine->globalObject.objectValue->setProperty(ctx, name, *value);
}

void __qmljs_copy_activation_property(Context *ctx, String *name, String *other)
{
    if (Value *source = ctx->lookupPropertyDescriptor(other))
        __qmljs_set_activation_property(ctx, name, source);
    else
        ctx->throwReferenceError(Value::fromString(name));
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
        *result = object->property(ctx, name);
    } else if (object->type == STRING_TYPE && name->isEqualTo(ctx->engine->id_length)) {
        __qmljs_init_number(result,  object->stringValue->toQString().length());
    } else {
        Value o;
        __qmljs_to_object(ctx, &o, object);

        if (o.isObject())
            __qmljs_get_property(ctx, result, &o, name);
        else
            ctx->throwTypeError(); // ### not necessary.
    }
}

void __qmljs_get_activation_property(Context *ctx, Value *result, String *name)
{
    if (Value *prop = ctx->lookupPropertyDescriptor(name))
        *result = *prop;
    else
        ctx->throwReferenceError(Value::fromString(name));
}

void __qmljs_get_activation(Context *ctx, Value *result)
{
    *result = ctx->activation;
}

void __qmljs_get_thisObject(Context *ctx, Value *result)
{
    if (ctx->thisObject.isObject())
        *result = ctx->thisObject;
    else
        *result = ctx->engine->globalObject;
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
    Value *func = context->lookupPropertyDescriptor(name);
    if (! func)
        context->throwReferenceError(Value::fromString(name));
    else
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
    Value func = baseObject.property(context, name);
    if (FunctionObject *f = func.asFunctionObject()) {
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
        context->throwTypeError();
    }
}

void __qmljs_call_value(Context *context, Value *result, const Value *thisObject, const Value *func, Value *args, int argc)
{
    if (FunctionObject *f = func->asFunctionObject()) {
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
        context->throwTypeError();
    }
}

void __qmljs_construct_activation_property(Context *context, Value *result, String *name, Value *args, int argc)
{
    Value *func = context->lookupPropertyDescriptor(name);
    if (! func)
        context->throwReferenceError(Value::fromString(name));
    else
        __qmljs_construct_value(context, result, func, args, argc);
}

void __qmljs_construct_value(Context *context, Value *result, const Value *func, Value *args, int argc)
{
    if (FunctionObject *f = func->asFunctionObject()) {
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
        context->throwTypeError();
    }
}

void __qmljs_construct_property(Context *context, Value *result, const Value *base, String *name, Value *args, int argc)
{
    Value thisObject = *base;
    if (thisObject.type != OBJECT_TYPE)
        __qmljs_to_object(context, &thisObject, base);

    assert(thisObject.type == OBJECT_TYPE);
    Value func = thisObject.property(context, name);
    if (FunctionObject *f = func.asFunctionObject()) {
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
        context->throwTypeError();
    }
}

void __qmljs_throw(Context *context, Value *value)
{
    context->hasUncaughtException = true;
    context->result = *value;
}

void __qmljs_rethrow(Context *context, Value *result)
{
    *result = context->result;
}

void __qmljs_builtin_typeof(Context *context, Value *result, Value *args, int argc)
{
    Q_UNUSED(argc);
    __qmljs_typeof(context, result, &args[0]);
}

void __qmljs_builtin_throw(Context *context, Value *result, Value *args, int argc)
{
    Q_UNUSED(argc);
    Q_UNUSED(result);
    __qmljs_throw(context, &args[0]);
}

void __qmljs_builtin_rethrow(Context *context, Value *result, Value *, int)
{
    __qmljs_rethrow(context, result);
}

} // extern "C"


} // namespace VM
} // namespace QQmlJS
