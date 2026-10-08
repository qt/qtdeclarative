// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <QString>
#include <qv4context_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include <private/qv4mm_p.h>
#include <qv4argumentsobject_p.h>
#include "qv4function_p.h"
#include "qv4stackframe_p.h"
#include "qv4symbol_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(ExecutionContext);
DEFINE_MANAGED_VTABLE(CallContext);

Heap::CallContext *ExecutionContext::newBlockContext(CppStackFrame *frame, int blockIndex)
{
    Function *function = frame->v4Function;

    Heap::InternalClass *ic = function->executableCompilationUnit()->runtimeBlocks.at(blockIndex);
    uint nLocals = ic->size;
    size_t requiredMemory = sizeof(CallContext::Data) - sizeof(Value) + sizeof(Value) * nLocals;

    ExecutionEngine *v4 = function->internalClass->engine;
    Heap::CallContext *c = v4->memoryManager->allocManaged<CallContext>(requiredMemory, ic);
    c->init();
    c->type = Heap::ExecutionContext::Type_BlockContext;

    Heap::ExecutionContext *outer = static_cast<Heap::ExecutionContext *>(frame->context()->m());
    c->outer.set(v4, outer);
    if (frame->isJSTypesFrame()) {
        c->function.set(v4, static_cast<Heap::JavaScriptFunctionObject *>(
                            Value::fromStaticValue(
                                static_cast<JSTypesStackFrame *>(frame)->jsFrame->function).m()));
    } else {
        c->function.set(v4, nullptr);
    }

    c->locals.size = nLocals;
    c->locals.alloc = nLocals;

    c->setupLocalTemporalDeadZone(function->executableCompilationUnit()->unitData()->blockAt(blockIndex));

    return c;
}

Heap::CallContext *ExecutionContext::cloneBlockContext(ExecutionEngine *engine,
                                                       Heap::CallContext *callContext)
{
    uint nLocals = callContext->locals.alloc;
    size_t requiredMemory = sizeof(CallContext::Data) - sizeof(Value) + sizeof(Value) * nLocals;

    Heap::CallContext *c = engine->memoryManager->allocManaged<CallContext>(
                requiredMemory, callContext->internalClass);
    memcpy(c, callContext, requiredMemory);

    return c;
}

Heap::CallContext *ExecutionContext::newCallContext(JSTypesStackFrame *frame)
{
    Function *function = frame->v4Function;
    Heap::ExecutionContext *outer = static_cast<Heap::ExecutionContext *>(frame->context()->m());

    uint nFormals = qMax(static_cast<uint>(frame->argc()), function->nFormals);
    uint localsAndFormals = function->compiledFunction->nLocals + nFormals;
    size_t requiredMemory = sizeof(CallContext::Data) - sizeof(Value) + sizeof(Value) * (localsAndFormals);

    ExecutionEngine *v4 = outer->internalClass->engine;
    Heap::CallContext *c = v4->memoryManager->allocManaged<CallContext>(requiredMemory, function->internalClass);
    c->init();

    c->outer.set(v4, outer);
    c->function.set(v4, static_cast<Heap::JavaScriptFunctionObject *>(
                                Value::fromStaticValue(frame->jsFrame->function).m()));

    const CompiledData::Function *compiledFunction = function->compiledFunction;
    uint nLocals = compiledFunction->nLocals;
    c->locals.size = nLocals;
    c->locals.alloc = localsAndFormals;
    // memory allocated from the JS heap is 0 initialized, so check if empty is 0
    Q_ASSERT(Value::undefinedValue().asReturnedValue() == 0);

    c->setupLocalTemporalDeadZone(compiledFunction);

    Value *args = c->locals.values + nLocals;
    ::memcpy(args, frame->argv(), frame->argc() * sizeof(Value));
    c->nArgs = frame->argc();
    for (uint i = frame->argc(); i < function->nFormals; ++i)
        args[i] = Encode::undefined();

    return c;
}

Heap::ExecutionContext *ExecutionContext::newWithContext(Heap::Object *with) const
{
    Heap::ExecutionContext *c = engine()->memoryManager->alloc<ExecutionContext>(Heap::ExecutionContext::Type_WithContext);
    c->outer.set(engine(), d());
    c->activation.set(engine(), with);

    return c;
}

Heap::ExecutionContext *ExecutionContext::newCatchContext(CppStackFrame *frame, int blockIndex, Heap::String *exceptionVarName)
{
    Scope scope(frame->context());
    ScopedString name(scope, exceptionVarName);
    ScopedValue val(scope, scope.engine->catchException(nullptr));
    ScopedContext ctx(scope, newBlockContext(frame, blockIndex));
    ctx->setProperty(name, val);
    return ctx->d();
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
        case Heap::ExecutionContext::Type_BlockContext:
            // never create activation records on block contexts
        default:
            break;
        }
        ctx = ctx->d()->outer;
    }

    PropertyKey id = name->toPropertyKey();
    if (activation->getOwnProperty(id) != Attr_Invalid)
        return;
    ScopedProperty desc(scope);
    PropertyAttributes attrs(Attr_Data);
    attrs.setConfigurable(deletable);
    if (!activation->defineOwnProperty(id, desc, attrs))
        scope.engine->throwTypeError();
}

static bool unscopable(ExecutionEngine *engine, Heap::Object *withObject, PropertyKey id)
{
    if (!withObject)
        return false;
    Scope scope(engine);
    ScopedObject w(scope, withObject);
    ScopedObject o(scope, w->get(scope.engine->symbol_unscopables()));
    if (o) {
        ScopedValue blocked(scope, o->get(id));
        return blocked->toBoolean();
    }
    return false;
}

bool ExecutionContext::deleteProperty(String *name)
{
    PropertyKey id = name->toPropertyKey();

    Heap::ExecutionContext *ctx = d();
    ExecutionEngine *engine = ctx->internalClass->engine;

    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_BlockContext:
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            uint index = c->internalClass->indexOfValueOrGetter(id);
            if (index < UINT_MAX)
                // ### throw in strict mode?
                return false;
            Q_FALLTHROUGH();
        }
        case Heap::ExecutionContext::Type_WithContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject object(scope, ctx->activation);
                if (object && object->hasProperty(id)) {
                    bool u = ::unscopable(engine, ctx->activation, id);
                    if (engine->hasException)
                        return false;
                    if (u)
                        break;
                    return object->deleteProperty(id);
                }
            }
            break;
        }
        case Heap::ExecutionContext::Type_GlobalContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject object(scope, ctx->activation);
                if (object && object->hasProperty(id))
                    return object->deleteProperty(id);
            }
            break;
        }
        case Heap::ExecutionContext::Type_QmlContext:
            // can't delete properties on qml objects
            break;
        }
    }

    return !engine->currentStackFrame->v4Function->isStrict();
}

ExecutionContext::Error ExecutionContext::setProperty(String *name, const Value &value)
{
    PropertyKey id = name->toPropertyKey();

    Heap::ExecutionContext *ctx = d();
    QV4::ExecutionEngine *engine = ctx->internalClass->engine;

    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_WithContext: {
            Scope scope(engine);
            ScopedObject w(scope, ctx->activation);
            if (w->hasProperty(id)) {
                bool u = ::unscopable(engine, ctx->activation, id);
                if (engine->hasException)
                    return TypeError;
                if (u)
                    break;
                if (!w->put(name, value))
                    return TypeError;
                return NoError;
            }
            break;
        }
        case Heap::ExecutionContext::Type_BlockContext:
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);
            uint index = c->internalClass->indexOfValueOrGetter(id);
            if (index < UINT_MAX) {
                static_cast<Heap::CallContext *>(c)->locals.set(engine, index, value);
                return NoError;
            }
        }
            Q_FALLTHROUGH();
        case Heap::ExecutionContext::Type_GlobalContext:
            if (ctx->activation) {
                auto member = ctx->activation->internalClass->findValueOrSetter(id);
                if (member.index < UINT_MAX) {
                    Scope scope(engine);
                    ScopedObject a(scope, ctx->activation);
                    if (!a->putValue(member.index, member.attrs, value))
                        return TypeError;
                    return NoError;
                }
            }
            break;
        case Heap::ExecutionContext::Type_QmlContext: {
            Scope scope(engine);
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
    PropertyKey id = name->toPropertyKey();

    Heap::ExecutionContext *ctx = d();
    QV4::ExecutionEngine *engine = ctx->internalClass->engine;

    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_BlockContext:
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);

            const uint index = c->internalClass->indexOfValueOrGetter(id);
            if (index < c->locals.alloc)
                return c->locals[index].asReturnedValue();

            // TODO: We should look up the module imports here, but those are part of the CU:
            //       imports[index - c->locals.size];
            //       See QTBUG-118478

            Q_FALLTHROUGH();
        }
        case Heap::ExecutionContext::Type_WithContext:
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject activation(scope, ctx->activation);
                if (activation->hasProperty(id)) {
                    bool u = ::unscopable(engine, ctx->activation, id);
                    if (engine->hasException)
                        return false;
                    if (u)
                        break;
                    return activation->get(id);
                }
            }
            break;
        case Heap::ExecutionContext::Type_GlobalContext:
        case Heap::ExecutionContext::Type_QmlContext: {
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject activation(scope, ctx->activation);
                bool hasProperty = false;
                ReturnedValue v = activation->get(id, nullptr, &hasProperty);
                if (hasProperty)
                    return v;
            }
            break;
        }
        }
    }
    return engine->throwReferenceError(*name);
}

ReturnedValue ExecutionContext::getPropertyAndBase(String *name, Value *base)
{
    base->setM(nullptr);
    PropertyKey id = name->toPropertyKey();

    Heap::ExecutionContext *ctx = d();
    QV4::ExecutionEngine *engine = ctx->internalClass->engine;

    for (; ctx; ctx = ctx->outer) {
        switch (ctx->type) {
        case Heap::ExecutionContext::Type_BlockContext:
        case Heap::ExecutionContext::Type_CallContext: {
            Heap::CallContext *c = static_cast<Heap::CallContext *>(ctx);

            const uint index = c->internalClass->indexOfValueOrGetter(id);
            if (index < c->locals.alloc)
                return c->locals[index].asReturnedValue();

            // TODO: We should look up the module imports here, but those are part of the CU:
            //       imports[index - c->locals.size];
            //       See QTBUG-118478

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
            if (ctx->activation) {
                Scope scope(this);
                ScopedObject activation(scope, ctx->activation);
                if (activation->hasProperty(id)) {
                    bool u = ::unscopable(engine, ctx->activation, id);
                    if (engine->hasException)
                        return false;
                    if (u)
                        break;
                    base->setM(activation->d());
                    return activation->get(id);
                }
            }
            break;
        case Heap::ExecutionContext::Type_QmlContext: {
            Scope scope(this);
            ScopedObject o(scope, ctx->activation);
            bool hasProperty = false;
            ReturnedValue v = o->get(id, nullptr, &hasProperty);
            if (hasProperty) {
                base->setM(o->d());
                return v;
            }
            break;
        }
        }
    }
    return engine->throwReferenceError(*name);
}

void Heap::CallContext::setArg(uint index, Value v)
{
    locals.set(internalClass->engine, locals.size + index, v);
}
