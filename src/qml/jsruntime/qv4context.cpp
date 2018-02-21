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

#include <QString>
#include "qv4debugging_p.h"
#include <qv4context_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include <private/qv4mm_p.h>
#include <qv4argumentsobject_p.h>
#include "qv4function_p.h"
#include "qv4errorobject_p.h"
#include "qv4string_p.h"
#include "qv4qmlcontext_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(ExecutionContext);
DEFINE_MANAGED_VTABLE(CallContext);
DEFINE_MANAGED_VTABLE(CatchContext);

Heap::CallContext *ExecutionContext::newCallContext(CppStackFrame *frame)
{
    Function *function = frame->v4Function;
    Heap::ExecutionContext *outer = static_cast<Heap::ExecutionContext *>(frame->context()->m());

    uint nFormals = qMax(static_cast<uint>(frame->originalArgumentsCount), function->nFormals);
    uint localsAndFormals = function->compiledFunction->nLocals + nFormals;
    size_t requiredMemory = sizeof(CallContext::Data) - sizeof(Value) + sizeof(Value) * (localsAndFormals);

    ExecutionEngine *v4 = outer->internalClass->engine;
    Heap::CallContext *c = v4->memoryManager->allocManaged<CallContext>(requiredMemory, function->internalClass);
    c->init();

    c->outer.set(v4, outer);
    c->function.set(v4, static_cast<Heap::FunctionObject *>(frame->jsFrame->function.m()));

    const CompiledData::Function *compiledFunction = function->compiledFunction;
    uint nLocals = compiledFunction->nLocals;
    c->locals.size = nLocals;
    c->locals.alloc = localsAndFormals;
    // memory allocated from the JS heap is 0 initialized, so check if undefined is 0
    Q_ASSERT(Primitive::undefinedValue().asReturnedValue() == 0);

    Value *args = c->locals.values + nLocals;
    ::memcpy(args, frame->originalArguments, frame->originalArgumentsCount * sizeof(Value));
    c->nArgs = frame->originalArgumentsCount;
    for (uint i = frame->originalArgumentsCount; i < function->nFormals; ++i)
        args[i] = Encode::undefined();

    return c;
}

Heap::ExecutionContext *ExecutionContext::newWithContext(Heap::Object *with)
{
    Heap::ExecutionContext *c = engine()->memoryManager->alloc<ExecutionContext>(Heap::ExecutionContext::Type_WithContext);
    c->outer.set(engine(), d());
    c->activation.set(engine(), with);

    return c;
}

Heap::CatchContext *ExecutionContext::newCatchContext(Heap::String *exceptionVarName, ReturnedValue exceptionValue)
{
    Scope scope(this);
    ScopedValue e(scope, exceptionValue);
    return engine()->memoryManager->alloc<CatchContext>(d(), exceptionVarName, e);
}

void ExecutionContext::createMutableBinding(String *name, bool deletable)
{
    Scope scope(this);

    // find the right context to create the binding on
    ScopedObject activation(scope);
    ScopedContext ctx(scope, this);
    while (ctx) {
        switch (ctx->d()->type) {
        case Heap::ExecutionContext::Type_CallContext:
            if (!activation) {
                Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx->d());
                if (!c->activation)
                    c->activation.set(scope.engine, scope.engine->newObject());
                activation = c->activation;
            }
            break;
        case Heap::ExecutionContext::Type_QmlContext: {
            // this is ugly, as it overrides the inner callcontext, but has to stay as long
            // as bindings still get their own callcontext
            activation = ctx->d()->activation;
            break;
        }
        case Heap::ExecutionContext::Type_GlobalContext: {
            Q_ASSERT(scope.engine->globalObject->d() == ctx->d()->activation);
            if (!activation)
                activation = ctx->d()->activation;
            break;
        }
        default:
            break;
        }
        ctx = ctx->d()->outer;
    }

    if (activation->hasOwnProperty(name))
        return;
    ScopedProperty desc(scope);
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    activation->__defineOwnProperty__(scope.engine, name, desc, attrs);
}

void Heap::CatchContext::init(ExecutionContext *outerContext, String *exceptionVarName,
                              const Value &exceptionValue)
{
    Heap::ExecutionContext::init(Heap::ExecutionContext::Type_CatchContext);
    outer.set(internalClass->engine, outerContext);

    this->exceptionVarName.set(internalClass->engine, exceptionVarName);
    this->exceptionValue.set(internalClass->engine, exceptionValue);
}

bool ExecutionContext::deleteProperty(String *name)
{
    name->makeIdentifier();
    Identifier *id = name->identifier();

    Heap::ExecutionContext *ctx = d();
    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_CatchContext: {
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name->d()))
                return false;
            break;
        }
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            uint index = c->internalClass->find(id);
            if (index < UINT_MAX)
                // ### throw in strict mode?
                return false;
            Q_FALLTHROUGH();
        }
        case Heap::ExecutionContext::Type_WithContext:
        case Heap::ExecutionContext::Type_GlobalContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject object(scope, ctx->activation);
                if (object && object->hasProperty(name))
                    return object->deleteProperty(name);
            }
            break;
        }
        case Heap::ExecutionContext::Type_QmlContext:
            // can't delete properties on qml objects
            break;
        }
    }

    return !engine()->currentStackFrame->v4Function->isStrict();
}

ExecutionContext::Error ExecutionContext::setProperty(String *name, const Value &value)
{
    name->makeIdentifier();
    Identifier *id = name->identifier();

    QV4::ExecutionEngine *v4 = engine();
    Heap::ExecutionContext *ctx = d();

    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_CatchContext: {
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name->d())) {
                    c->exceptionValue.set(v4, value);
                    return NoError;
            }
            break;
        }
        case Heap::ExecutionContext::Type_WithContext: {
            Scope scope(v4);
            ScopedObject w(scope, ctx->activation);
            if (w->hasProperty(name)) {
                if (!w->put(name, value))
                    return TypeError;
                return NoError;
            }
            break;
        }
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            uint index = c->internalClass->find(id);
            if (index < UINT_MAX) {
                static_cast<Heap::CallContext *>(c)->locals.set(v4, index, value);
                return NoError;
            }
        }
            Q_FALLTHROUGH();
        case Heap::ExecutionContext::Type_GlobalContext:
            if (ctx->activation) {
                uint member = ctx->activation->internalClass->find(id);
                if (member < UINT_MAX) {
                    Scope scope(v4);
                    ScopedObject a(scope, ctx->activation);
                    if (!a->putValue(member, value))
                        return TypeError;
                    return NoError;
                }
            }
            break;
        case Heap::ExecutionContext::Type_QmlContext: {
            Scope scope(v4);
            ScopedObject activation(scope, ctx->activation);
            if (!activation->put(name, value))
                return TypeError;
            return NoError;
        }
        }

    }

    return RangeError;
}

ReturnedValue ExecutionContext::getProperty(String *name)
{
    name->makeIdentifier();

    Heap::ExecutionContext *ctx = d();
    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_CatchContext: {
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name->d()))
                return c->exceptionValue.asReturnedValue();
            break;
        }
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            Identifier *id = name->identifier();

            uint index = c->internalClass->find(id);
            if (index < UINT_MAX)
                return c->locals[index].asReturnedValue();
            Q_FALLTHROUGH();
        }
        case Heap::ExecutionContext::Type_WithContext:
        case Heap::ExecutionContext::Type_GlobalContext:
        case Heap::ExecutionContext::Type_QmlContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject activation(scope, ctx->activation);
                bool hasProperty = false;
                ReturnedValue v = activation->get(name, &hasProperty);
                if (hasProperty)
                    return v;
            }
            break;
        }
        }
    }
    return engine()->throwReferenceError(*name);
}

ReturnedValue ExecutionContext::getPropertyAndBase(String *name, Value *base)
{
    base->setM(nullptr);
    name->makeIdentifier();

    Heap::ExecutionContext *ctx = d();
    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_CatchContext: {
            Heap::CatchContext *c = static_cast<Heap::CatchContext *>(ctx);
            if (c->exceptionVarName->isEqualTo(name->d()))
                return c->exceptionValue.asReturnedValue();
            break;
        }
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            name->makeIdentifier();
            Identifier *id = name->identifier();

            uint index = c->internalClass->find(id);
            if (index < UINT_MAX)
                return c->locals[index].asReturnedValue();
            Q_FALLTHROUGH();
        }
        case Heap::ExecutionContext::Type_GlobalContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject activation(scope, ctx->activation);
                bool hasProperty = false;
                ReturnedValue v = activation->get(name, &hasProperty);
                if (hasProperty)
                    return v;
            }
            break;
        }
        case Heap::ExecutionContext::Type_WithContext:
        case Heap::ExecutionContext::Type_QmlContext: {
            Scope scope(this);
            ScopedObject o(scope, ctx->activation);
            bool hasProperty = false;
            ReturnedValue v = o->get(name, &hasProperty);
            if (hasProperty) {
                base->setM(o->d());
                return v;
            }
            break;
        }
        }
    }
    return engine()->throwReferenceError(*name);
}

void Heap::CallContext::setArg(uint index, Value v)
{
    locals.set(internalClass->engine, locals.size + index, v);
}
