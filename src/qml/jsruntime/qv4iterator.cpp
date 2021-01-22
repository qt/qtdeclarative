/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
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

