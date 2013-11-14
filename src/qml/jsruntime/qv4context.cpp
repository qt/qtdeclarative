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

#include <QString>
#include "qv4debugging_p.h"
#include <qv4context_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include "qv4mm_p.h"
#include <qv4argumentsobject_p.h>
#include "qv4function_p.h"
#include "qv4errorobject_p.h"

using namespace QV4;

const ManagedVTable ExecutionContext::static_vtbl =
{
    call,
    construct,
    markObjects,
    destroy,
    0 /*collectDeletables*/,
    hasInstance,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    isEqualTo,
    0,
    "ExecutionContext",
};

CallContext *ExecutionContext::newCallContext(FunctionObject *function, CallData *callData)
{
    CallContext *c = static_cast<CallContext *>(engine->memoryManager->allocManaged(requiredMemoryForExecutionContect(function, callData->argc)));
    c->init();

    engine->current = c;

    c->initBaseContext(Type_CallContext, engine, this);

    c->function = function;
    c->realArgumentCount = callData->argc;

    c->strictMode = function->strictMode;
    c->outer = function->scope;
#ifndef QT_NO_DEBUG
    assert(c->outer->next != (ExecutionContext *)0x1);
#endif

    c->activation = 0;

    if (function->function) {
        c->compilationUnit = function->function->compilationUnit;
        c->lookups = c->compilationUnit->runtimeLookups;
    }

    c->locals = (SafeValue *)(c + 1);

    if (function->varCount)
        std::fill(c->locals, c->locals + function->varCount, Primitive::undefinedValue());

    c->callData = reinterpret_cast<CallData *>(c->locals + function->varCount);
    ::memcpy(c->callData, callData, sizeof(CallData) + (callData->argc - 1) * sizeof(SafeValue));
    if (callData->argc < static_cast<int>(function->formalParameterCount))
        std::fill(c->callData->args + c->callData->argc, c->callData->args + function->formalParameterCount, Primitive::undefinedValue());
    c->callData->argc = qMax((uint)callData->argc, function->formalParameterCount);

    return c;
}

WithContext *ExecutionContext::newWithContext(ObjectRef with)
{
    WithContext *w = new (engine->memoryManager) WithContext;
    engine->current = w;
    w->initWithContext(this, with);
    return w;
}

CatchContext *ExecutionContext::newCatchContext(const StringRef exceptionVarName, const ValueRef exceptionValue)
{
    CatchContext *c = new (engine->memoryManager) CatchContext;
    engine->current = c;
    c->initCatchContext(this, exceptionVarName, exceptionValue);
    return c;
}

CallContext *ExecutionContext::newQmlContext(FunctionObject *f, ObjectRef qml)
{
    CallContext *c = static_cast<CallContext *>(engine->memoryManager->allocManaged(requiredMemoryForExecutionContect(f, 0)));
    c->init();

    engine->current = c;
    c->initQmlContext(this, qml, f);

    return c;
}



void ExecutionContext::createMutableBinding(const StringRef name, bool deletable)
{
    Scope scope(this);

    // find the right context to create the binding on
    ScopedObject activation(scope, engine->globalObject);
    ExecutionContext *ctx = this;
    while (ctx) {
        if (ctx->type >= Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            if (!c->activation)
                c->activation = engine->newObject()->getPointer();
            activation = c->activation;
            break;
        }
        ctx = ctx->outer;
    }

    if (activation->__hasProperty__(name))
        return;
    Property desc = Property::fromValue(Primitive::undefinedValue());
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    activation->__defineOwnProperty__(this, name, desc, attrs);
}

String * const *ExecutionContext::formals() const
{
    if (type < Type_SimpleCallContext)
        return 0;
    QV4::FunctionObject *f = static_cast<const CallContext *>(this)->function;
    return f ? f->formalParameterList : 0;
}

unsigned int ExecutionContext::formalCount() const
{
    if (type < Type_SimpleCallContext)
        return 0;
    QV4::FunctionObject *f = static_cast<const CallContext *>(this)->function;
    return f ? f->formalParameterCount : 0;
}

String * const *ExecutionContext::variables() const
{
    if (type < Type_SimpleCallContext)
        return 0;
    QV4::FunctionObject *f = static_cast<const CallContext *>(this)->function;
    return f ? f->varList : 0;
}

unsigned int ExecutionContext::variableCount() const
{
    if (type < Type_SimpleCallContext)
        return 0;
    QV4::FunctionObject *f = static_cast<const CallContext *>(this)->function;
    return f ? f->varCount : 0;
}


void GlobalContext::initGlobalContext(ExecutionEngine *eng)
{
    initBaseContext(Type_GlobalContext, eng, /*parentContext*/0);
    callData = reinterpret_cast<CallData *>(this + 1);
    callData->tag = QV4::Value::_Integer_Type;
    callData->argc = 0;
    callData->thisObject = eng->globalObject;
    callData->args[0] = Encode::undefined();
    global = 0;
}

void WithContext::initWithContext(ExecutionContext *p, ObjectRef with)
{
    initBaseContext(Type_WithContext, p->engine, p);
    callData = p->callData;
    outer = p;
    lookups = p->lookups;
    compilationUnit = p->compilationUnit;

    withObject = with.getPointer();
}

void CatchContext::initCatchContext(ExecutionContext *p, const StringRef exceptionVarName, const ValueRef exceptionValue)
{
    initBaseContext(Type_CatchContext, p->engine, p);
    strictMode = p->strictMode;
    callData = p->callData;
    outer = p;
    lookups = p->lookups;
    compilationUnit = p->compilationUnit;

    this->exceptionVarName = exceptionVarName;
    this->exceptionValue = exceptionValue;
}

void CallContext::initQmlContext(ExecutionContext *parentContext, ObjectRef qml, FunctionObject *function)
{
    initBaseContext(Type_QmlContext, parentContext->engine, parentContext);

    this->function = function;
    this->callData = reinterpret_cast<CallData *>(this + 1);
    this->callData->tag = QV4::Value::_Integer_Type;
    this->callData->argc = 0;
    this->callData->thisObject = Primitive::undefinedValue();

    strictMode = true;
    this->outer = function->scope;
#ifndef QT_NO_DEBUG
    assert(outer->next != (ExecutionContext *)0x1);
#endif

    activation = qml.getPointer();

    if (function->function) {
        compilationUnit = function->function->compilationUnit;
        lookups = compilationUnit->runtimeLookups;
    }

    locals = (SafeValue *)(this + 1);
    if (function->varCount)
        std::fill(locals, locals + function->varCount, Primitive::undefinedValue());
}


bool ExecutionContext::deleteProperty(const StringRef name)
{
    Scope scope(this);
    bool hasWith = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            hasWith = true;
            WithContext *w = static_cast<WithContext *>(ctx);
            if (w->withObject->__hasProperty__(name))
                return w->withObject->deleteProperty(name);
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
                return c->activation->deleteProperty(name);
        } else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            if (g->global->__hasProperty__(name))
                return g->global->deleteProperty(name);
        }
    }

    if (strictMode)
        throwSyntaxError(QStringLiteral("Can't delete property %1").arg(name->toQString()));
    return true;
}

bool CallContext::needsOwnArguments() const
{
    return function->needsActivation || callData->argc < static_cast<int>(function->formalParameterCount);
}

void ExecutionContext::markObjects(Managed *m, ExecutionEngine *engine)
{
    ExecutionContext *ctx = static_cast<ExecutionContext *>(m);

    if (ctx->outer)
        ctx->outer->mark(engine);

    // ### shouldn't need these 3 lines
    ctx->callData->thisObject.mark(engine);
    for (int arg = 0; arg < ctx->callData->argc; ++arg)
        ctx->callData->args[arg].mark(engine);

    if (ctx->type >= Type_CallContext) {
        QV4::CallContext *c = static_cast<CallContext *>(ctx);
        for (unsigned local = 0, lastLocal = c->variableCount(); local < lastLocal; ++local)
            c->locals[local].mark(engine);
        if (c->activation)
            c->activation->mark(engine);
        c->function->mark(engine);
    } else if (ctx->type == Type_WithContext) {
        WithContext *w = static_cast<WithContext *>(ctx);
        w->withObject->mark(engine);
    } else if (ctx->type == Type_CatchContext) {
        CatchContext *c = static_cast<CatchContext *>(ctx);
        c->exceptionVarName->mark(engine);
        c->exceptionValue.mark(engine);
    } else if (ctx->type == Type_GlobalContext) {
        GlobalContext *g = static_cast<GlobalContext *>(ctx);
        g->global->mark(engine);
    }
}

void ExecutionContext::setProperty(const StringRef name, const ValueRef value)
{
    Scope scope(this);
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            ScopedObject w(scope, static_cast<WithContext *>(ctx)->withObject);
            if (w->__hasProperty__(name)) {
                w->put(name, value);
                return;
            }
        } else if (ctx->type == Type_CatchContext && static_cast<CatchContext *>(ctx)->exceptionVarName->isEqualTo(name)) {
            static_cast<CatchContext *>(ctx)->exceptionValue = *value;
            return;
        } else {
            ScopedObject activation(scope, (Object *)0);
            if (ctx->type >= Type_CallContext) {
                CallContext *c = static_cast<CallContext *>(ctx);
                for (unsigned int i = 0; i < c->function->varCount; ++i)
                    if (c->function->varList[i]->isEqualTo(name)) {
                        c->locals[i] = *value;
                        return;
                    }
                for (int i = (int)c->function->formalParameterCount - 1; i >= 0; --i)
                    if (c->function->formalParameterList[i]->isEqualTo(name)) {
                        c->callData->args[i] = *value;
                        return;
                    }
                activation = c->activation;
            } else if (ctx->type == Type_GlobalContext) {
                activation = static_cast<GlobalContext *>(ctx)->global;
            }

            if (activation && (ctx->type == Type_QmlContext || activation->__hasProperty__(name))) {
                activation->put(name, value);
                return;
            }
        }
    }
    if (strictMode || name->equals(engine->id_this)) {
        ScopedValue n(scope, name.asReturnedValue());
        throwReferenceError(n);
        return;
    }
    engine->globalObject->put(name, value);
}

ReturnedValue ExecutionContext::getProperty(const StringRef name)
{
    Scope scope(this);
    ScopedValue v(scope);
    name->makeIdentifier();

    if (name->equals(engine->id_this))
        return callData->thisObject.asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            ScopedObject w(scope, static_cast<WithContext *>(ctx)->withObject);
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                return v.asReturnedValue();
            }
            continue;
        }

        else if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue.asReturnedValue();
        }

        else if (ctx->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            ScopedFunctionObject f(scope, c->function);
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i].asReturnedValue();
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->callData->args[i].asReturnedValue();
            }
            if (c->activation) {
                bool hasProperty = false;
                v = c->activation->get(name, &hasProperty);
                if (hasProperty)
                    return v.asReturnedValue();
            }
            if (f->function && f->function->isNamedExpression()
                && name->equals(f->function->name))
                return f.asReturnedValue();
        }

        else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            v = g->global->get(name, &hasProperty);
            if (hasProperty)
                return v.asReturnedValue();
        }
    }
    ScopedValue n(scope, name.asReturnedValue());
    return throwReferenceError(n);
}

ReturnedValue ExecutionContext::getPropertyAndBase(const StringRef name, ObjectRef base)
{
    Scope scope(this);
    ScopedValue v(scope);
    base = (Object *)0;
    name->makeIdentifier();

    if (name->equals(engine->id_this))
        return callData->thisObject.asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->outer) {
        if (ctx->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->withObject;
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                base = w;
                return v.asReturnedValue();
            }
            continue;
        }

        else if (ctx->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue.asReturnedValue();
        }

        else if (ctx->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation || hasWith || hasCatchScope) {
                for (unsigned int i = 0; i < f->varCount; ++i)
                    if (f->varList[i]->isEqualTo(name))
                        return c->locals[i].asReturnedValue();
                for (int i = (int)f->formalParameterCount - 1; i >= 0; --i)
                    if (f->formalParameterList[i]->isEqualTo(name))
                        return c->callData->args[i].asReturnedValue();
            }
            if (c->activation) {
                bool hasProperty = false;
                v = c->activation->get(name, &hasProperty);
                if (hasProperty) {
                    if (ctx->type == Type_QmlContext)
                        base = c->activation;
                    return v.asReturnedValue();
                }
            }
            if (f->function && f->function->isNamedExpression()
                && name->equals(f->function->name))
                return c->function->asReturnedValue();
        }

        else if (ctx->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            v = g->global->get(name, &hasProperty);
            if (hasProperty)
                return v.asReturnedValue();
        }
    }
    ScopedValue n(scope, name.asReturnedValue());
    return throwReferenceError(n);
}


ReturnedValue ExecutionContext::throwError(const ValueRef value)
{
    return engine->throwException(value);
}

ReturnedValue ExecutionContext::throwError(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, engine->newString(message));
    v = engine->newErrorObject(v);
    return throwError(v);
}

ReturnedValue ExecutionContext::throwSyntaxError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    Scoped<Object> error(scope, engine->newSyntaxErrorObject(message, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwSyntaxError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, engine->newSyntaxErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwTypeError()
{
    Scope scope(this);
    Scoped<Object> error(scope, engine->newTypeErrorObject(QStringLiteral("Type error")));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwTypeError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, engine->newTypeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwUnimplemented(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, engine->newString(QStringLiteral("Unimplemented ") + message));
    v = engine->newErrorObject(v);
    return throwError(v);
}

ReturnedValue ExecutionContext::catchException(StackTrace *trace)
{
    return engine->catchException(this, trace);
}

ReturnedValue ExecutionContext::throwReferenceError(const ValueRef value)
{
    Scope scope(this);
    Scoped<String> s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    Scoped<Object> error(scope, engine->newReferenceErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwReferenceError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    QString msg = message;
    Scoped<Object> error(scope, engine->newReferenceErrorObject(msg, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwRangeError(const ValueRef value)
{
    Scope scope(this);
    ScopedString s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" out of range");
    ScopedObject error(scope, engine->newRangeErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwRangeError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, engine->newRangeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwURIError(const ValueRef msg)
{
    Scope scope(this);
    ScopedObject error(scope, engine->newURIErrorObject(msg));
    return throwError(error);
}
