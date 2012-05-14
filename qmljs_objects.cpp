
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"
#include <QtCore/QDebug>
#include <cassert>

using namespace QQmlJS::VM;

Object::~Object()
{
    delete members;
}

void Object::setProperty(Context *ctx, const QString &name, const Value &value)
{
    put(ctx->engine->identifier(name), value);
}

void Object::setProperty(Context *ctx, const QString &name, void (*code)(Context *))
{
    setProperty(ctx, name, Value::object(ctx, new NativeFunction(ctx, code)));
}

bool Object::get(String *name, Value *result)
{
    if (Value *prop = getProperty(name)) {
        *result = *prop;
        return true;
    }

    __qmljs_init_undefined(0, result);
    return false;
}

Value *Object::getOwnProperty(String *name, PropertyAttributes *attributes)
{
    if (members) {
        if (Property *prop = members->find(name)) {
            if (attributes)
                *attributes = prop->attributes;
            return &prop->value;
        }
    }
    return 0;
}

Value *Object::getProperty(String *name, PropertyAttributes *attributes)
{
    if (Value *prop = getOwnProperty(name, attributes))
        return prop;
    else if (prototype)
        return prototype->getProperty(name, attributes);
    return 0;
}

void Object::put(String *name, const Value &value, bool flag)
{
    Q_UNUSED(flag);

    if (! members)
        members = new Table();

    members->insert(name, value);
}

bool Object::canPut(String *name)
{
    PropertyAttributes attrs = PropertyAttributes();
    if (getOwnProperty(name, &attrs)) {
        return attrs & WritableAttribute;
    } else if (! prototype) {
        return extensible;
    } else if (prototype->getProperty(name, &attrs)) {
        return attrs & WritableAttribute;
    } else {
        return extensible;
    }
    return true;
}

bool Object::hasProperty(String *name) const
{
    if (members)
        return members->find(name) != 0;

    return false;
}

bool Object::deleteProperty(String *name, bool flag)
{
    Q_UNUSED(flag);

    if (members)
        return members->remove(name);

    return false;
}

void Object::defaultValue(Context *ctx, Value *result, int typeHint)
{
    if (typeHint == STRING_HINT) {
        if (asFunctionObject() != 0)
            __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("function")));
        else
            __qmljs_init_string(ctx, result, ctx->engine->identifier(QLatin1String("object")));
    } else {
        __qmljs_init_undefined(ctx, result);
    }
}


bool FunctionObject::hasInstance(const Value &value) const
{
    Q_UNUSED(value);
    return false;
}

void FunctionObject::call(Context *ctx)
{
    Q_UNUSED(ctx);
}

void FunctionObject::construct(Context *ctx)
{
    __qmljs_init_object(ctx, &ctx->thisObject, new Object());
    call(ctx);
}

ScriptFunction::ScriptFunction(Context *scope, IR::Function *function)
    : FunctionObject(scope)
    , function(function)
{
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (size_t i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = scope->engine->identifier(*function->formals.at(i));
        }
    }
}

ScriptFunction::~ScriptFunction()
{
    delete[] formalParameterList;
}

void ScriptFunction::call(VM::Context *ctx)
{
    function->code(ctx);
}

void ScriptFunction::construct(VM::Context *ctx)
{
    __qmljs_init_object(ctx, &ctx->thisObject, new Object());
    function->code(ctx);
}

Value *ArgumentsObject::getProperty(String *name, PropertyAttributes *attributes)
{
    if (context) {
        for (size_t i = 0; i < context->formalCount; ++i) {
            String *formal = context->formals[i];
            if (__qmljs_string_equal(context, formal, name)) {
                if (attributes)
                    *attributes = PropertyAttributes(*attributes | WritableAttribute);
                return &context->arguments[i];
            }
        }
    }
    if (Value *prop = Object::getProperty(name, attributes))
        return prop;
    return 0;
}

ExecutionEngine::ExecutionEngine()
{
    rootContext = new VM::Context;
    rootContext->init(this);

    //
    // set up the global object
    //
    VM::Object *glo = new VM::ArgumentsObject(rootContext);
    __qmljs_init_object(rootContext, &globalObject, glo);
    __qmljs_init_object(rootContext, &rootContext->activation, glo);

    objectCtor = ObjectCtor::create(this);
    stringCtor = StringCtor::create(this);
    numberCtor = NumberCtor::create(this);

    String *prototype = String::get(rootContext, QLatin1String("prototype"));

    objectCtor.objectValue->get(prototype, &objectPrototype);
    stringCtor.objectValue->get(prototype, &stringPrototype);
    numberCtor.objectValue->get(prototype, &numberPrototype);

    glo->put(VM::String::get(rootContext, QLatin1String("Object")), objectCtor);
    glo->put(VM::String::get(rootContext, QLatin1String("String")), stringCtor);
    glo->put(VM::String::get(rootContext, QLatin1String("Number")), numberCtor);
}

String *ExecutionEngine::identifier(const QString &s)
{
    String *&id = identifiers[s];
    if (! id)
        id = new String(s);
    return id;
}
