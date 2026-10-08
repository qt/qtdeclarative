// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4booleanobject_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(BooleanCtor);
DEFINE_OBJECT_VTABLE(BooleanObject);

void Heap::BooleanCtor::init(QV4::ExecutionEngine *engine)
{
    Heap::FunctionObject::init(engine, QStringLiteral("Boolean"));
}

ReturnedValue BooleanCtor::virtualCallAsConstructor(const FunctionObject *that, const Value *argv, int argc, const Value *newTarget)
{
    auto v4 = that->engine();
    bool n = argc ? argv[0].toBoolean() : false;

    ReturnedValue o = Encode(v4->newBooleanObject(n));
    if (!newTarget)
        return o;
    Scope scope(v4);
    ScopedObject obj(scope, o);
    obj->setProtoFromNewTarget(newTarget);
    return obj->asReturnedValue();
}

ReturnedValue BooleanCtor::virtualCall(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    bool value = argc ? argv[0].toBoolean() : 0;
    return Encode(value);
}

void BooleanPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString(), method_toString);
    defineDefaultProperty(engine->id_valueOf(), method_valueOf);
}

static bool value(const Value *thisObject, bool *exception)
{
    *exception = false;
    if (thisObject->isBoolean()) {
        return thisObject->booleanValue();
    } else {
        const BooleanObject *that = thisObject->as<BooleanObject>();
        if (that)
            return that->value();
    }
    *exception = true;
    return false;
}

ReturnedValue BooleanPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    bool exception;
    bool result = ::value(thisObject, &exception);
    ExecutionEngine *v4 = b->engine();
    if (exception)
        return v4->throwTypeError();

    return (result ? v4->id_true() : v4->id_false())->asReturnedValue();
}

ReturnedValue BooleanPrototype::method_valueOf(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    bool exception;
    bool result = ::value(thisObject, &exception);
    if (exception) {
        ExecutionEngine *v4 = b->engine();
        return v4->throwTypeError();
    }

    return Encode(result);
}
