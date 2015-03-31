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

#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4function_p.h"
#include "qv4mm_p.h"

#include "qv4arrayobject_p.h"
#include "qv4scopedvalue_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlengine_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"
#include "private/qqmlbuiltinfunctions_p.h"

#include <QtCore/QDebug>
#include <algorithm>
#include "qv4alloca_p.h"
#include "qv4profiling_p.h"

using namespace QV4;


DEFINE_OBJECT_VTABLE(FunctionObject);

Heap::FunctionObject::FunctionObject(QV4::ExecutionContext *scope, QV4::String *name, bool createProto)
    : Heap::Object(scope->d()->engine->functionClass, scope->d()->engine->functionPrototype.asObject())
    , scope(scope->d())
    , function(Q_NULLPTR)
{
    Scope s(scope->engine());
    ScopedFunctionObject f(s, this);
    f->init(name, createProto);
}

Heap::FunctionObject::FunctionObject(QV4::ExecutionContext *scope, Function *function, bool createProto)
    : Heap::Object(scope->d()->engine->functionClass, scope->d()->engine->functionPrototype.asObject())
    , scope(scope->d())
    , function(Q_NULLPTR)
{
    Scope s(scope->engine());
    ScopedString name(s, function->name());
    ScopedFunctionObject f(s, this);
    f->init(name, createProto);
}

Heap::FunctionObject::FunctionObject(QV4::ExecutionContext *scope, const QString &name, bool createProto)
    : Heap::Object(scope->d()->engine->functionClass, scope->d()->engine->functionPrototype.asObject())
    , scope(scope->d())
    , function(Q_NULLPTR)
{
    Scope s(scope->engine());
    ScopedFunctionObject f(s, this);
    ScopedString n(s, s.engine->newString(name));
    f->init(n, createProto);
}

Heap::FunctionObject::FunctionObject(ExecutionContext *scope, const QString &name, bool createProto)
    : Heap::Object(scope->engine->functionClass, scope->engine->functionPrototype.asObject())
    , scope(scope)
    , function(Q_NULLPTR)
{
    Scope s(scope->engine);
    ScopedFunctionObject f(s, this);
    ScopedString n(s, s.engine->newString(name));
    f->init(n, createProto);
}

Heap::FunctionObject::FunctionObject(QV4::ExecutionContext *scope, const ReturnedValue name)
    : Heap::Object(scope->d()->engine->functionClass, scope->d()->engine->functionPrototype.asObject())
    , scope(scope->d())
    , function(Q_NULLPTR)
{
    Scope s(scope);
    ScopedFunctionObject f(s, this);
    ScopedString n(s, name);
    f->init(n, false);
}

Heap::FunctionObject::FunctionObject(ExecutionContext *scope, const ReturnedValue name)
    : Heap::Object(scope->engine->functionClass, scope->engine->functionPrototype.asObject())
    , scope(scope)
    , function(Q_NULLPTR)
{
    Scope s(scope->engine);
    ScopedFunctionObject f(s, this);
    ScopedString n(s, name);
    f->init(n, false);
}

Heap::FunctionObject::FunctionObject(InternalClass *ic, QV4::Object *prototype)
    : Heap::Object(ic, prototype)
    , scope(ic->engine->rootContext())
    , function(Q_NULLPTR)
{
    Scope scope(ic->engine);
    ScopedObject o(scope, this);
    o->ensureMemberIndex(ic->engine, Index_Prototype);
    memberData->data[Index_Prototype] = Encode::undefined();
}


Heap::FunctionObject::~FunctionObject()
{
    if (function)
        function->compilationUnit->release();
}

void FunctionObject::init(String *n, bool createProto)
{
    Scope s(internalClass()->engine);
    ScopedValue protectThis(s, this);

    ensureMemberIndex(s.engine, Heap::FunctionObject::Index_Prototype);
    if (createProto) {
        ScopedObject proto(s, scope()->engine->newObject(s.engine->protoClass, s.engine->objectPrototype.asObject()));
        proto->ensureMemberIndex(s.engine, Heap::FunctionObject::Index_ProtoConstructor);
        proto->memberData()->data[Heap::FunctionObject::Index_ProtoConstructor] = this->asReturnedValue();
        memberData()->data[Heap::FunctionObject::Index_Prototype] = proto.asReturnedValue();
    } else {
        memberData()->data[Heap::FunctionObject::Index_Prototype] = Encode::undefined();
    }

    ScopedValue v(s, n);
    defineReadonlyProperty(s.engine->id_name, v);
}

ReturnedValue FunctionObject::name()
{
    return get(scope()->engine->id_name);
}


ReturnedValue FunctionObject::newInstance()
{
    Scope scope(internalClass()->engine);
    ScopedCallData callData(scope);
    return construct(callData);
}

ReturnedValue FunctionObject::construct(Managed *that, CallData *)
{
    static_cast<FunctionObject *>(that)->internalClass()->engine->throwTypeError();
    return Encode::undefined();
}

ReturnedValue FunctionObject::call(Managed *, CallData *)
{
    return Encode::undefined();
}

void FunctionObject::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    Heap::FunctionObject *o = static_cast<Heap::FunctionObject *>(that);
    if (o->scope)
        o->scope->mark(e);

    Object::markObjects(that, e);
}

Heap::FunctionObject *FunctionObject::createScriptFunction(ExecutionContext *scope, Function *function, bool createProto)
{
    if (function->needsActivation() ||
        function->compiledFunction->flags & CompiledData::Function::HasCatchOrWith ||
        function->compiledFunction->nFormals > QV4::Global::ReservedArgumentCount ||
        function->isNamedExpression())
        return scope->d()->engine->memoryManager->alloc<ScriptFunction>(scope, function);
    return scope->d()->engine->memoryManager->alloc<SimpleScriptFunction>(scope, function, createProto);
}

bool FunctionObject::isBinding() const
{
    return d()->vtable == QQmlBindingFunction::staticVTable();
}

bool FunctionObject::isBoundFunction() const
{
    return d()->vtable == BoundFunction::staticVTable();
}

QQmlSourceLocation FunctionObject::sourceLocation() const
{
    if (isBinding()) {
        Q_ASSERT(as<const QV4::QQmlBindingFunction>());
        return static_cast<QV4::Heap::QQmlBindingFunction *>(d())->bindingLocation;
    }
    QV4::Function *function = d()->function;
    Q_ASSERT(function);

    return QQmlSourceLocation(function->sourceFile(), function->compiledFunction->location.line, function->compiledFunction->location.column);
}

DEFINE_OBJECT_VTABLE(FunctionCtor);

Heap::FunctionCtor::FunctionCtor(QV4::ExecutionContext *scope)
    : Heap::FunctionObject(scope, QStringLiteral("Function"))
{
}

// 15.3.2
ReturnedValue FunctionCtor::construct(Managed *that, CallData *callData)
{
    Scope scope(static_cast<Object *>(that)->engine());
    Scoped<FunctionCtor> f(scope, static_cast<FunctionCtor *>(that));
    ScopedContext ctx(scope, scope.engine->currentContext());
    QString arguments;
    QString body;
    if (callData->argc > 0) {
        for (int i = 0; i < callData->argc - 1; ++i) {
            if (i)
                arguments += QLatin1String(", ");
            arguments += callData->args[i].toQString();
        }
        body = callData->args[callData->argc - 1].toQString();
    }
    if (ctx->d()->engine->hasException)
        return Encode::undefined();

    QString function = QLatin1String("function(") + arguments + QLatin1String("){") + body + QLatin1String("}");

    QQmlJS::Engine ee, *engine = &ee;
    QQmlJS::Lexer lexer(engine);
    lexer.setCode(function, 1, false);
    QQmlJS::Parser parser(engine);

    const bool parsed = parser.parseExpression();

    if (!parsed)
        return scope.engine->throwSyntaxError(QLatin1String("Parse error"));

    using namespace QQmlJS::AST;
    FunctionExpression *fe = QQmlJS::AST::cast<FunctionExpression *>(parser.rootNode());
    if (!fe)
        return scope.engine->throwSyntaxError(QLatin1String("Parse error"));

    IR::Module module(scope.engine->debugger != 0);

    QQmlJS::RuntimeCodegen cg(scope.engine, f->strictMode());
    cg.generateFromFunctionExpression(QString(), function, fe, &module);

    Compiler::JSUnitGenerator jsGenerator(&module);
    QScopedPointer<EvalInstructionSelection> isel(scope.engine->iselFactory->create(QQmlEnginePrivate::get(scope.engine), scope.engine->executableAllocator, &module, &jsGenerator));
    QQmlRefPointer<CompiledData::CompilationUnit> compilationUnit = isel->compile();
    Function *vmf = compilationUnit->linkToEngine(scope.engine);

    ScopedContext global(scope, scope.engine->rootContext());
    return FunctionObject::createScriptFunction(global, vmf)->asReturnedValue();
}

// 15.3.1: This is equivalent to new Function(...)
ReturnedValue FunctionCtor::call(Managed *that, CallData *callData)
{
    return construct(that, callData);
}

DEFINE_OBJECT_VTABLE(FunctionPrototype);

Heap::FunctionPrototype::FunctionPrototype(InternalClass *ic, QV4::Object *prototype)
    : Heap::FunctionObject(ic, prototype)
{
}

void FunctionPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);

    ctor->defineReadonlyProperty(engine->id_length, Primitive::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype, (o = this));

    defineReadonlyProperty(engine->id_length, Primitive::fromInt32(0));
    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString, method_toString, 0);
    defineDefaultProperty(QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(QStringLiteral("bind"), method_bind, 1);

}

ReturnedValue FunctionPrototype::method_toString(CallContext *ctx)
{
    FunctionObject *fun = ctx->thisObject().asFunctionObject();
    if (!fun)
        return ctx->engine()->throwTypeError();

    return ctx->d()->engine->newString(QStringLiteral("function() { [code] }"))->asReturnedValue();
}

ReturnedValue FunctionPrototype::method_apply(CallContext *ctx)
{
    Scope scope(ctx);
    ScopedFunctionObject o(scope, ctx->thisObject().asFunctionObject());
    if (!o)
        return ctx->engine()->throwTypeError();

    ScopedValue arg(scope, ctx->argument(1));

    ScopedObject arr(scope, arg);

    quint32 len;
    if (!arr) {
        len = 0;
        if (!arg->isNullOrUndefined())
            return ctx->engine()->throwTypeError();
    } else {
        len = arr->getLength();
    }

    ScopedCallData callData(scope, len);

    if (len) {
        if (arr->arrayType() != Heap::ArrayData::Simple || arr->protoHasArray()) {
            for (quint32 i = 0; i < len; ++i)
                callData->args[i] = arr->getIndexed(i);
        } else {
            uint alen = arr->arrayData() ? arr->arrayData()->len : 0;
            if (alen > len)
                alen = len;
            for (uint i = 0; i < alen; ++i)
                callData->args[i] = static_cast<Heap::SimpleArrayData *>(arr->arrayData())->data(i);
            for (quint32 i = alen; i < len; ++i)
                callData->args[i] = Primitive::undefinedValue();
        }
    }

    callData->thisObject = ctx->argument(0);
    return o->call(callData);
}

ReturnedValue FunctionPrototype::method_call(CallContext *ctx)
{
    Scope scope(ctx);

    ScopedFunctionObject o(scope, ctx->thisObject().asFunctionObject());
    if (!o)
        return ctx->engine()->throwTypeError();

    ScopedCallData callData(scope, ctx->argc() ? ctx->argc() - 1 : 0);
    if (ctx->argc()) {
        for (int i = 1; i < ctx->argc(); ++i)
            callData->args[i - 1] = ctx->args()[i];
    }
    callData->thisObject = ctx->argument(0);
    return o->call(callData);
}

ReturnedValue FunctionPrototype::method_bind(CallContext *ctx)
{
    Scope scope(ctx);
    ScopedFunctionObject target(scope, ctx->thisObject());
    if (!target)
        return ctx->engine()->throwTypeError();

    ScopedValue boundThis(scope, ctx->argument(0));
    Scoped<MemberData> boundArgs(scope, (Heap::MemberData *)0);
    if (ctx->argc() > 1) {
        boundArgs = MemberData::reallocate(scope.engine, 0, ctx->argc() - 1);
        boundArgs->d()->size = ctx->argc() - 1;
        memcpy(boundArgs->data(), ctx->args() + 1, (ctx->argc() - 1)*sizeof(Value));
    }

    ScopedContext global(scope, scope.engine->rootContext());
    return BoundFunction::create(global, target, boundThis, boundArgs)->asReturnedValue();
}

DEFINE_OBJECT_VTABLE(ScriptFunction);

Heap::ScriptFunction::ScriptFunction(QV4::ExecutionContext *scope, Function *function)
    : Heap::SimpleScriptFunction(scope, function, true)
{
}

ReturnedValue ScriptFunction::construct(Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<Object *>(that)->engine();
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    Scoped<ScriptFunction> f(scope, static_cast<ScriptFunction *>(that));

    InternalClass *ic = scope.engine->emptyClass;
    ScopedObject proto(scope, f->protoForConstructor());
    ScopedObject obj(scope, v4->newObject(ic, proto));

    ScopedContext context(scope, v4->currentContext());
    callData->thisObject = obj.asReturnedValue();
    Scoped<CallContext> ctx(scope, context->newCallContext(f, callData));

    ExecutionContextSaver ctxSaver(scope, context);
    ScopedValue result(scope, Q_V4_PROFILE(v4, f->function()));

    if (f->function()->compiledFunction->hasQmlDependencies())
        QmlContextWrapper::registerQmlDependencies(v4, f->function()->compiledFunction);

    if (v4->hasException)
        return Encode::undefined();

    if (result->isObject())
        return result->asReturnedValue();
    return obj.asReturnedValue();
}

ReturnedValue ScriptFunction::call(Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<Object *>(that)->engine();
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    Scoped<ScriptFunction> f(scope, static_cast<ScriptFunction *>(that));
    ScopedContext context(scope, v4->currentContext());

    Scoped<CallContext> ctx(scope, context->newCallContext(f, callData));

    ExecutionContextSaver ctxSaver(scope, context);
    ScopedValue result(scope, Q_V4_PROFILE(v4, f->function()));

    if (f->function()->compiledFunction->hasQmlDependencies())
        QmlContextWrapper::registerQmlDependencies(ctx->d()->engine, f->function()->compiledFunction);

    return result->asReturnedValue();
}

DEFINE_OBJECT_VTABLE(SimpleScriptFunction);

Heap::SimpleScriptFunction::SimpleScriptFunction(QV4::ExecutionContext *scope, Function *function, bool createProto)
    : Heap::FunctionObject(function->compilationUnit->engine->simpleScriptFunctionClass, function->compilationUnit->engine->functionPrototype.asObject())
{
    this->scope = scope->d();

    this->function = function;
    function->compilationUnit->addref();
    Q_ASSERT(function);
    Q_ASSERT(function->code);

    Scope s(scope);
    ScopedFunctionObject f(s, this);

    if (createProto) {
        ScopedString name(s, function->name());
        f->init(name, createProto);
        f->defineReadonlyProperty(scope->d()->engine->id_length, Primitive::fromInt32(f->formalParameterCount()));
    } else {
        f->ensureMemberIndex(s.engine, Index_Length);
        memberData->data[Index_Name] = function->name();
        memberData->data[Index_Length] = Primitive::fromInt32(f->formalParameterCount());
    }

    if (scope->d()->strictMode) {
        ScopedProperty pd(s);
        pd->value = s.engine->thrower;
        pd->set = s.engine->thrower;
        f->insertMember(scope->d()->engine->id_caller, pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        f->insertMember(scope->d()->engine->id_arguments, pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    }
}

ReturnedValue SimpleScriptFunction::construct(Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<Object *>(that)->engine();
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    Scoped<SimpleScriptFunction> f(scope, static_cast<SimpleScriptFunction *>(that));

    InternalClass *ic = scope.engine->emptyClass;
    ScopedObject proto(scope, f->protoForConstructor());
    callData->thisObject = v4->newObject(ic, proto);

    ExecutionContextSaver ctxSaver(scope, v4->currentContext());

    CallContext::Data ctx(v4);
    ctx.vtable = CallContext::staticVTable();
    ctx.strictMode = f->strictMode();
    ctx.callData = callData;
    ctx.function = f->d();
    ctx.compilationUnit = f->function()->compilationUnit;
    ctx.lookups = ctx.compilationUnit->runtimeLookups;
    ctx.outer = f->scope();
    ctx.locals = scope.alloc(f->varCount());
    for (int i = callData->argc; i < (int)f->formalParameterCount(); ++i)
        callData->args[i] = Encode::undefined();
    Q_ASSERT(v4->currentContext() == &ctx);

    ScopedObject result(scope, Q_V4_PROFILE(v4, f->function()));

    if (f->function()->compiledFunction->hasQmlDependencies())
        QmlContextWrapper::registerQmlDependencies(v4, f->function()->compiledFunction);

    if (!result)
        return callData->thisObject.asReturnedValue();
    return result.asReturnedValue();
}

ReturnedValue SimpleScriptFunction::call(Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<SimpleScriptFunction *>(that)->internalClass()->engine;
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    Scoped<SimpleScriptFunction> f(scope, static_cast<SimpleScriptFunction *>(that));

    ExecutionContextSaver ctxSaver(scope, v4->currentContext());

    CallContext::Data ctx(v4);
    ctx.vtable = CallContext::staticVTable();
    ctx.strictMode = f->strictMode();
    ctx.callData = callData;
    ctx.function = f->d();
    ctx.compilationUnit = f->function()->compilationUnit;
    ctx.lookups = ctx.compilationUnit->runtimeLookups;
    ctx.outer = f->scope();
    ctx.locals = scope.alloc(f->varCount());
    for (int i = callData->argc; i < (int)f->formalParameterCount(); ++i)
        callData->args[i] = Encode::undefined();
    Q_ASSERT(v4->currentContext() == &ctx);

    ScopedValue result(scope, Q_V4_PROFILE(v4, f->function()));

    if (f->function()->compiledFunction->hasQmlDependencies())
        QmlContextWrapper::registerQmlDependencies(v4, f->function()->compiledFunction);

    return result->asReturnedValue();
}

Heap::Object *SimpleScriptFunction::protoForConstructor()
{
    Scope scope(engine());
    ScopedObject p(scope, protoProperty());
    if (p)
        return p->d();
    return scope.engine->objectPrototype.asObject()->d();
}



DEFINE_OBJECT_VTABLE(BuiltinFunction);

Heap::BuiltinFunction::BuiltinFunction(QV4::ExecutionContext *scope, QV4::String *name, ReturnedValue (*code)(QV4::CallContext *))
    : Heap::FunctionObject(scope, name)
    , code(code)
{
}

ReturnedValue BuiltinFunction::construct(Managed *f, CallData *)
{
    return static_cast<BuiltinFunction *>(f)->internalClass()->engine->throwTypeError();
}

ReturnedValue BuiltinFunction::call(Managed *that, CallData *callData)
{
    BuiltinFunction *f = static_cast<BuiltinFunction *>(that);
    ExecutionEngine *v4 = f->internalClass()->engine;
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    ExecutionContextSaver ctxSaver(scope, v4->currentContext());

    CallContext::Data ctx(v4);
    ctx.vtable = CallContext::staticVTable();
    ctx.strictMode = f->scope()->strictMode; // ### needed? scope or parent context?
    ctx.callData = callData;
    Q_ASSERT(v4->currentContext() == &ctx);
    Scoped<CallContext> sctx(scope, &ctx);

    return f->d()->code(sctx);
}

ReturnedValue IndexedBuiltinFunction::call(Managed *that, CallData *callData)
{
    IndexedBuiltinFunction *f = static_cast<IndexedBuiltinFunction *>(that);
    ExecutionEngine *v4 = f->internalClass()->engine;
    if (v4->hasException)
        return Encode::undefined();
    CHECK_STACK_LIMITS(v4);

    Scope scope(v4);
    ExecutionContextSaver ctxSaver(scope, v4->currentContext());

    CallContext::Data ctx(v4);
    ctx.vtable = CallContext::staticVTable();
    ctx.strictMode = f->scope()->strictMode; // ### needed? scope or parent context?
    ctx.callData = callData;
    Q_ASSERT(v4->currentContext() == &ctx);
    Scoped<CallContext> sctx(scope, &ctx);

    return f->d()->code(sctx, f->d()->index);
}

DEFINE_OBJECT_VTABLE(IndexedBuiltinFunction);

DEFINE_OBJECT_VTABLE(BoundFunction);

Heap::BoundFunction::BoundFunction(QV4::ExecutionContext *scope, QV4::FunctionObject *target,
                                   const Value &boundThis, QV4::MemberData *boundArgs)
    : Heap::FunctionObject(scope, QStringLiteral("__bound function__"))
    , target(target->d())
    , boundArgs(boundArgs ? boundArgs->d() : 0)
{
    this->boundThis = boundThis;

    Scope s(scope);
    ScopedObject f(s, this);

    ScopedValue l(s, target->get(s.engine->id_length));
    int len = l->toUInt32();
    if (boundArgs)
        len -= boundArgs->size();
    if (len < 0)
        len = 0;
    f->defineReadonlyProperty(s.engine->id_length, Primitive::fromInt32(len));

    ScopedProperty pd(s);
    pd->value = s.engine->thrower;
    pd->set = s.engine->thrower;
    f->insertMember(s.engine->id_arguments, pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    f->insertMember(s.engine->id_caller, pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
}

ReturnedValue BoundFunction::call(Managed *that, CallData *dd)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    Scope scope(f->engine());
    if (scope.hasException())
        return Encode::undefined();

    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    ScopedCallData callData(scope, (boundArgs ? boundArgs->size() : 0) + dd->argc);
    callData->thisObject = f->boundThis();
    Value *argp = callData->args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, dd->args, dd->argc*sizeof(Value));
    ScopedFunctionObject t(scope, f->target());
    return t->call(callData);
}

ReturnedValue BoundFunction::construct(Managed *that, CallData *dd)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    Scope scope(f->engine());
    if (scope.hasException())
        return Encode::undefined();

    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    ScopedCallData callData(scope, (boundArgs ? boundArgs->size() : 0) + dd->argc);
    Value *argp = callData->args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, dd->args, dd->argc*sizeof(Value));
    ScopedFunctionObject t(scope, f->target());
    return t->construct(callData);
}

void BoundFunction::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    BoundFunction::Data *o = static_cast<BoundFunction::Data *>(that);
    o->target->mark(e);
    o->boundThis.mark(e);
    if (o->boundArgs)
        o->boundArgs->mark(e);
    FunctionObject::markObjects(that, e);
}
