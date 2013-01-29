/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4booleanobject.h"

using namespace QQmlJS::VM;

BooleanCtor::BooleanCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

Value BooleanCtor::construct(ExecutionContext *ctx)
{
    const double n = ctx->argument(0).toBoolean(ctx);
    return Value::fromObject(ctx->engine->newBooleanObject(Value::fromBoolean(n)));
}

Value BooleanCtor::call(ExecutionContext *ctx)
{
    bool value = ctx->argumentCount ? ctx->argument(0).toBoolean(ctx) : 0;
    return Value::fromBoolean(value);
}

void BooleanPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString);
    defineDefaultProperty(ctx, QStringLiteral("valueOf"), method_valueOf);
}

Value BooleanPrototype::method_toString(ExecutionContext *ctx)
{
    bool result;
    if (ctx->thisObject.isBoolean()) {
        result = ctx->thisObject.booleanValue();
    } else {
        BooleanObject *thisObject = ctx->thisObject.asBooleanObject();
        if (!thisObject)
            ctx->throwTypeError();
        result = thisObject->value.booleanValue();
    }

    return Value::fromString(ctx, QLatin1String(result ? "true" : "false"));
}

Value BooleanPrototype::method_valueOf(ExecutionContext *ctx)
{
    BooleanObject *thisObject = ctx->thisObject.asBooleanObject();
    if (!thisObject)
        ctx->throwTypeError();

    return thisObject->value;
}
