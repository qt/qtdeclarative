// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qv4iterator_p.h>
#include <qv4symbol_p.h>
#include <qv4engine_p.h>

using namespace QV4;

void IteratorPrototype::init(ExecutionEngine *engine)
{
    defineDefaultProperty(engine->symbol_iterator(), method_iterator, 0);
}

ReturnedValue IteratorPrototype::method_iterator(const FunctionObject *, const Value *thisObject, const Value *, int)
{
    return thisObject->asReturnedValue();
}


ReturnedValue IteratorPrototype::createIterResultObject(ExecutionEngine *engine, const Value &value, bool done)
{
    Scope scope(engine);
    ScopedObject obj(scope, engine->newObject());
    obj->set(ScopedString(scope, engine->newString(QStringLiteral("value"))), value, Object::DoNotThrow);
    obj->set(ScopedString(scope, engine->newString(QStringLiteral("done"))), Value::fromBoolean(done), Object::DoNotThrow);
    return obj->asReturnedValue();
}

