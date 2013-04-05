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
#include <qv4context.h>
#include <qv4object.h>
#include <qv4objectproto.h>
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

void ExecutionContext::createMutableBinding(String *name, bool deletable)
{
    if (!activation)
        activation = engine->newObject();

    if (activation->__hasProperty__(this, name))
        return;
    PropertyDescriptor desc;
    desc.value = Value::undefinedValue();
    desc.type = PropertyDescriptor::Data;
    desc.configurable = deletable ? PropertyDescriptor::Enabled : PropertyDescriptor::Disabled;
    desc.writable = PropertyDescriptor::Enabled;
    desc.enumerable = PropertyDescriptor::Enabled;
    activation->__defineOwnProperty__(this, name, &desc);
}

bool ExecutionContext::setMutableBinding(ExecutionContext *scope, String *name, const Value &value)
{
    // ### throw if scope->strict is true, and it would change an immutable binding
    if (type == Type_CallContext) {
        CallContext *c = static_cast<CallContext *>(this);
        for (unsigned int i = 0; i < c->function->varCount; ++i)
            if (c->function->varList[i]->isEqualTo(name)) {
                c->locals[i] = value;
                return true;
            }
        for (int i = (int)c->function->formalParameterCount - 1; i >= 0; --i)
            if (c->function->formalParameterList[i]->isEqualTo(name)) {
                c->arguments[i] = value;
                return true;
            }
    }

    if (activation && (type == Type_QmlContext || activation->__hasProperty__(scope, name))) {
        activation->put(scope, name, value);
        return true;
    }

    return false;
}

String * const *ExecutionContext::formals() const
{
    return type == Type_CallContext ? static_cast<const CallContext *>(this)->function->formalParameterList : 0;
}

unsigned int ExecutionContext::formalCount() const
{
    return type == Type_CallContext ? static_cast<const CallContext *>(this)->function->formalParameterCount : 0;
}

String * const *ExecutionContext::variables() const
{
    return type == Type_CallContext ? static_cast<const CallContext *>(this)->function->varList : 0;
}

unsigned int ExecutionContext::variableCount() const
{
    return type == Type_CallContext ? static_cast<const CallContext *>(this)->function->varCount : 0;
}


void GlobalContext::init(ExecutionEngine *eng)
{
    type = Type_GlobalContext;
    strictMode = false;
    marked = false;
    thisObject = eng->globalObject;
    engine = eng;
    outer = 0;
    lookups = 0;

    // ### remove
    arguments = 0;
    argumentCount = 0;
    activation = 0;
}

void WithContext::init(ExecutionContext *p, Object *with)
{
    type = Type_WithContext;
    strictMode = false;
    marked = false;
    thisObject = p->thisObject;
    engine = p->engine;
    outer = p;
    lookups = p->lookups;

    withObject = with;

    // ### remove
    arguments = 0;
    argumentCount = 0;
    activation = 0;
}

void CatchContext::init(ExecutionContext *p, String *exceptionVarName, const Value &exceptionValue)
{
    type = Type_CatchContext;
    strictMode = p->strictMode;
    marked = false;
    thisObject = p->thisObject;
    engine = p->engine;
    outer = p;
    lookups = p->lookups;

    this->exceptionVarName = exceptionVarName;
    this->exceptionValue = exceptionValue;

    // ### remove
    arguments = 0;
    argumentCount = 0;
    activation = 0;
}

void CallContext::initCallContext(ExecutionEngine *engine)
{
    type = Type_CallContext;
    strictMode = function->strictMode;
    marked = false;
    this->engine = engine;
    outer = function->scope;
#ifndef QT_NO_DEBUG
    assert(outer->next != (ExecutionContext *)0x1);
#endif

    activation = 0;

    if (function->function)
        lookups = function->function->lookups;

    uint argc = argumentCount;

    locals = (Value *)(this + 1);
    if (function->varCount)
        std::fill(locals, locals + function->varCount, Value::undefinedValue());

    if (needsOwnArguments()) {
        Value *args = arguments;
        argumentCount = qMax(argc, function->formalParameterCount);
        arguments = locals + function->varCount;
        if (argc)
            ::memcpy(arguments, args, argc * sizeof(Value));
        if (argc < function->formalParameterCount)
            std::fill(arguments + argc, arguments + function->formalParameterCount, Value::undefinedValue());

    }

    if (function->usesArgumentsObject) {
        ArgumentsObject *args = new (engine->memoryManager) ArgumentsObject(this, function->formalParameterCount, argc);
        args->prototype = engine->objectPrototype;
        Value arguments = Value::fromObject(args);
        createMutableBinding(engine->id_arguments, false);
        setMutableBinding(this, engine->id_arguments, arguments);
    }

    if (engine->debugger)
        engine->debugger->aboutToCall(function, this);
}


bool ExecutionContext::deleteProperty(String *name)
{
    bool hasWith = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            hasWith = true;
            WithContext *w = static_cast<WithContext *>(ctx);
            if (w->withObject->__hasProperty__(this, name))
                return w->withObject->deleteProperty(this, name);
        } else {
            if (ctx->activation && ctx->activation->__hasProperty__(this, name))
                return ctx->activation->deleteProperty(this, name);
        }
        if (ctx->type == Type_CatchContext) {
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return false;
        }
        if (ctx->type == Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return false;
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return false;
            }
        }
    }
    if (strictMode)
        throwSyntaxError(0);
    return true;
}

bool CallContext::needsOwnArguments() const
{
    return function->needsActivation || argumentCount < function->formalParameterCount;
}

void ExecutionContext::mark()
{
    if (marked)
        return;
    marked = true;

    if (outer)
        outer->mark();

    thisObject.mark();
    for (unsigned arg = 0, lastArg = argumentCount; arg < lastArg; ++arg)
        arguments[arg].mark();

    if (type == Type_CallContext) {
        VM::CallContext *c = static_cast<CallContext *>(this);
        for (unsigned local = 0, lastLocal = c->variableCount(); local < lastLocal; ++local)
            c->locals[local].mark();
        c->function->mark();
    }

    if (activation)
        activation->mark();
    if (type == Type_WithContext) {
        WithContext *w = static_cast<WithContext *>(this);
        w->withObject->mark();
    }
    if (type == Type_CatchContext) {
        CatchContext *c = static_cast<CatchContext *>(this);
        if (c->exceptionVarName)
            c->exceptionVarName->mark();
        c->exceptionValue.mark();
    }
}

void ExecutionContext::setProperty(String *name, const Value& value)
{
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            if (w->__hasProperty__(ctx, name)) {
                w->put(ctx, name, value);
                return;
            }
        } else if (ctx->type == Type_CatchContext && static_cast<CatchContext *>(ctx)->exceptionVarName->isEqualTo(name)) {
            static_cast<CatchContext *>(ctx)->exceptionValue = value;
            return;
        } else {
            if (ctx->setMutableBinding(this, name, value))
                return;
        }
    }
    if (strictMode || name->isEqualTo(engine->id_this))
        throwReferenceError(Value::fromString(name));
    engine->globalObject.objectValue()->put(this, name, value);
}

Value ExecutionContext::getProperty(String *name)
{
    name->makeIdentifier(this);

    if (name->isEqualTo(engine->id_this))
        return thisObject;

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            hasWith = true;
            bool hasProperty = false;
            Value v = w->get(ctx, name, &hasProperty);
            if (hasProperty) {
                return v;
            }
            continue;
        }

        if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        if (ctx->type == Type_CallContext) {
            VM::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->get(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
        if (ctx->type == Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }
    }
    throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}

Value ExecutionContext::getPropertyNoThrow(String *name)
{
    name->makeIdentifier(this);

    if (name->isEqualTo(engine->id_this))
        return thisObject;

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            hasWith = true;
            bool hasProperty = false;
            Value v = w->get(ctx, name, &hasProperty);
            if (hasProperty) {
                return v;
            }
            continue;
        }

        if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        if (ctx->type == Type_CallContext) {
            VM::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->get(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
        if (ctx->type == Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }
    }
    return Value::undefinedValue();
}

Value ExecutionContext::getPropertyAndBase(String *name, Object **base)
{
    *base = 0;
    name->makeIdentifier(this);

    if (name->isEqualTo(engine->id_this))
        return thisObject;

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            hasWith = true;
            bool hasProperty = false;
            Value v = w->get(ctx, name, &hasProperty);
            if (hasProperty) {
                *base = w;
                return v;
            }
            continue;
        }

        if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        if (ctx->type == Type_CallContext) {
            VM::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
        }
        if (ctx->activation) {
            bool hasProperty = false;
            Value v = ctx->activation->get(ctx, name, &hasProperty);
            if (hasProperty)
                return v;
        }
        if (ctx->type == Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }
    }
    throwReferenceError(Value::fromString(name));
    return Value::undefinedValue();
}



void ExecutionContext::inplaceBitOp(String *name, const Value &value, BinOp op)
{
    Value lhs = getProperty(name);
    Value result;
    op(this, &result, lhs, value);
    setProperty(name, result);
}

void ExecutionContext::throwError(const Value &value)
{
    __qmljs_builtin_throw(this, value);
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

void ExecutionContext::throwURIError(Value msg)
{
    throwError(Value::fromObject(engine->newURIErrorObject(this, msg)));
}

} // namespace VM
} // namespace QQmlJS
