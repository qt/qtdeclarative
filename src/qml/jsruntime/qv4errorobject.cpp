/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>

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

ErrorObject::ErrorObject(InternalClass *ic)
    : Object(ic)
    , stack(0)
{
    Scope scope(engine());
    ScopedValue protectThis(scope, this);

    ScopedString s(scope, ic->engine->newString(QStringLiteral("Error")));
    defineDefaultProperty(QStringLiteral("name"), s);
}

ErrorObject::ErrorObject(InternalClass *ic, const ValueRef message, ErrorType t)
    : Object(ic)
    , stack(0)
{
    subtype = t;

    Scope scope(engine());
    ScopedValue protectThis(scope, this);

    defineAccessorProperty(QStringLiteral("stack"), ErrorObject::method_get_stack, 0);

    if (!message->isUndefined())
        defineDefaultProperty(QStringLiteral("message"), message);
    ScopedString s(scope);
    defineDefaultProperty(QStringLiteral("name"), (s = ic->engine->newString(className())));

    stackTrace = ic->engine->stackTrace();
    if (!stackTrace.isEmpty()) {
        defineDefaultProperty(QStringLiteral("fileName"), (s = ic->engine->newString(stackTrace.at(0).source)));
        defineDefaultProperty(QStringLiteral("lineNumber"), Primitive::fromInt32(stackTrace.at(0).line));
    }
}

ErrorObject::ErrorObject(InternalClass *ic, const QString &message, ErrorObject::ErrorType t)
    : Object(ic)
    , stack(0)
{
    subtype = t;

    Scope scope(engine());
    ScopedValue protectThis(scope, this);
    ScopedString s(scope);

    defineAccessorProperty(QStringLiteral("stack"), ErrorObject::method_get_stack, 0);

    ScopedValue v(scope, ic->engine->newString(message));
    defineDefaultProperty(QStringLiteral("message"), v);
    defineDefaultProperty(QStringLiteral("name"), (s = ic->engine->newString(className())));

    stackTrace = ic->engine->stackTrace();
    if (!stackTrace.isEmpty()) {
        defineDefaultProperty(QStringLiteral("fileName"), (s = ic->engine->newString(stackTrace.at(0).source)));
        defineDefaultProperty(QStringLiteral("lineNumber"), Primitive::fromInt32(stackTrace.at(0).line));
    }
}

ErrorObject::ErrorObject(InternalClass *ic, const QString &message, const QString &fileName, int line, int column, ErrorObject::ErrorType t)
    : Object(ic)
    , stack(0)
{
    subtype = t;

    Scope scope(engine());
    ScopedValue protectThis(scope, this);
    ScopedString s(scope);

    defineAccessorProperty(QStringLiteral("stack"), ErrorObject::method_get_stack, 0);
    defineDefaultProperty(QStringLiteral("name"), (s = ic->engine->newString(className())));

    stackTrace = ic->engine->stackTrace();
    StackFrame frame;
    frame.source = fileName;
    frame.line = line;
    frame.column = column;
    stackTrace.prepend(frame);

    if (!stackTrace.isEmpty()) {
        defineDefaultProperty(QStringLiteral("fileName"), (s = ic->engine->newString(stackTrace.at(0).source)));
        defineDefaultProperty(QStringLiteral("lineNumber"), Primitive::fromInt32(stackTrace.at(0).line));
    }

    ScopedValue v(scope, ic->engine->newString(message));
    defineDefaultProperty(QStringLiteral("message"), v);
}

ReturnedValue ErrorObject::method_get_stack(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<ErrorObject> This(scope, ctx->callData->thisObject);
    if (!This)
        return ctx->throwTypeError();
    if (!This->stack) {
        QString trace;
        for (int i = 0; i < This->stackTrace.count(); ++i) {
            if (i > 0)
                trace += QLatin1Char('\n');
            const StackFrame &frame = This->stackTrace[i];
            trace += frame.function;
            trace += QLatin1Char('@');
            trace += frame.source;
            if (frame.line >= 0) {
                trace += QLatin1Char(':');
                trace += QString::number(frame.line);
            }
        }
        This->stack = ctx->engine->newString(trace)->getPointer();
    }
    return This->stack->asReturnedValue();
}

void ErrorObject::markObjects(Managed *that, ExecutionEngine *e)
{
    ErrorObject *This = that->asErrorObject();
    if (This->stack)
        This->stack->mark(e);
    Object::markObjects(that, e);
}

DEFINE_OBJECT_VTABLE(ErrorObject);

DEFINE_OBJECT_VTABLE(SyntaxErrorObject);

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const ValueRef msg)
    : ErrorObject(engine->syntaxErrorClass, msg, SyntaxError)
{
}

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber)
    : ErrorObject(engine->syntaxErrorClass, msg, fileName, lineNumber, columnNumber, SyntaxError)
{
}

EvalErrorObject::EvalErrorObject(ExecutionEngine *engine, const ValueRef message)
    : ErrorObject(engine->evalErrorClass, message, EvalError)
{
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const ValueRef message)
    : ErrorObject(engine->rangeErrorClass, message, RangeError)
{
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->rangeErrorClass, message, RangeError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const ValueRef message)
    : ErrorObject(engine->referenceErrorClass, message, ReferenceError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->referenceErrorClass, message, ReferenceError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber)
    : ErrorObject(engine->referenceErrorClass, msg, fileName, lineNumber, columnNumber, ReferenceError)
{
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const ValueRef message)
    : ErrorObject(engine->typeErrorClass, message, TypeError)
{
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->typeErrorClass, message, TypeError)
{
}

URIErrorObject::URIErrorObject(ExecutionEngine *engine, const ValueRef message)
    : ErrorObject(engine->uriErrorClass, message, URIError)
{
}

DEFINE_OBJECT_VTABLE(ErrorCtor);
DEFINE_OBJECT_VTABLE(EvalErrorCtor);
DEFINE_OBJECT_VTABLE(RangeErrorCtor);
DEFINE_OBJECT_VTABLE(ReferenceErrorCtor);
DEFINE_OBJECT_VTABLE(SyntaxErrorCtor);
DEFINE_OBJECT_VTABLE(TypeErrorCtor);
DEFINE_OBJECT_VTABLE(URIErrorCtor);

ErrorCtor::ErrorCtor(ExecutionContext *scope)
    : FunctionObject(scope, QStringLiteral("Error"))
{
    setVTable(staticVTable());
}

ErrorCtor::ErrorCtor(ExecutionContext *scope, const QString &name)
    : FunctionObject(scope, name)
{
    setVTable(staticVTable());
}

ReturnedValue ErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return Encode(m->engine()->newErrorObject(v));
}

ReturnedValue ErrorCtor::call(Managed *that, CallData *callData)
{
    return static_cast<Object *>(that)->construct(callData);
}

EvalErrorCtor::EvalErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("EvalError"))
{
    setVTable(staticVTable());
}

ReturnedValue EvalErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) EvalErrorObject(m->engine(), v))->asReturnedValue();
}

RangeErrorCtor::RangeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("RangeError"))
{
    setVTable(staticVTable());
}

ReturnedValue RangeErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) RangeErrorObject(scope.engine, v))->asReturnedValue();
}

ReferenceErrorCtor::ReferenceErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("ReferenceError"))
{
    setVTable(staticVTable());
}

ReturnedValue ReferenceErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) ReferenceErrorObject(scope.engine, v))->asReturnedValue();
}

SyntaxErrorCtor::SyntaxErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("SyntaxError"))
{
    setVTable(staticVTable());
}

ReturnedValue SyntaxErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) SyntaxErrorObject(scope.engine, v))->asReturnedValue();
}

TypeErrorCtor::TypeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("TypeError"))
{
    setVTable(staticVTable());
}

ReturnedValue TypeErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) TypeErrorObject(scope.engine, v))->asReturnedValue();
}

URIErrorCtor::URIErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, QStringLiteral("URIError"))
{
    setVTable(staticVTable());
}

ReturnedValue URIErrorCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(m->engine());
    ScopedValue v(scope, callData->argument(0));
    return (new (m->engine()->memoryManager) URIErrorObject(scope.engine, v))->asReturnedValue();
}

void ErrorPrototype::init(ExecutionEngine *engine, ObjectRef ctor, Object *obj)
{
    Scope scope(engine);
    ScopedString s(scope);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_prototype, (o = obj));
    ctor->defineReadonlyProperty(engine->id_length, Primitive::fromInt32(1));
    obj->defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    obj->defineDefaultProperty(engine->id_toString, method_toString, 0);
    obj->defineDefaultProperty(QStringLiteral("message"), (s = engine->newString(QString())));
}

ReturnedValue ErrorPrototype::method_toString(CallContext *ctx)
{
    Scope scope(ctx);

    Object *o = ctx->callData->thisObject.asObject();
    if (!o)
        return ctx->throwTypeError();

    ScopedValue name(scope, o->get(ctx->engine->id_name));
    QString qname;
    if (name->isUndefined())
        qname = QString::fromLatin1("Error");
    else
        qname = name->toQString();

    ScopedString s(scope, ctx->engine->newString(QString::fromLatin1("message")));
    ScopedValue message(scope, o->get(s));
    QString qmessage;
    if (!message->isUndefined())
        qmessage = message->toQString();

    QString str;
    if (qname.isEmpty()) {
        str = qmessage;
    } else if (qmessage.isEmpty()) {
        str = qname;
    } else {
        str = qname + QLatin1String(": ") + qmessage;
    }

    return ctx->engine->newString(str)->asReturnedValue();
}
