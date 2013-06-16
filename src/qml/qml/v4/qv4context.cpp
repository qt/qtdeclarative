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
#include "qv4debugging_p.h"
#include <qv4context_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include "qv4mm_p.h"
#include <qv4argumentsobject_p.h>
#include "qv4function_p.h"

using namespace QV4;

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
    if (type == QV4::DiagnosticMessage::Error)
        msg += QLatin1String("error");
    else
        msg += QLatin1String("warning");
    msg += ": " + message;

    return ctx->engine->newString(msg);
}

void ExecutionContext::createMutableBinding(String *name, bool deletable)
{

    // find the right context to create the binding on
    Object *activation = engine->globalObject;
    ExecutionContext *ctx = this;
    while (ctx) {
        if (ctx->type >= Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            if (!c->activation)
                c->activation = engine->newObject();
            activation = c->activation;
            break;
        }
        ctx = ctx->outer;
    }

    if (activation->__hasProperty__(name))
        return;
    Property desc = Property::fromValue(Value::undefinedValue());
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    activation->__defineOwnProperty__(this, name, desc, attrs);
}

String * const *ExecutionContext::formals() const
{
    return type >= Type_CallContext ? static_cast<const CallContext *>(this)->function->formalParameterList : 0;
}

unsigned int ExecutionContext::formalCount() const
{
    return type >= Type_CallContext ? static_cast<const CallContext *>(this)->function->formalParameterCount : 0;
}

String * const *ExecutionContext::variables() const
{
    return type >= Type_CallContext ? static_cast<const CallContext *>(this)->function->varList : 0;
}

unsigned int ExecutionContext::variableCount() const
{
    return type >= Type_CallContext ? static_cast<const CallContext *>(this)->function->varCount : 0;
}


void GlobalContext::initGlobalContext(ExecutionEngine *eng)
{
    initBaseContext(Type_GlobalContext, eng);
    thisObject = Value::fromObject(eng->globalObject);
    global = 0;
}

void WithContext::initWithContext(ExecutionContext *p, Object *with)
{
    initBaseContext(Type_WithContext, p->engine);
    thisObject = p->thisObject;
    outer = p;
    lookups = p->lookups;

    withObject = with;
}

void CatchContext::initCatchContext(ExecutionContext *p, String *exceptionVarName, const Value &exceptionValue)
{
    initBaseContext(Type_CatchContext, p->engine);
    strictMode = p->strictMode;
    thisObject = p->thisObject;
    outer = p;
    lookups = p->lookups;

    this->exceptionVarName = exceptionVarName;
    this->exceptionValue = exceptionValue;
}

void CallContext::initCallContext(ExecutionEngine *engine, FunctionObject *function, Value *_arguments, int _argumentCount, const Value &_thisObject)
{
    initBaseContext(Type_CallContext, engine);

    this->function = function;
    this->arguments = _arguments;
    this->argumentCount = _argumentCount;
    this->thisObject = _thisObject;

    strictMode = function->strictMode;
    marked = false;
    outer = function->scope;
#ifndef QT_NO_DEBUG
    assert(outer->next != (ExecutionContext *)0x1);
#endif

    activation = 0;

    if (function->function)
        lookups = function->function->lookups;

    uint argc = argumentCount;

    locals = (Value *)(this + 1);

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
        activation = engine->newObject();
        Property desc = Property::fromValue(Value::fromObject(args));
        activation->__defineOwnProperty__(this, engine->id_arguments, desc, Attr_NotConfigurable);
    }
}

void CallContext::initQmlContext(ExecutionEngine *engine, Object *qml, FunctionObject *function)
{
    initBaseContext(Type_QmlContext, engine);

    this->function = function;
    this->arguments = 0;
    this->argumentCount = 0;
    this->thisObject = Value::undefinedValue();

    strictMode = true;
    marked = false;
    this->outer = function->scope;
#ifndef QT_NO_DEBUG
    assert(outer->next != (ExecutionContext *)0x1);
#endif

    activation = qml;

    lookups = function->function->lookups;

    locals = (Value *)(this + 1);
    if (function->varCount)
        std::fill(locals, locals + function->varCount, Value::undefinedValue());
}


bool ExecutionContext::deleteProperty(String *name)
{
    bool hasWith = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            hasWith = true;
            WithContext *w = static_cast<WithContext *>(ctx);
            if (w->withObject->__hasProperty__(name))
                return w->withObject->deleteProperty(this, name);
        } else if (ctx->type == Type_CatchContext) {
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return false;
        } else if (ctx->type >= Type_CallContext) {
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
            if (c->activation && c->activation->__hasProperty__(name))
                return c->activation->deleteProperty(this, name);
        } else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            if (g->global->__hasProperty__(name))
                return g->global->deleteProperty(this, name);
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

    if (type != Type_SimpleCallContext && outer)
        outer->mark();

    thisObject.mark();

    if (type >= Type_SimpleCallContext) {
        QV4::CallContext *c = static_cast<CallContext *>(this);
        for (unsigned arg = 0, lastArg = c->argumentCount; arg < lastArg; ++arg)
            c->arguments[arg].mark();
        if (type >= Type_CallContext) {
            for (unsigned local = 0, lastLocal = c->variableCount(); local < lastLocal; ++local)
                c->locals[local].mark();
            if (c->activation)
                c->activation->mark();
            c->function->mark();
        }
    } else if (type == Type_WithContext) {
        WithContext *w = static_cast<WithContext *>(this);
        w->withObject->mark();
    } else if (type == Type_CatchContext) {
        CatchContext *c = static_cast<CatchContext *>(this);
        if (c->exceptionVarName)
            c->exceptionVarName->mark();
        c->exceptionValue.mark();
    } else if (type == Type_GlobalContext) {
        GlobalContext *g = static_cast<GlobalContext *>(this);
        g->global->mark();
    }
}

void ExecutionContext::setProperty(String *name, const Value& value)
{
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            if (w->__hasProperty__(name)) {
                w->put(ctx, name, value);
                return;
            }
        } else if (ctx->type == Type_CatchContext && static_cast<CatchContext *>(ctx)->exceptionVarName->isEqualTo(name)) {
            static_cast<CatchContext *>(ctx)->exceptionValue = value;
            return;
        } else {
            Object *activation = 0;
            if (ctx->type >= Type_CallContext) {
                CallContext *c = static_cast<CallContext *>(ctx);
                for (unsigned int i = 0; i < c->function->varCount; ++i)
                    if (c->function->varList[i]->isEqualTo(name)) {
                        c->locals[i] = value;
                        return;
                    }
                for (int i = (int)c->function->formalParameterCount - 1; i >= 0; --i)
                    if (c->function->formalParameterList[i]->isEqualTo(name)) {
                        c->arguments[i] = value;
                        return;
                    }
                activation = c->activation;
            } else if (ctx->type == Type_GlobalContext) {
                activation = static_cast<GlobalContext *>(ctx)->global;
            }

            if (activation && (ctx->type == Type_QmlContext || activation->__hasProperty__(name))) {
                activation->put(this, name, value);
                return;
            }
        }
    }
    if (strictMode || name->isEqualTo(engine->id_this))
        throwReferenceError(Value::fromString(name));
    engine->globalObject->put(this, name, value);
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

        else if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        else if (ctx->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
            if (c->activation) {
                bool hasProperty = false;
                Value v = c->activation->get(c, name, &hasProperty);
                if (hasProperty)
                    return v;
            }
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }

        else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            Value v = g->global->get(g, name, &hasProperty);
            if (hasProperty)
                return v;
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

        else if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        else if (ctx->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
            if (c->activation) {
                bool hasProperty = false;
                Value v = c->activation->get(c, name, &hasProperty);
                if (hasProperty)
                    return v;
            }
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }

        else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            Value v = g->global->get(g, name, &hasProperty);
            if (hasProperty)
                return v;
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

        else if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue;
        }

        else if (ctx->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i];
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->arguments[i];
            }
            if (c->activation) {
                bool hasProperty = false;
                Value v = c->activation->get(c, name, &hasProperty);
                if (hasProperty) {
                    *base = c->activation;
                    return v;
                }
            }
            if (f->function && f->function->isNamedExpression
                && name->isEqualTo(f->function->name))
                return Value::fromObject(c->function);
        }

        else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            Value v = g->global->get(g, name, &hasProperty);
            if (hasProperty)
                return v;
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
    __qmljs_throw(this, value);
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
    throwError(Value::fromObject(engine->newTypeErrorObject(QStringLiteral("Type error"))));
}

void ExecutionContext::throwTypeError(const QString &message)
{
    throwError(Value::fromObject(engine->newTypeErrorObject(message)));
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
    throwError(Value::fromObject(engine->newReferenceErrorObject(msg)));
}

void ExecutionContext::throwReferenceError(Value value, const QString &fileName, int line)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newReferenceErrorObject(msg, fileName, line)));
}

void ExecutionContext::throwRangeError(Value value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" out of range");
    throwError(Value::fromObject(engine->newRangeErrorObject(msg)));
}

void ExecutionContext::throwURIError(Value msg)
{
    throwError(Value::fromObject(engine->newURIErrorObject(msg)));
}
