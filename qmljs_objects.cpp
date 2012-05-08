
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include <QtCore/QDebug>
#include <cassert>

using namespace QQmlJS::VM;

Object::~Object()
{
    delete members;
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

void Object::defaultValue(Value *result, int typeHint)
{
    Context *ctx = 0; // ###

    if (typeHint == STRING_HINT) {
        if (asFunctionObject() != 0)
            __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("function")));
        else
            __qmljs_init_string(ctx, result, String::get(ctx, QLatin1String("object")));
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
    Q_UNUSED(ctx);
    Q_UNIMPLEMENTED();
}

ScriptFunction::ScriptFunction(IR::Function *function)
    : function(function)
{
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (size_t i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = String::get(0, *function->formals.at(i)); // ### unique
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
    else if (context && context->scope)
        return context->scope->getProperty(name, attributes);
    return 0;
}
