
#include "qmljs_objects.h"
#include <cassert>

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
    if (! members)
        members = new (GC) Table();

    members->insert(name, value);
}

bool Object::canPut(String *name)
{
    if (Property *prop = getOwnProperty(name))
        return true;
    if (! prototype)
        return extensible;
    if (Property *inherited = prototype->getProperty(name)) {
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

Value FunctionObject::call(const Value &thisObject, const Value args[], unsigned argc)
{
    (void) thisObject;

    Value v;
    __qmljs_init_undefined(0, &v);
    return v;
}

Value FunctionObject::construct(const Value args[], unsigned argc)
{
    Value thisObject;
    __qmljs_init_object(0, &thisObject, new (GC) Object);
    call(thisObject, args, argc);
    return thisObject;
}
