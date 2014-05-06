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

DEFINE_MANAGED_VTABLE(ExecutionContext);

CallContext *ExecutionContext::newCallContext(FunctionObject *function, CallData *callData)
{
    Q_ASSERT(function->function());

    CallContext *c = static_cast<CallContext *>(d()->engine->memoryManager->allocManaged(requiredMemoryForExecutionContect(function, callData->argc)));
    new (c) CallContext(d()->engine, Type_CallContext);

    c->function = function;
    c->realArgumentCount = callData->argc;

    c->d()->strictMode = function->strictMode();
    c->d()->outer = function->scope();

    c->activation = 0;

    c->d()->compilationUnit = function->function()->compilationUnit;
    c->d()->lookups = c->d()->compilationUnit->runtimeLookups;
    c->locals = (Value *)((quintptr(c + 1) + 7) & ~7);

    const CompiledData::Function *compiledFunction = function->function()->compiledFunction;
    int nLocals = compiledFunction->nLocals;
    if (nLocals)
        std::fill(c->locals, c->locals + nLocals, Primitive::undefinedValue());

    c->d()->callData = reinterpret_cast<CallData *>(c->locals + nLocals);
    ::memcpy(c->d()->callData, callData, sizeof(CallData) + (callData->argc - 1) * sizeof(Value));
    if (callData->argc < static_cast<int>(compiledFunction->nFormals))
        std::fill(c->d()->callData->args + c->d()->callData->argc, c->d()->callData->args + compiledFunction->nFormals, Primitive::undefinedValue());
    c->d()->callData->argc = qMax((uint)callData->argc, compiledFunction->nFormals);

    return c;
}

WithContext *ExecutionContext::newWithContext(ObjectRef with)
{
    WithContext *w = new (d()->engine->memoryManager) WithContext(d()->engine, with);
    return w;
}

CatchContext *ExecutionContext::newCatchContext(const StringRef exceptionVarName, const ValueRef exceptionValue)
{
    CatchContext *c = new (d()->engine->memoryManager) CatchContext(d()->engine, exceptionVarName, exceptionValue);
    return c;
}

CallContext *ExecutionContext::newQmlContext(FunctionObject *f, ObjectRef qml)
{
    CallContext *c = static_cast<CallContext *>(d()->engine->memoryManager->allocManaged(requiredMemoryForExecutionContect(f, 0)));
    new (c) CallContext(d()->engine, qml, f);
    return c;
}



void ExecutionContext::createMutableBinding(const StringRef name, bool deletable)
{
    Scope scope(this);

    // find the right context to create the binding on
    ScopedObject activation(scope, d()->engine->globalObject);
    ExecutionContext *ctx = this;
    while (ctx) {
        if (ctx->d()->type >= Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            if (!c->activation)
                c->activation = d()->engine->newObject()->getPointer();
            activation = c->activation;
            break;
        }
        ctx = ctx->d()->outer;
    }

    if (activation->hasProperty(name))
        return;
    Property desc(Primitive::undefinedValue());
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    activation->__defineOwnProperty__(this, name, desc, attrs);
}


GlobalContext::GlobalContext(ExecutionEngine *eng)
    : ExecutionContext(eng, Type_GlobalContext)
{
    d()->global = eng->globalObject;
}

WithContext::WithContext(ExecutionEngine *engine, ObjectRef with)
    : ExecutionContext(engine, Type_WithContext)
{
    d()->callData = d()->parent->d()->callData;
    d()->outer = d()->parent;
    d()->lookups = d()->parent->d()->lookups;
    d()->compilationUnit = d()->parent->d()->compilationUnit;

    d()->withObject = with.getPointer();
}

CatchContext::CatchContext(ExecutionEngine *engine, const StringRef exceptionVarName, const ValueRef exceptionValue)
    : ExecutionContext(engine, Type_CatchContext)
{
    d()->strictMode = d()->parent->d()->strictMode;
    d()->callData = d()->parent->d()->callData;
    d()->outer = d()->parent;
    d()->lookups = d()->parent->d()->lookups;
    d()->compilationUnit = d()->parent->d()->compilationUnit;

    this->d()->exceptionVarName = exceptionVarName;
    this->d()->exceptionValue = exceptionValue;
}

CallContext::CallContext(ExecutionEngine *engine, ObjectRef qml, FunctionObject *function)
    : ExecutionContext(engine, Type_QmlContext)
{
    this->function = function;
    d()->callData = reinterpret_cast<CallData *>(this + 1);
    d()->callData->tag = QV4::Value::_Integer_Type;
    d()->callData->argc = 0;
    d()->callData->thisObject = Primitive::undefinedValue();

    d()->strictMode = true;
    d()->outer = function->scope();

    activation = qml.getPointer();

    if (function->function()) {
        d()->compilationUnit = function->function()->compilationUnit;
        d()->lookups = d()->compilationUnit->runtimeLookups;
    }

    locals = (Value *)(this + 1);
    if (function->varCount())
        std::fill(locals, locals + function->varCount(), Primitive::undefinedValue());
}

String * const *CallContext::formals() const
{
    return (function && function->function()) ? function->function()->internalClass->nameMap.constData() : 0;
}

unsigned int CallContext::formalCount() const
{
    return function ? function->formalParameterCount() : 0;
}

String * const *CallContext::variables() const
{
    return (function && function->function()) ? function->function()->internalClass->nameMap.constData() + function->function()->compiledFunction->nFormals : 0;
}

unsigned int CallContext::variableCount() const
{
    return function ? function->varCount() : 0;
}



bool ExecutionContext::deleteProperty(const StringRef name)
{
    Scope scope(this);
    bool hasWith = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Type_WithContext) {
            hasWith = true;
            WithContext *w = static_cast<WithContext *>(ctx);
            if (w->d()->withObject->hasProperty(name))
                return w->d()->withObject->deleteProperty(name);
        } else if (ctx->d()->type == Type_CatchContext) {
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->d()->exceptionVarName->isEqualTo(name))
                return false;
        } else if (ctx->d()->type >= Type_CallContext) {
            CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->needsActivation() || hasWith) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX)
                    // ### throw in strict mode?
                    return false;
            }
            if (c->activation && c->activation->hasProperty(name))
                return c->activation->deleteProperty(name);
        } else if (ctx->d()->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            if (g->d()->global->hasProperty(name))
                return g->d()->global->deleteProperty(name);
        }
    }

    if (d()->strictMode)
        throwSyntaxError(QStringLiteral("Can't delete property %1").arg(name->toQString()));
    return true;
}

bool CallContext::needsOwnArguments() const
{
    return function->needsActivation() || d()->callData->argc < static_cast<int>(function->formalParameterCount());
}

void ExecutionContext::markObjects(Managed *m, ExecutionEngine *engine)
{
    ExecutionContext *ctx = static_cast<ExecutionContext *>(m);

    if (ctx->d()->outer)
        ctx->d()->outer->mark(engine);

    // ### shouldn't need these 3 lines
    ctx->d()->callData->thisObject.mark(engine);
    for (int arg = 0; arg < ctx->d()->callData->argc; ++arg)
        ctx->d()->callData->args[arg].mark(engine);

    if (ctx->d()->type >= Type_CallContext) {
        QV4::CallContext *c = static_cast<CallContext *>(ctx);
        for (unsigned local = 0, lastLocal = c->function->varCount(); local < lastLocal; ++local)
            c->locals[local].mark(engine);
        if (c->activation)
            c->activation->mark(engine);
        c->function->mark(engine);
    } else if (ctx->d()->type == Type_WithContext) {
        WithContext *w = static_cast<WithContext *>(ctx);
        w->d()->withObject->mark(engine);
    } else if (ctx->d()->type == Type_CatchContext) {
        CatchContext *c = static_cast<CatchContext *>(ctx);
        c->d()->exceptionVarName->mark(engine);
        c->d()->exceptionValue.mark(engine);
    } else if (ctx->d()->type == Type_GlobalContext) {
        GlobalContext *g = static_cast<GlobalContext *>(ctx);
        g->d()->global->mark(engine);
    }
}

void ExecutionContext::setProperty(const StringRef name, const ValueRef value)
{
    Scope scope(this);
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Type_WithContext) {
            ScopedObject w(scope, static_cast<WithContext *>(ctx)->d()->withObject);
            if (w->hasProperty(name)) {
                w->put(name, value);
                return;
            }
        } else if (ctx->d()->type == Type_CatchContext && static_cast<CatchContext *>(ctx)->d()->exceptionVarName->isEqualTo(name)) {
            static_cast<CatchContext *>(ctx)->d()->exceptionValue = *value;
            return;
        } else {
            ScopedObject activation(scope, (Object *)0);
            if (ctx->d()->type >= Type_CallContext) {
                CallContext *c = static_cast<CallContext *>(ctx);
                if (c->function->function()) {
                    uint index = c->function->function()->internalClass->find(name);
                    if (index < UINT_MAX) {
                        if (index < c->function->formalParameterCount()) {
                            c->d()->callData->args[c->function->formalParameterCount() - index - 1] = *value;
                        } else {
                            index -= c->function->formalParameterCount();
                            c->locals[index] = *value;
                        }
                        return;
                    }
                }
                activation = c->activation;
            } else if (ctx->d()->type == Type_GlobalContext) {
                activation = static_cast<GlobalContext *>(ctx)->d()->global;
            }

            if (activation) {
                if (ctx->d()->type == Type_QmlContext) {
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
        ScopedValue n(scope, name.asReturnedValue());
        throwReferenceError(n);
        return;
    }
    d()->engine->globalObject->put(name, value);
}

ReturnedValue ExecutionContext::getProperty(const StringRef name)
{
    Scope scope(this);
    ScopedValue v(scope);
    name->makeIdentifier();

    if (name->equals(d()->engine->id_this))
        return d()->callData->thisObject.asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Type_WithContext) {
            ScopedObject w(scope, static_cast<WithContext *>(ctx)->d()->withObject);
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                return v.asReturnedValue();
            }
            continue;
        }

        else if (ctx->d()->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->d()->exceptionVarName->isEqualTo(name))
                return c->d()->exceptionValue.asReturnedValue();
        }

        else if (ctx->d()->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            ScopedFunctionObject f(scope, c->function);
            if (f->function() && (f->needsActivation() || hasWith || hasCatchScope)) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX) {
                    if (index < c->function->formalParameterCount())
                        return c->d()->callData->args[c->function->formalParameterCount() - index - 1].asReturnedValue();
                    return c->locals[index - c->function->formalParameterCount()].asReturnedValue();
                }
            }
            if (c->activation) {
                bool hasProperty = false;
                v = c->activation->get(name, &hasProperty);
                if (hasProperty)
                    return v.asReturnedValue();
            }
            if (f->function() && f->function()->isNamedExpression()
                && name->equals(f->function()->name()))
                return f.asReturnedValue();
        }

        else if (ctx->d()->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            v = g->d()->global->get(name, &hasProperty);
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

    if (name->equals(d()->engine->id_this))
        return d()->callData->thisObject.asReturnedValue();

    bool hasWith = false;
    bool hasCatchScope = false;
    for (ExecutionContext *ctx = this; ctx; ctx = ctx->d()->outer) {
        if (ctx->d()->type == Type_WithContext) {
            Object *w = static_cast<WithContext *>(ctx)->d()->withObject;
            hasWith = true;
            bool hasProperty = false;
            v = w->get(name, &hasProperty);
            if (hasProperty) {
                base = w;
                return v.asReturnedValue();
            }
            continue;
        }

        else if (ctx->d()->type == Type_CatchContext) {
            hasCatchScope = true;
            CatchContext *c = static_cast<CatchContext *>(ctx);
            if (c->d()->exceptionVarName->isEqualTo(name))
                return c->d()->exceptionValue.asReturnedValue();
        }

        else if (ctx->d()->type >= Type_CallContext) {
            QV4::CallContext *c = static_cast<CallContext *>(ctx);
            FunctionObject *f = c->function;
            if (f->function() && (f->needsActivation() || hasWith || hasCatchScope)) {
                uint index = f->function()->internalClass->find(name);
                if (index < UINT_MAX) {
                    if (index < c->function->formalParameterCount())
                        return c->d()->callData->args[c->function->formalParameterCount() - index - 1].asReturnedValue();
                    return c->locals[index - c->function->formalParameterCount()].asReturnedValue();
                }
            }
            if (c->activation) {
                bool hasProperty = false;
                v = c->activation->get(name, &hasProperty);
                if (hasProperty) {
                    if (ctx->d()->type == Type_QmlContext)
                        base = c->activation;
                    return v.asReturnedValue();
                }
            }
            if (f->function() && f->function()->isNamedExpression()
                && name->equals(f->function()->name()))
                return c->function->asReturnedValue();
        }

        else if (ctx->d()->type == Type_GlobalContext) {
            GlobalContext *g = static_cast<GlobalContext *>(ctx);
            bool hasProperty = false;
            v = g->d()->global->get(name, &hasProperty);
            if (hasProperty)
                return v.asReturnedValue();
        }
    }
    ScopedValue n(scope, name.asReturnedValue());
    return throwReferenceError(n);
}


ReturnedValue ExecutionContext::throwError(const ValueRef value)
{
    return d()->engine->throwException(value);
}

ReturnedValue ExecutionContext::throwError(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, d()->engine->newString(message));
    v = d()->engine->newErrorObject(v);
    return throwError(v);
}

ReturnedValue ExecutionContext::throwSyntaxError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    Scoped<Object> error(scope, d()->engine->newSyntaxErrorObject(message, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwSyntaxError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, d()->engine->newSyntaxErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwTypeError()
{
    Scope scope(this);
    Scoped<Object> error(scope, d()->engine->newTypeErrorObject(QStringLiteral("Type error")));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwTypeError(const QString &message)
{
    Scope scope(this);
    Scoped<Object> error(scope, d()->engine->newTypeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwUnimplemented(const QString &message)
{
    Scope scope(this);
    ScopedValue v(scope, d()->engine->newString(QStringLiteral("Unimplemented ") + message));
    v = d()->engine->newErrorObject(v);
    return throwError(v);
}

ReturnedValue ExecutionContext::catchException(StackTrace *trace)
{
    return d()->engine->catchException(this, trace);
}

ReturnedValue ExecutionContext::throwReferenceError(const ValueRef value)
{
    Scope scope(this);
    Scoped<String> s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    Scoped<Object> error(scope, d()->engine->newReferenceErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwReferenceError(const QString &message, const QString &fileName, int line, int column)
{
    Scope scope(this);
    QString msg = message;
    Scoped<Object> error(scope, d()->engine->newReferenceErrorObject(msg, fileName, line, column));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwRangeError(const ValueRef value)
{
    Scope scope(this);
    ScopedString s(scope, value->toString(this));
    QString msg = s->toQString() + QStringLiteral(" out of range");
    ScopedObject error(scope, d()->engine->newRangeErrorObject(msg));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwRangeError(const QString &message)
{
    Scope scope(this);
    ScopedObject error(scope, d()->engine->newRangeErrorObject(message));
    return throwError(error);
}

ReturnedValue ExecutionContext::throwURIError(const ValueRef msg)
{
    Scope scope(this);
    ScopedObject error(scope, d()->engine->newURIErrorObject(msg));
    return throwError(error);
}
