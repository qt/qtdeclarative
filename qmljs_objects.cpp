
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
    if (Property *prop = getProperty(name)) {
        *result = prop->value;
        return true;
    }

    __qmljs_init_undefined(0, result);
    return false;
}

Property *Object::getOwnProperty(String *name)
{
    if (members) {
        if (Property *prop = members->find(name)) {
            return prop;
        }
    }
    return 0;
}

Property *Object::getProperty(String *name)
{
    if (Property *prop = getOwnProperty(name))
        return prop;
    else if (prototype)
        return prototype->getProperty(name);
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
    if (Property *prop = getOwnProperty(name)) {
        Q_UNUSED(prop);
        return true;
    } else if (! prototype) {
        return extensible;
    } else if (Property *inherited = prototype->getProperty(name)) {
        return inherited->isWritable();
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

void ScriptFunction::call(VM::Context *ctx)
{
    // bind the actual arguments. ### slow
    for (int i = 0; i < function->formals.size(); ++i) {
        const QString *f = function->formals.at(i);
        ctx->activation.objectValue->put(String::get(ctx, *f), ctx->arguments[i]);
    }
    function->code(ctx);
}

Property *ArgumentsObject::getOwnProperty(String *name)
{
    if (Property *prop = Object::getOwnProperty(name))
        return prop;
    else if (context && context->scope)
        return context->scope->getOwnProperty(name);
    return 0;
}
