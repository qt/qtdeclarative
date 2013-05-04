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


#include "qv4errorobject_p.h"
#include "qv4mm_p.h"
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

using namespace QV4;

ErrorObject::ErrorObject(ExecutionEngine *engine, const Value &message, ErrorType t)
    : Object(engine)
{
    type = Type_ErrorObject;
    subtype = t;

    if (!message.isUndefined())
        defineDefaultProperty(engine->newString(QStringLiteral("message")), message);
    defineDefaultProperty(engine, QStringLiteral("name"), Value::fromString(engine, className()));
}

DEFINE_MANAGED_VTABLE(SyntaxErrorObject);

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const Value &msg)
    : ErrorObject(engine, msg, SyntaxError)
{
    vtbl = &static_vtbl;
    prototype = engine->syntaxErrorPrototype;
}

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const QString &msg)
    : ErrorObject(engine, Value::fromString(engine, msg), SyntaxError)
{
    vtbl = &static_vtbl;
    prototype = engine->syntaxErrorPrototype;
}

SyntaxErrorObject::SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message)
    : ErrorObject(ctx->engine, message ? Value::fromString(message->buildFullMessage(ctx)) : ctx->argument(0), SyntaxError)
    , msg(message)
{
    vtbl = &static_vtbl;
    prototype = ctx->engine->syntaxErrorPrototype;
}



EvalErrorObject::EvalErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine, message, EvalError)
{
    prototype = engine->evalErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine, message, RangeError)
{
    prototype = engine->rangeErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine, Value::fromString(engine, message), RangeError)
{
    prototype = engine->rangeErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine, message, ReferenceError)
{
    prototype = engine->referenceErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine, Value::fromString(engine, message), ReferenceError)
{
    prototype = engine->referenceErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine, message, TypeError)
{
    prototype = engine->typeErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine, Value::fromString(engine, message), TypeError)
{
    prototype = engine->typeErrorPrototype;
}

URIErrorObject::URIErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine, message, URIError)
{
    prototype = engine->uRIErrorPrototype;
}

DEFINE_MANAGED_VTABLE(ErrorCtor);
DEFINE_MANAGED_VTABLE(EvalErrorCtor);
DEFINE_MANAGED_VTABLE(RangeErrorCtor);
DEFINE_MANAGED_VTABLE(ReferenceErrorCtor);
DEFINE_MANAGED_VTABLE(SyntaxErrorCtor);
DEFINE_MANAGED_VTABLE(TypeErrorCtor);
DEFINE_MANAGED_VTABLE(URIErrorCtor);

ErrorCtor::ErrorCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("Error")))
{
    vtbl = &static_vtbl;
}

ErrorCtor::ErrorCtor(ExecutionContext *scope, String *name)
    : FunctionObject(scope, name)
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

EvalErrorCtor::EvalErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("EvalError"))
{
    vtbl = &static_vtbl;
}

Value EvalErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) EvalErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

RangeErrorCtor::RangeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("RangeError"))
{
    vtbl = &static_vtbl;
}

Value RangeErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) RangeErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

ReferenceErrorCtor::ReferenceErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("ReferenceError"))
{
    vtbl = &static_vtbl;
}

Value ReferenceErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) ReferenceErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

SyntaxErrorCtor::SyntaxErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("SyntaxError"))
{
    vtbl = &static_vtbl;
}

Value SyntaxErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) SyntaxErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

TypeErrorCtor::TypeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("TypeError"))
{
    vtbl = &static_vtbl;
}

Value TypeErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) TypeErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

URIErrorCtor::URIErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("URIError"))
{
    vtbl = &static_vtbl;
}

Value URIErrorCtor::construct(Managed *, ExecutionContext *ctx, Value *args, int argc)
{
    return Value::fromObject(new (ctx->engine->memoryManager) URIErrorObject(ctx->engine, argc ? args[0] : Value::undefinedValue()));
}

void ErrorPrototype::init(ExecutionEngine *engine, const Value &ctor, Object *obj)
{
    ctor.objectValue()->defineReadonlyProperty(engine->id_prototype, Value::fromObject(obj));
    ctor.objectValue()->defineReadonlyProperty(engine->id_length, Value::fromInt32(1));
    obj->defineDefaultProperty(engine, QStringLiteral("constructor"), ctor);
    obj->defineDefaultProperty(engine, QStringLiteral("toString"), method_toString, 0);
    obj->defineDefaultProperty(engine, QStringLiteral("message"), Value::fromString(engine, QString()));
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
