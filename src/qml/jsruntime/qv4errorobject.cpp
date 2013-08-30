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
#include <cassert>

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
    type = Type_ErrorObject;
    vtbl = &static_vtbl;
    defineDefaultProperty(ic->engine, QStringLiteral("name"), Value::fromString(ic->engine, "Error"));
}

ErrorObject::ErrorObject(InternalClass *ic, const Value &message, ErrorType t)
    : Object(ic)
    , stack(0)
{
    type = Type_ErrorObject;
    vtbl = &static_vtbl;
    subtype = t;
    defineAccessorProperty(ic->engine, QStringLiteral("stack"), ErrorObject::method_get_stack, 0);

    if (!message.isUndefined())
        defineDefaultProperty(ic->engine->newString(QStringLiteral("message")), message);
    defineDefaultProperty(ic->engine, QStringLiteral("name"), Value::fromString(ic->engine, className()));

    stackTrace = ic->engine->stackTrace();
    if (!stackTrace.isEmpty()) {
        defineDefaultProperty(ic->engine, QStringLiteral("fileName"), Value::fromString(ic->engine, stackTrace.at(0).source));
        defineDefaultProperty(ic->engine, QStringLiteral("lineNumber"), Value::fromInt32(stackTrace.at(0).line));
    }
}

ErrorObject::ErrorObject(InternalClass *ic, const QString &message, const QString &fileName, int line, int column, ErrorObject::ErrorType t)
    : Object(ic)
    , stack(0)
{
    type = Type_ErrorObject;
    vtbl = &static_vtbl;
    subtype = t;
    defineAccessorProperty(ic->engine, QStringLiteral("stack"), ErrorObject::method_get_stack, 0);

    defineDefaultProperty(ic->engine, QStringLiteral("name"), Value::fromString(ic->engine, className()));

    stackTrace = ic->engine->stackTrace();
    ExecutionEngine::StackFrame frame;
    frame.source = fileName;
    frame.line = line;
    frame.column = column;
    stackTrace.prepend(frame);

    if (!stackTrace.isEmpty()) {
        defineDefaultProperty(ic->engine, QStringLiteral("fileName"), Value::fromString(ic->engine, stackTrace.at(0).source));
        defineDefaultProperty(ic->engine, QStringLiteral("lineNumber"), Value::fromInt32(stackTrace.at(0).line));
    }

    defineDefaultProperty(ic->engine, QStringLiteral("message"), Value::fromString(ic->engine->newString(message)));
}

Value ErrorObject::method_get_stack(SimpleCallContext *ctx)
{
    ErrorObject *This = ctx->thisObject.asErrorObject();
    if (!This)
        ctx->throwTypeError();
    if (!This->stack) {
        QString trace;
        for (int i = 0; i < This->stackTrace.count(); ++i) {
            if (i > 0)
                trace += QLatin1Char('\n');
            const ExecutionEngine::StackFrame &frame = This->stackTrace[i];
            trace += frame.function;
            trace += QLatin1Char('@');
            trace += frame.source;
            if (frame.line >= 0) {
                trace += QLatin1Char(':');
                trace += QString::number(frame.line);
            }
        }
        This->stack = ctx->engine->newString(trace);
    }
    return Value::fromString(This->stack);
}

void ErrorObject::markObjects(Managed *that)
{
    ErrorObject *This = that->asErrorObject();
    if (This->stack)
        This->stack->mark();
    Object::markObjects(that);
}

DEFINE_MANAGED_VTABLE(ErrorObject);

DEFINE_MANAGED_VTABLE(SyntaxErrorObject);

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const Value &msg)
    : ErrorObject(engine->syntaxErrorClass, msg, SyntaxError)
{
    vtbl = &static_vtbl;
}

SyntaxErrorObject::SyntaxErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber)
    : ErrorObject(engine->syntaxErrorClass, msg, fileName, lineNumber, columnNumber, SyntaxError)
{
    vtbl = &static_vtbl;
}

EvalErrorObject::EvalErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine->evalErrorClass, message, EvalError)
{
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine->rangeErrorClass, message, RangeError)
{
}

RangeErrorObject::RangeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->rangeErrorClass, Value::fromString(engine, message), RangeError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine->referenceErrorClass, message, ReferenceError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->referenceErrorClass, Value::fromString(engine, message), ReferenceError)
{
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber)
    : ErrorObject(engine->referenceErrorClass, msg, fileName, lineNumber, columnNumber, ReferenceError)
{
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine->typeErrorClass, message, TypeError)
{
}

TypeErrorObject::TypeErrorObject(ExecutionEngine *engine, const QString &message)
    : ErrorObject(engine->typeErrorClass, Value::fromString(engine, message), TypeError)
{
}

URIErrorObject::URIErrorObject(ExecutionEngine *engine, const Value &message)
    : ErrorObject(engine->uriErrorClass, message, URIError)
{
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

Value ErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(m->engine()->newErrorObject(d.argc ? d.args[0] : Value::undefinedValue()));
}

Value ErrorCtor::call(Managed *that, const CallData &d)
{
    return that->construct(d);
}

EvalErrorCtor::EvalErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("EvalError"))
{
    vtbl = &static_vtbl;
}

Value EvalErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) EvalErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
}

RangeErrorCtor::RangeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("RangeError"))
{
    vtbl = &static_vtbl;
}

Value RangeErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) RangeErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
}

ReferenceErrorCtor::ReferenceErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("ReferenceError"))
{
    vtbl = &static_vtbl;
}

Value ReferenceErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) ReferenceErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
}

SyntaxErrorCtor::SyntaxErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("SyntaxError"))
{
    vtbl = &static_vtbl;
}

Value SyntaxErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) SyntaxErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
}

TypeErrorCtor::TypeErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("TypeError"))
{
    vtbl = &static_vtbl;
}

Value TypeErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) TypeErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
}

URIErrorCtor::URIErrorCtor(ExecutionContext *scope)
    : ErrorCtor(scope, scope->engine->newIdentifier("URIError"))
{
    vtbl = &static_vtbl;
}

Value URIErrorCtor::construct(Managed *m, const CallData &d)
{
    return Value::fromObject(new (m->engine()->memoryManager) URIErrorObject(m->engine(), d.argc ? d.args[0] : Value::undefinedValue()));
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

    Value name = o->get(ctx->engine->newString(QString::fromLatin1("name")));
    QString qname;
    if (name.isUndefined())
        qname = QString::fromLatin1("Error");
    else
        qname = __qmljs_to_string(name, ctx).stringValue()->toQString();

    Value message = o->get(ctx->engine->newString(QString::fromLatin1("message")));
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
