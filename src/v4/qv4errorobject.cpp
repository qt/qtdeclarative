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


#include "qv4errorobject.h"
#include "qv4mm.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <cmath>
#include <qmath.h>
#include <qnumeric.h>
#include <cassert>

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include <qv4isel_masm_p.h>

#ifndef Q_OS_WIN
#  include <time.h>
#  ifndef Q_OS_VXWORKS
#    include <sys/time.h>
#  else
#    include "qplatformdefs.h"
#  endif
#else
#  include <windows.h>
#endif

using namespace QQmlJS::VM;

ErrorObject::ErrorObject(ExecutionEngine* engine, const Value &message)
    : Object(engine)
{
    type = Type_ErrorObject;
    subtype = Error;

    if (!message.isUndefined())
        defineDefaultProperty(engine->newString(QStringLiteral("message")), message);
}

void ErrorObject::setNameProperty(ExecutionContext *ctx)
{
    defineDefaultProperty(ctx, QLatin1String("name"), Value::fromString(ctx, className()));
}

DEFINE_MANAGED_VTABLE(SyntaxErrorObject);

SyntaxErrorObject::SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message)
    : ErrorObject(ctx->engine, message ? Value::fromString(message->buildFullMessage(ctx)) : ctx->argument(0))
    , msg(message)
{
    vtbl = &static_vtbl;
    subtype = SyntaxError;
    prototype = ctx->engine->syntaxErrorPrototype;
    setNameProperty(ctx);
}



EvalErrorObject::EvalErrorObject(ExecutionContext *ctx, const Value &message)
    : ErrorObject(ctx->engine, message)
{
    subtype = EvalError;
    setNameProperty(ctx);
    prototype = ctx->engine->evalErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionContext *ctx, const Value &message)
    : ErrorObject(ctx->engine, message)
{
    subtype = RangeError;
    setNameProperty(ctx);
    prototype = ctx->engine->rangeErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionContext *ctx, const QString &message)
    : ErrorObject(ctx->engine, Value::fromString(ctx,message))
{
    subtype = RangeError;
    setNameProperty(ctx);
    prototype = ctx->engine->rangeErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionContext *ctx, const Value &message)
    : ErrorObject(ctx->engine, message)
{
    subtype = ReferenceError;
    setNameProperty(ctx);
    prototype = ctx->engine->referenceErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionContext *ctx, const QString &message)
    : ErrorObject(ctx->engine, Value::fromString(ctx,message))
{
    subtype = ReferenceError;
    setNameProperty(ctx);
    prototype = ctx->engine->referenceErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionContext *ctx, const Value &message)
    : ErrorObject(ctx->engine, message)
{
    subtype = TypeError;
    setNameProperty(ctx);
    prototype = ctx->engine->typeErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionContext *ctx, const QString &message)
    : ErrorObject(ctx->engine, Value::fromString(ctx,message))
{
    subtype = TypeError;
    setNameProperty(ctx);
    prototype = ctx->engine->typeErrorPrototype;
}

URIErrorObject::URIErrorObject(ExecutionContext *ctx, const Value &message)
    : ErrorObject(ctx->engine, message)
{
    subtype = URIError;
    setNameProperty(ctx);
    prototype = ctx->engine->uRIErrorPrototype;
}

DEFINE_MANAGED_VTABLE(ErrorCtor);
DEFINE_MANAGED_VTABLE(EvalErrorCtor);
DEFINE_MANAGED_VTABLE(RangeErrorCtor);
DEFINE_MANAGED_VTABLE(ReferenceErrorCtor);
DEFINE_MANAGED_VTABLE(SyntaxErrorCtor);
DEFINE_MANAGED_VTABLE(TypeErrorCtor);
DEFINE_MANAGED_VTABLE(URIErrorCtor);

ErrorCtor::ErrorCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
    vtbl = &static_vtbl;
}

Value ErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(ctx->engine->newErrorObject(argc ? args[0] : Value::undefinedValue()));
}

Value ErrorCtor::call(Managed *that, ExecutionContext *ctx, const Value &, Value *args, int argc)
{
    return that->construct(ctx, args, argc);
}

Value EvalErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) EvalErrorObject(ctx, argc ? args[0] : Value::undefinedValue()));
}

Value RangeErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) RangeErrorObject(ctx, argc ? args[0] : Value::undefinedValue()));
}

Value ReferenceErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) ReferenceErrorObject(ctx, argc ? args[0] : Value::undefinedValue()));
}

Value SyntaxErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *, int)
{
    return Value::fromObject(new (ctx->engine->memoryManager) SyntaxErrorObject(ctx, 0));
}

Value TypeErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) TypeErrorObject(ctx, argc ? args[0] : Value::undefinedValue()));
}

Value URIErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) URIErrorObject(ctx, argc ? args[0] : Value::undefinedValue()));
}

void ErrorPrototype::init(ExecutionContext *ctx, const Value &ctor, Object *obj)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(obj));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    obj->defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    obj->defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    obj->defineDefaultProperty(ctx, QStringLiteral("message"), Value::fromString(ctx, QString()));
    obj->defineDefaultProperty(ctx, QStringLiteral("name"), Value::fromString(ctx, QStringLiteral("Error")));
}

Value ErrorPrototype::method_toString(SimpleCallContext *ctx)
{
    Object *o = ctx->thisObject.asObject();
    if (!o)
        ctx->throwTypeError();

    Value name = o->get(ctx, ctx->engine->newString(QString::fromLatin1("name")));
    QString qname;
    if (name.isUndefined())
        qname = QString::fromLatin1("Error");
    else
        qname = __qmljs_to_string(name, ctx).stringValue()->toQString();

    Value message = o->get(ctx, ctx->engine->newString(QString::fromLatin1("message")));
    QString qmessage;
    if (!message.isUndefined())
        qmessage = __qmljs_to_string(message, ctx).stringValue()->toQString();

    QString str;
    if (qname.isEmpty()) {
        str = qmessage;
    } else if (qmessage.isEmpty()) {
        str = qname;
    } else {
        str = qname + QLatin1String(": ") + qmessage;
    }

    return Value::fromString(ctx, str);
}
