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

#include <QString>
#include "debugging.h"
#include <qmljs_environment.h>
#include <qv4object.h>
#include <qv4ecmaobjects_p.h>
#include "qv4mm.h"
#include <qv4argumentsobject.h>

namespace QQmlJS {
namespace VM {

DiagnosticMessage::DiagnosticMessage()
    : offset(0)
    , length(0)
    , startLine(0)
    , startColumn(0)
    , type(0)
    , next(0)
{}

DiagnosticMessage::~DiagnosticMessage()
{
    delete next;
}

String *DiagnosticMessage::buildFullMessage(ExecutionContext *ctx) const
{
    QString msg;
    if (!fileName.isEmpty())
        msg = fileName + QLatin1Char(':');
    msg += QString::number(startLine) + QLatin1Char(':') + QString::number(startColumn) + QLatin1String(": ");
    if (type == QQmlJS::VM::DiagnosticMessage::Error)
        msg += QLatin1String("error");
    else
        msg += QLatin1String("warning");
    msg += ": " + message;

    return ctx->engine->newString(msg);
}

bool ExecutionContext::hasBinding(String *name) const
{
    if (!function)
        return false;

    for (unsigned int i = 0; i < function->varCount; ++i) {
        if (function->varList[i]->isEqualTo(name))
            return true;
    }
    for (unsigned int i = 0; i < function->formalParameterCount; ++i) {
        if (function->formalParameterList[i]->isEqualTo(name))
            return true;
    }
    if (activation)
        return activation->__hasProperty__(this, name);
    return false;
}

void ExecutionContext::createMutableBinding(String *name, bool deletable)
{
    if (!activation)
        activation = engine->newActivationObject();

    if (activation->__hasProperty__(this, name))
        return;
    PropertyDescriptor desc;
    desc.value = Value::undefinedValue();
    desc.type = PropertyDescriptor::Data;
    desc.configurable = deletable ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;
    desc.writable = PropertyDescriptor::Enabled;
    desc.enumberable = PropertyDescriptor::Enabled;
    activation->__defineOwnProperty__(this, name, &desc);
}

bool ExecutionContext::setMutableBinding(ExecutionContext *scope, String *name, Value value)
{
    // ### throw if scope->strict is true, and it would change an immutable binding
    if (function) {
        for (unsigned int i = 0; i < function->varCount; ++i)
            if (function->varList[i]->isEqualTo(name)) {
                locals[i] = value;
                return true;
            }
        for (int i = (int)function->formalParameterCount - 1; i >= 0; --i)
            if (function->formalParameterList[i]->isEqualTo(name)) {
                arguments[i] = value;
                return true;
            }
    }

    if (activation && activation->__hasProperty__(scope, name)) {
        activation->__put__(scope, name, value);
        return true;
    }

    return false;
}

Value ExecutionContext::getBindingValue(ExecutionContext *scope, String *name, bool strict) const
{
    Q_UNUSED(strict);
    assert(function);

    if (function) {
        for (unsigned int i = 0; i < function->varCount; ++i)
            if (function->varList[i]->isEqualTo(name))
                return locals[i];
        for (int i = (int)function->formalParameterCount - 1; i >= 0; --i)
            if (function->formalParameterList[i]->isEqualTo(name))
                return arguments[i];
    }

    if (activation) {
        bool hasProperty = false;
        Value v = activation->__get__(scope, name, &hasProperty);
        if (hasProperty)
            return v;
    }
    assert(false);
}

bool ExecutionContext::deleteBinding(ExecutionContext *scope, String *name)
{
    if (activation)
        activation->__delete__(scope, name);

    if (scope->strictMode)
        __qmljs_throw_type_error(scope);
    return false;
}

void ExecutionContext::pushWithObject(Object *with)
{
    With *w = new With;
    w->next = withObject;
    w->object = with;
    withObject = w;
}

void ExecutionContext::popWithObject()
{
    assert(withObject);

    With *w = withObject;
    withObject = w->next;
    delete w;
}

ExecutionContext *ExecutionContext::outer() const
{
    return function ? function->scope : 0;
}

String **ExecutionContext::formals() const
{
    return function ? function->formalParameterList : 0;
}

unsigned int ExecutionContext::formalCount() const
{
    return function ? function->formalParameterCount : 0;
}

String **ExecutionContext::variables() const
{
    return function ? function->varList : 0;
}

unsigned int ExecutionContext::variableCount() const
{
    return function ? function->varCount : 0;
}


void ExecutionContext::init(ExecutionEngine *eng)
{
    engine = eng;
    parent = 0;
    thisObject = eng->globalObject;

    function = 0;
    arguments = 0;
    argumentCount = 0;
    locals = 0;
    strictMode = false;
    activation = 0;
    withObject = 0;

    eng->exception = Value::undefinedValue();
}

void ExecutionContext::destroy()
{
    delete[] arguments;
    delete[] locals;
}

bool ExecutionContext::deleteProperty(String *name)
{
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer()) {
        if (ctx->withObject) {
            ExecutionContext::With *w = ctx->withObject;
            while (w) {
                if (w->object->__hasProperty__(this, name))
                    return w->object->__delete__(this, name);
                w = w->next;
            }
        }
        if (ctx->activation && ctx->activation->__hasProperty__(this, name))
            return ctx->activation->__delete__(this, name);
    }
    if (strictMode)
        throwSyntaxError(0);
    return true;
}

void ExecutionContext::setProperty(String *name, Value value)
{
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer()) {
        if (ctx->withObject) {
            With *w = ctx->withObject;
            while (w) {
                if (w->object->__hasProperty__(ctx, name)) {
                    w->object->__put__(ctx, name, value);
                    return;
                }
                w = w->next;
            }
        }
        if (ctx->setMutableBinding(this, name, value))
            return;
    }
    if (strictMode || name == engine->id_this)
        throwReferenceError(Value::fromString(name));
    engine->globalObject.objectValue()->__put__(this, name, value);
}

Value ExecutionContext::getProperty(String *name)
{
    if (name == engine->id_this)
        return thisObject;

    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer()) {
        if (With *w = ctx->withObject) {
            while (w) {
                bool hasProperty = false;
                Value v = w->object->__get__(ctx, name, &hasProperty);
                if (hasProperty)
                    return v;
                w = w->next;
            }
        }

        if (FunctionObject *f = ctx->function) {
            if (f->needsActivation || ctx->withObject) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return ctx->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return ctx->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->__get__(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
    }
    throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}

Value ExecutionContext::getPropertyNoThrow(String *name)
{
    if (name == engine->id_this)
        return thisObject;

    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer()) {
        if (With *w = ctx->withObject) {
            while (w) {
                bool hasProperty = false;
                Value v = w->object->__get__(ctx, name, &hasProperty);
                if (hasProperty)
                    return v;
                w = w->next;
            }
        }

        if (FunctionObject *f = ctx->function) {
            if (f->needsActivation || ctx->withObject) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return ctx->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return ctx->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->__get__(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
    }
    return Value::undefinedValue();
}

Value ExecutionContext::getPropertyAndBase(String *name, Object **base)
{
    *base = 0;

    if (name == engine->id_this)
        return thisObject;

    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer()) {
        if (With *w = ctx->withObject) {
            while (w) {
                bool hasProperty = false;
                Value v = w->object->__get__(ctx, name, &hasProperty);
                if (hasProperty) {
                    *base = w->object;
                    return v;
                }
                w = w->next;
            }
        }

        if (FunctionObject *f = ctx->function) {
            if (f->needsActivation || ctx->withObject) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return ctx->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return ctx->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->__get__(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
    }
    throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}



void ExecutionContext::inplaceBitOp(Value value, String *name, BinOp op)
{
    Value lhs = getProperty(name);
    value = op(lhs, value, this);
    setProperty(name, value);
}

void ExecutionContext::throwError(Value value)
{
    __qmljs_builtin_throw(value, this);
}

void ExecutionContext::throwError(const QString &message)
{
    Value v = Value::fromString(this, message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void ExecutionContext::throwSyntaxError(DiagnosticMessage *message)
{
    throwError(Value::fromObject(engine->newSyntaxErrorObject(this, message)));
}

void ExecutionContext::throwTypeError()
{
    throwError(Value::fromObject(engine->newTypeErrorObject(this, QStringLiteral("Type error"))));
}

void ExecutionContext::throwUnimplemented(const QString &message)
{
    Value v = Value::fromString(this, QStringLiteral("Unimplemented ") + message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void ExecutionContext::throwReferenceError(Value value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newReferenceErrorObject(this, msg)));
}

void ExecutionContext::throwRangeError(Value value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" out of range");
    throwError(Value::fromObject(engine->newRangeErrorObject(this, msg)));
}

void ExecutionContext::initCallContext(ExecutionContext *parent, const Value that, FunctionObject *f, Value *args, unsigned argc)
{
    MemoryManager::GCBlocker blockGC(parent->engine->memoryManager);

    engine = parent->engine;
    this->parent = parent;
    engine->current = this;

    function = f;
    strictMode = f->strictMode;

    thisObject = that;
    if (!strictMode && !thisObject.isObject()) {
        if (thisObject.isUndefined() || thisObject.isNull())
            thisObject = engine->globalObject;
        else
            thisObject = thisObject.toObject(this);
    }

    arguments = args;
    argumentCount = argc;
    if (function->needsActivation || argc < function->formalParameterCount){
        argumentCount = qMax(argc, function->formalParameterCount);
        arguments = new Value[argumentCount];
        if (argc)
            std::copy(args, args + argc, arguments);
        if (argc < function->formalParameterCount)
            std::fill(arguments + argc, arguments + function->formalParameterCount, Value::undefinedValue());
    }

    locals = function->varCount ? new Value[function->varCount] : 0;
    if (function->varCount)
        std::fill(locals, locals + function->varCount, Value::undefinedValue());

    activation = 0;
    withObject = 0;

    if (function->usesArgumentsObject) {
        ArgumentsObject *args = new (engine->memoryManager) ArgumentsObject(this, function->formalParameterCount, argc);
        args->prototype = engine->objectPrototype;
        Value arguments = Value::fromObject(args);
        createMutableBinding(engine->id_arguments, false);
        setMutableBinding(this, engine->id_arguments, arguments);
    }

    if (engine->debugger)
        engine->debugger->aboutToCall(f, this);
}

void ExecutionContext::leaveCallContext()
{
    if (!function->needsActivation) {
        delete[] locals;
        locals = 0;
    }
    engine->current = parent;
    parent = 0;

    if (engine->debugger)
        engine->debugger->justLeft(this);
}

void ExecutionContext::wireUpPrototype()
{
    assert(thisObject.isObject());

    Value proto = function->__get__(this, engine->id_prototype);
    if (proto.isObject())
        thisObject.objectValue()->prototype = proto.objectValue();
    else
        thisObject.objectValue()->prototype = engine->objectPrototype;
}

} // namespace VM
} // namespace QQmlJS
