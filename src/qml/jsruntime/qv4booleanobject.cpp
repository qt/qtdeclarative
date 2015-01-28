/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4booleanobject_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(BooleanCtor);
DEFINE_OBJECT_VTABLE(BooleanObject);

Heap::BooleanCtor::BooleanCtor(QV4::ExecutionContext *scope)
    : Heap::FunctionObject(scope, QStringLiteral("Boolean"))
{
}

ReturnedValue BooleanCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(static_cast<BooleanCtor *>(m)->engine());
    bool n = callData->argc ? callData->args[0].toBoolean() : false;
    return Encode(scope.engine->newBooleanObject(n));
}

ReturnedValue BooleanCtor::call(Managed *, CallData *callData)
{
    bool value = callData->argc ? callData->args[0].toBoolean() : 0;
    return Encode(value);
}

void BooleanPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_length, Primitive::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype, (o = this));
    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString, method_toString);
    defineDefaultProperty(engine->id_valueOf, method_valueOf);
}

ReturnedValue BooleanPrototype::method_toString(CallContext *ctx)
{
    bool result;
    if (ctx->thisObject().isBoolean()) {
        result = ctx->thisObject().booleanValue();
    } else {
        BooleanObject *thisObject = ctx->thisObject().as<BooleanObject>();
        if (!thisObject)
            return ctx->engine()->throwTypeError();
        result = thisObject->value();
    }

    return Encode(ctx->d()->engine->newString(QLatin1String(result ? "true" : "false")));
}

ReturnedValue BooleanPrototype::method_valueOf(CallContext *ctx)
{
    if (ctx->thisObject().isBoolean())
        return ctx->thisObject().asReturnedValue();

    BooleanObject *thisObject = ctx->thisObject().as<BooleanObject>();
    if (!thisObject)
        return ctx->engine()->throwTypeError();

    return Encode(thisObject->value());
}
