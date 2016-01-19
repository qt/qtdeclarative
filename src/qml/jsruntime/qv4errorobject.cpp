/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qv4errorobject_p.h"
#include <QtCore/qnumeric.h>
#include <QtCore/qmath.h>
#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include "qv4string_p.h"
#include <private/qv4mm_p.h>
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

Heap::ErrorObject::ErrorObject()
{
    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    if (internalClass == scope.engine->errorProtoClass)
        return;

    *propertyData(QV4::ErrorObject::Index_Stack) = scope.engine->getStackFunction();
    *propertyData(QV4::ErrorObject::Index_Stack + QV4::Object::SetterOffset) = Encode::undefined();
    *propertyData(QV4::ErrorObject::Index_FileName) = Encode::undefined();
    *propertyData(QV4::ErrorObject::Index_LineNumber) = Encode::undefined();
}

Heap::ErrorObject::ErrorObject(const Value &message, ErrorType t)
{
    errorType = t;

    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    *propertyData(QV4::ErrorObject::Index_Stack) = scope.engine->getStackFunction();
    *propertyData(QV4::ErrorObject::Index_Stack + QV4::Object::SetterOffset) = Encode::undefined();

    e->d()->stackTrace = scope.engine->stackTrace();
    if (!e->d()->stackTrace.isEmpty()) {
        *propertyData(QV4::ErrorObject::Index_FileName) = scope.engine->newString(e->d()->stackTrace.at(0).source);
        *propertyData(QV4::ErrorObject::Index_LineNumber) = Primitive::fromInt32(e->d()->stackTrace.at(0).line);
    }

    if (!message.isUndefined())
        *propertyData(QV4::ErrorObject::Index_Message) = message;
}

Heap::ErrorObject::ErrorObject(const Value &message, const QString &fileName, int line, int column, ErrorObject::ErrorType t)
{
    errorType = t;

    Scope scope(internalClass->engine);
    Scoped<QV4::ErrorObject> e(scope, this);

    *propertyData(QV4::ErrorObject::Index_Stack) = scope.engine->getStackFunction();
    *propertyData(QV4::ErrorObject::Index_Stack + QV4::Object::SetterOffset) = Encode::undefined();

    e->d()->stackTrace = scope.engine->stackTrace();
    StackFrame frame;
    frame.source = fileName;
    frame.line = line;
    frame.column = column;
    e->d()->stackTrace.prepend(frame);

    if (!e->d()->stackTrace.isEmpty()) {
        *propertyData(QV4::ErrorObject::Index_FileName) = scope.engine->newString(e->d()->stackTrace.at(0).source);
        *propertyData(QV4::ErrorObject::Index_LineNumber) = Primitive::fromInt32(e->d()->stackTrace.at(0).line);
    }

    if (!message.isUndefined())
        *propertyData(QV4::ErrorObject::Index_Message) = message;
}

const char *ErrorObject::className(Heap::ErrorObject::ErrorType t)
{
    switch (t) {
    case Heap::ErrorObject::Error:
        return "Error";
    case Heap::ErrorObject::EvalError:
        return "EvalError";
    case Heap::ErrorObject::RangeError:
        return "RangeError";
    case Heap::ErrorObject::ReferenceError:
        return "ReferenceError";
    case Heap::ErrorObject::SyntaxError:
        return "SyntaxError";
    case Heap::ErrorObject::TypeError:
        return "TypeError";
    case Heap::ErrorObject::URIError:
        return "URIError";
    }
    Q_UNREACHABLE();
}

ReturnedValue ErrorObject::method_get_stack(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<ErrorObject> This(scope, ctx->thisObject());
    if (!This)
        return ctx->engine()->throwTypeError();
    if (!This->d()->stack) {
        QString trace;
        for (int i = 0; i < This->d()->stackTrace.count(); ++i) {
            if (i > 0)
                trace += QLatin1Char('\n');
            const StackFrame &frame = This->d()->stackTrace[i];
            trace += frame.function;
            trace += QLatin1Char('@');
            trace += frame.source;
            if (frame.line >= 0) {
                trace += QLatin1Char(':');
                trace += QString::number(frame.line);
            }
        }
        This->d()->stack = ctx->d()->engine->newString(trace);
    }
    return This->d()->stack->asReturnedValue();
}

void ErrorObject::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    ErrorObject::Data *This = static_cast<ErrorObject::Data *>(that);
    if (This->stack)
        This->stack->mark(e);
    Object::markObjects(that, e);
}

DEFINE_OBJECT_VTABLE(ErrorObject);

DEFINE_OBJECT_VTABLE(SyntaxErrorObject);

Heap::SyntaxErrorObject::SyntaxErrorObject(const Value &msg)
    : Heap::ErrorObject(msg, SyntaxError)
{
}

Heap::SyntaxErrorObject::SyntaxErrorObject(const Value &msg, const QString &fileName, int lineNumber, int columnNumber)
    : Heap::ErrorObject(msg, fileName, lineNumber, columnNumber, SyntaxError)
{
}

Heap::EvalErrorObject::EvalErrorObject(const Value &message)
    : Heap::ErrorObject(message, EvalError)
{
}

Heap::RangeErrorObject::RangeErrorObject(const Value &message)
    : Heap::ErrorObject(message, RangeError)
{
}

Heap::ReferenceErrorObject::ReferenceErrorObject(const Value &message)
    : Heap::ErrorObject(message, ReferenceError)
{
}

Heap::ReferenceErrorObject::ReferenceErrorObject(const Value &msg, const QString &fileName, int lineNumber, int columnNumber)
    : Heap::ErrorObject(msg, fileName, lineNumber, columnNumber, ReferenceError)
{
}

Heap::TypeErrorObject::TypeErrorObject(const Value &message)
    : Heap::ErrorObject(message, TypeError)
{
}

Heap::URIErrorObject::URIErrorObject(const Value &message)
    : Heap::ErrorObject(message, URIError)
{
}

DEFINE_OBJECT_VTABLE(ErrorCtor);
DEFINE_OBJECT_VTABLE(EvalErrorCtor);
DEFINE_OBJECT_VTABLE(RangeErrorCtor);
DEFINE_OBJECT_VTABLE(ReferenceErrorCtor);
DEFINE_OBJECT_VTABLE(SyntaxErrorCtor);
DEFINE_OBJECT_VTABLE(TypeErrorCtor);
DEFINE_OBJECT_VTABLE(URIErrorCtor);

Heap::ErrorCtor::ErrorCtor(QV4::ExecutionContext *scope)
    : Heap::FunctionObject(scope, QStringLiteral("Error"))
{
}

Heap::ErrorCtor::ErrorCtor(QV4::ExecutionContext *scope, const QString &name)
    : Heap::FunctionObject(scope, name)
{
}

ReturnedValue ErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const ErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<ErrorObject>(scope.engine, v)->asReturnedValue();
}

ReturnedValue ErrorCtor::call(const Managed *that, CallData *callData)
{
    return static_cast<const Object *>(that)->construct(callData);
}

Heap::EvalErrorCtor::EvalErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("EvalError"))
{
}

ReturnedValue EvalErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const EvalErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<EvalErrorObject>(scope.engine, v)->asReturnedValue();
}

Heap::RangeErrorCtor::RangeErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("RangeError"))
{
}

ReturnedValue RangeErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const RangeErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<RangeErrorObject>(scope.engine, v)->asReturnedValue();
}

Heap::ReferenceErrorCtor::ReferenceErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("ReferenceError"))
{
}

ReturnedValue ReferenceErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const ReferenceErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<ReferenceErrorObject>(scope.engine, v)->asReturnedValue();
}

Heap::SyntaxErrorCtor::SyntaxErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("SyntaxError"))
{
}

ReturnedValue SyntaxErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const SyntaxErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<SyntaxErrorObject>(scope.engine, v)->asReturnedValue();
}

Heap::TypeErrorCtor::TypeErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("TypeError"))
{
}

ReturnedValue TypeErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const TypeErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<TypeErrorObject>(scope.engine, v)->asReturnedValue();
}

Heap::URIErrorCtor::URIErrorCtor(QV4::ExecutionContext *scope)
    : Heap::ErrorCtor(scope, QStringLiteral("URIError"))
{
}

ReturnedValue URIErrorCtor::construct(const Managed *m, CallData *callData)
{
    Scope scope(static_cast<const URIErrorCtor *>(m)->engine());
    ScopedValue v(scope, callData->argument(0));
    return ErrorObject::create<URIErrorObject>(scope.engine, v)->asReturnedValue();
}

void ErrorPrototype::init(ExecutionEngine *engine, Object *ctor, Object *obj, Heap::ErrorObject::ErrorType t)
{
    Scope scope(engine);
    ScopedString s(scope);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = obj));
    ctor->defineReadonlyProperty(engine->id_length(), Primitive::fromInt32(1));
    *obj->propertyData(Index_Constructor) = ctor;
    *obj->propertyData(Index_Message) = engine->id_empty();
    *obj->propertyData(Index_Name) = engine->newString(QString::fromLatin1(ErrorObject::className(t)));
    if (t == Heap::ErrorObject::Error)
        obj->defineDefaultProperty(engine->id_toString(), method_toString, 0);
}

ReturnedValue ErrorPrototype::method_toString(CallContext *ctx)
{
    Scope scope(ctx);

    Object *o = ctx->thisObject().as<Object>();
    if (!o)
        return ctx->engine()->throwTypeError();

    ScopedValue name(scope, o->get(ctx->d()->engine->id_name()));
    QString qname;
    if (name->isUndefined())
        qname = QStringLiteral("Error");
    else
        qname = name->toQString();

    ScopedString s(scope, ctx->d()->engine->newString(QStringLiteral("message")));
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

    return ctx->d()->engine->newString(str)->asReturnedValue();
}
