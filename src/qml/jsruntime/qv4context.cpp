/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

DEFINE_MANAGED_VTABLE(ExecutionContext);
DEFINE_MANAGED_VTABLE(CallContext);
DEFINE_MANAGED_VTABLE(WithContext);
DEFINE_MANAGED_VTABLE(CatchContext);
DEFINE_MANAGED_VTABLE(GlobalContext);

Heap::CallContext *ExecutionContext::newCallContext(FunctionObject *function, CallData *callData)
{
    Q_ASSERT(function->function());

    Heap::CallContext *c = d()->engine->memoryManager->allocManaged<CallContext>(requiredMemoryForExecutionContect(function, callData->argc));
    new (c) Heap::CallContext(d()->engine, Heap::ExecutionContext::Type_CallContext);

    c->function = function->d();

    c->strictMode = function->strictMode();
    c->outer = function->scope();

    c->activation = 0;

    c->compilationUnit = function->function()->compilationUnit;
    c->lookups = c->compilationUnit->runtimeLookups;
    c->locals = (Value *)((quintptr(c + 1) + 7) & ~7);

    const CompiledData::Function *compiledFunction = function->function()->compiledFunction;
    int nLocals = compiledFunction->nLocals;
    if (nLocals)
        std::fill(c->locals, c->locals + nLocals, Primitive::undefinedValue());

    c->callData = reinterpret_cast<CallData *>(c->locals + nLocals);
    ::memcpy(c->callData, callData, sizeof(CallData) + (callData->argc - 1) * sizeof(Value));
    if (callData->argc < static_cast<int>(compiledFunction->nFormals))
        std::fill(c->callData->args + c->callData->argc, c->callData->args + compiledFunction->nFormals, Primitive::undefinedValue());

    return c;
}

Heap::WithContext *ExecutionContext::newWithContext(Object *with)
{
    return d()->engine->memoryManager->alloc<WithContext>(d()->engine, with);
}

Heap::CatchContext *ExecutionContext::newCatchContext(String *exceptionVarName, const Value &exceptionValue)
{
    return d()->engine->memoryManager->alloc<CatchContext>(d()->engine, exceptionVarName, exceptionValue);
}

Heap::CallContext *ExecutionContext::newQmlContext(FunctionObject *f, Object *qml)
{
    Scope scope(this);
    Scoped<CallContext> c(scope, d()->engine->memoryManager->allocManaged<CallContext>(requiredMemoryForExecutionContect(f, 0)));
    new (c->d()) Heap::CallContext(d()->engine, qml, f);
    return c->d();
}



void ExecutionContext::createMutableBinding(String *name, bool deletable)
{
    Scope scope(this);

    // find the right context to create the binding on
    ScopedObject activation(scope, d()->engine->globalObject());
    ScopedContext ctx(scope, this);
    while (ctx) {
        if (ctx->d()->type >= Heap::ExecutionContext::Type_CallContext) {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
            if (!c->activation)
                c->activation = scope.engine->newObject();
            activation = c->activation;
            break;
        }
        ctx = ctx->d()->outer;
    }

    if (activation->hasProperty(name))
        return;
    ScopedProperty desc(scope);
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    activation->__defineOwnProperty__(scope.engine, name, desc, attrs);
}


Heap::GlobalContext::GlobalContext(ExecutionEngine *eng)
    : Heap::ExecutionContext(eng, Heap::ExecutionContext::Type_GlobalContext)
{
    global = eng->globalObject()->d();
}

Heap::WithContext::WithContext(ExecutionEngine *engine, QV4::Object *with)
    : Heap::ExecutionContext(engine, Heap::ExecutionContext::Type_WithContext)
{
    callData = parent->callData;
    outer = parent;
    lookups = parent->lookups;
    compilationUnit = parent->compilationUnit;

    withObject = with ? with->d() : 0;
}

Heap::CatchContext::CatchContext(ExecutionEngine *engine, QV4::String *exceptionVarName, const Value &exceptionValue)
    : Heap::ExecutionContext(engine, Heap::ExecutionContext::Type_CatchContext)
{
    strictMode = parent->strictMode;
    callData = parent->callData;
    outer = parent;
    lookups = parent->lookups;
    compilationUnit = parent->compilationUnit;

    this->exceptionVarName = exceptionVarName;
    this->exceptionValue = exceptionValue;
}

Heap::CallContext::CallContext(ExecutionEngine *engine, QV4::Object *qml, QV4::FunctionObject *function)
    : Heap::ExecutionContext(engine, Heap::ExecutionContext::Type_QmlContext)
{
    this->function = function->d();
    callData = reinterpret_cast<CallData *>(this + 1);
    callData->tag = QV4::Value::_Integer_Type;
    callData->argc = 0;
    callData->thisObject = Primitive::undefinedValue();

    strictMode = false;
    outer = function->scope();

    activation = qml->d();

    if (function->function()) {
        compilationUnit = function->function()->compilationUnit;
        lookups = compilationUnit->runtimeLookups;
    }

    locals = (Value *)(this + 1);
    if (function->varCount())
        std::fill(locals, locals + function->varCount(), Primitive::undefinedValue());
}

Identifier * const *CallContext::formals() const
{
    return (d()->function && d()->function->function) ? d()->function->function->internalClass->nameMap.constData() : 0;
}

unsigned int CallContext::formalCount() const
{
    return d()->function ? d()->function->formalParameterCount() : 0;
}

Identifier * const *CallContext::variables() const
{
    return (d()->function && d()->function->function) ? d()->function->function->internalClass->nameMap.constData() + d()->function->formalParameterCount() : 0;
}

unsigned int CallContext::variableCount() const
{
    return d()->function ? d()->function->varCount() : 0;
}



bool ExecutionContext::deleteProperty(String *name)
{
    Scope scope(this);
    bool hasWith = false;
    ScopedContext ctx(scope, this);
    for (; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Heap::ExecutionContext::Type_WithContext) {
            hasWith = true;
            ScopedObject withObject(scope, static_cast<Heap::WithContext *>(ctx->d())->withObject);
            if (withObject->hasProperty(name))
                return withObject->deleteProperty(name);
        } else if (ctx->d()->type == Heap::ExecutionContext::Type_CatchContext) {
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx->d());
            if (c->exceptionVarName->isEqualTo(name))
                return false;
        } else if (ctx->d()->type >= Heap::ExecutionContext::Type_CallContext) {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
            ScopedFunctionObject f(scope, c->function);
            if (f->needsActivation() || hasWith) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX)
                    // ### throw in strict mode?
                    return false;
            }
            ScopedObject activation(scope, c->activation);
            if (activation && activation->hasProperty(name))
                return activation->deleteProperty(name);
        } else if (ctx->d()->type == Heap::ExecutionContext::Type_GlobalContext) {
            ScopedObject global(scope, static_cast<Heap::GlobalContext *>(ctx->d())->global);
            if (global->hasProperty(name))
                return global->deleteProperty(name);
        }
    }

    if (d()->strictMode)
        engine()->throwSyntaxError(QStringLiteral("Can't delete property %1").arg(name->toQString()));
    return true;
}

bool CallContext::needsOwnArguments() const
{
    return d()->function->needsActivation() || argc() < static_cast<int>(d()->function->formalParameterCount());
}

void ExecutionContext::markObjects(Heap::Base *m, ExecutionEngine *engine)
{
    ExecutionContext::Data *ctx = static_cast<ExecutionContext::Data *>(m);

    if (ctx->outer)
        ctx->outer->mark(engine);

    if (ctx->type >= Heap::ExecutionContext::Type_CallContext) {
        QV4::Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
        ctx->callData->thisObject.mark(engine);
        for (int arg = 0; arg < qMax(ctx->callData->argc, (int)c->function->formalParameterCount()); ++arg)
            ctx->callData->args[arg].mark(engine);
        for (unsigned local = 0, lastLocal = c->function->varCount(); local < lastLocal; ++local)
            c->locals[local].mark(engine);
        if (c->activation)
            c->activation->mark(engine);
        c->function->mark(engine);
    } else if (ctx->type == Heap::ExecutionContext::Type_WithContext) {
        WithContext::Data *w = static_cast<WithContext::Data *>(ctx);
        if (w->withObject)
            w->withObject->mark(engine);
    } else if (ctx->type == Heap::ExecutionContext::Type_CatchContext) {
        CatchContext::Data *c = static_cast<CatchContext::Data *>(ctx);
        c->exceptionVarName->mark(engine);
        c->exceptionValue.mark(engine);
    } else if (ctx->type == Heap::ExecutionContext::Type_GlobalContext) {
        GlobalContext::Data *g = static_cast<GlobalContext::Data *>(ctx);
        g->global->mark(engine);
    }
}

void ExecutionContext::setProperty(String *name, const Value &value)
{
    Scope scope(this);
    ScopedContext ctx(scope, this);
    for (; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Heap::ExecutionContext::Type_WithContext) {
            ScopedObject w(scope, static_cast<Heap::WithContext *>(ctx->d())->withObject);
            if (w->hasProperty(name)) {
                w->put(name, value);
                return;
            }
        } else if (ctx->d()->type == Heap::ExecutionContext::Type_CatchContext && static_cast<Heap::CatchContext *>(ctx->d())->exceptionVarName->isEqualTo(name)) {
            static_cast<Heap::CatchContext *>(ctx->d())->exceptionValue = value;
            return;
        } else {
            ScopedObject activation(scope, (Object *)0);
            if (ctx->d()->type >= Heap::ExecutionContext::Type_CallContext) {
                Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
                if (c->function->function) {
                    uint index = c->function->function->internalClass->find(name);
                    if (index < UINT_MAX) {
                        if (index < c->function->formalParameterCount()) {
                            c->callData->args[c->function->formalParameterCount() - index - 1] = value;
                        } else {
                            index -= c->function->formalParameterCount();
                            c->locals[index] = value;
                        }
                        return;
                    }
                }
                activation = c->activation;
            } else if (ctx->d()->type == Heap::ExecutionContext::Type_GlobalContext) {
                activation = static_cast<Heap::GlobalContext *>(ctx->d())->global;
            }

            if (activation) {
                if (ctx->d()->type == Heap::ExecutionContext::Type_QmlContext) {
                    activation->put(name, value);
                    return;
                } else {
                    uint member = activation->internalClass()->find(name);
                    if (member < UINT_MAX) {
                        activation->putValue(activation->propertyAt(member), activation->internalClass()->propertyData[member], value);
                        return;
                    }
                }
            }
        }
    }
    if (d()->strictMode || name->equals(d()->engine->id_this)) {
        ScopedValue n(scope, name->asReturnedValue());
        engine()->throwReferenceError(n);
        return;
    }
    d()->engine->globalObject()->put(name, value);
}

ReturnedValue ExecutionContext::getProperty(String *name)
{
    Scope scope(this);
    ScopedValue v(scope);
    name->makeIdentifier(scope.engine);

    if (name->equals(d()->engine->id_this))
        return thisObject().asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    ScopedContext ctx(scope, this);
    for (; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Heap::ExecutionContext::Type_WithContext) {
            ScopedObject w(scope, static_cast<Heap::WithContext *>(ctx->d())->withObject);
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                return v->asReturnedValue();
            }
            continue;
        }

        else if (ctx->d()->type == Heap::ExecutionContext::Type_CatchContext) {
            hasCatchScope = true;
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx->d());
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue.asReturnedValue();
        }

        else if (ctx->d()->type >= Heap::ExecutionContext::Type_CallContext) {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
            ScopedFunctionObject f(scope, c->function);
            if (f->function() && (f->needsActivation() || hasWith || hasCatchScope)) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX) {
                    if (index < c->function->formalParameterCount())
                        return c->callData->args[c->function->formalParameterCount() - index - 1].asReturnedValue();
                    return c->locals[index - c->function->formalParameterCount()].asReturnedValue();
                }
            }
            ScopedObject activation(scope, c->activation);
            if (activation) {
                bool hasProperty = false;
                v = activation->get(name, &hasProperty);
                if (hasProperty)
                    return v->asReturnedValue();
            }
            if (f->function() && f->function()->isNamedExpression()
                && name->equals(ScopedString(scope, f->function()->name())))
                return f.asReturnedValue();
        }

        else if (ctx->d()->type == Heap::ExecutionContext::Type_GlobalContext) {
            ScopedObject global(scope, static_cast<Heap::GlobalContext *>(ctx->d())->global);
            bool hasProperty = false;
            v = global->get(name, &hasProperty);
            if (hasProperty)
                return v->asReturnedValue();
        }
    }
    ScopedValue n(scope, name);
    return engine()->throwReferenceError(n);
}

ReturnedValue ExecutionContext::getPropertyAndBase(String *name, Heap::Object **base)
{
    Scope scope(this);
    ScopedValue v(scope);
    *base = (Heap::Object *)0;
    name->makeIdentifier(scope.engine);

    if (name->equals(d()->engine->id_this))
        return thisObject().asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    ScopedContext ctx(scope, this);
    for (; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Heap::ExecutionContext::Type_WithContext) {
            ScopedObject w(scope, static_cast<Heap::WithContext *>(ctx->d())->withObject);
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                *base = w->d();
                return v->asReturnedValue();
            }
            continue;
        }

        else if (ctx->d()->type == Heap::ExecutionContext::Type_CatchContext) {
            hasCatchScope = true;
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx->d());
            if (c->exceptionVarName->isEqualTo(name))
                return c->exceptionValue.asReturnedValue();
        }

        else if (ctx->d()->type >= Heap::ExecutionContext::Type_CallContext) {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
            ScopedFunctionObject f(scope, c->function);
            if (f->function() && (f->needsActivation() || hasWith || hasCatchScope)) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX) {
                    if (index < f->formalParameterCount())
                        return c->callData->args[f->formalParameterCount() - index - 1].asReturnedValue();
                    return c->locals[index - f->formalParameterCount()].asReturnedValue();
                }
            }
            ScopedObject activation(scope, c->activation);
            if (activation) {
                bool hasProperty = false;
                v = activation->get(name, &hasProperty);
                if (hasProperty) {
                    if (ctx->d()->type == Heap::ExecutionContext::Type_QmlContext)
                        *base = activation->d();
                    return v->asReturnedValue();
                }
            }
            if (f->function() && f->function()->isNamedExpression()
                && name->equals(ScopedString(scope, f->function()->name())))
                return c->function->asReturnedValue();
        }

        else if (ctx->d()->type == Heap::ExecutionContext::Type_GlobalContext) {
            ScopedObject global(scope, static_cast<Heap::GlobalContext *>(ctx->d())->global);
            bool hasProperty = false;
            v = global->get(name, &hasProperty);
            if (hasProperty)
                return v->asReturnedValue();
        }
    }
    ScopedValue n(scope, name);
    return engine()->throwReferenceError(n);
}

Heap::FunctionObject *ExecutionContext::getFunctionObject() const
{
    Scope scope(d()->engine);
    ScopedContext it(scope, this->d());
    for (; it; it = it->d()->parent) {
        if (const CallContext *callCtx = it->asCallContext())
            return callCtx->d()->function;
        else if (it->asCatchContext() || it->asWithContext())
            continue; // look in the parent context for a FunctionObject
        else
            break;
    }

    return 0;
}
