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

#include "qv4object_p.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4function_p.h"
#include "qv4mm_p.h"
#include "qv4exception_p.h"
#include "qv4arrayobject_p.h"
#include "qv4scopedvalue_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include "qv4alloca_p.h"

using namespace QV4;


DEFINE_MANAGED_VTABLE(FunctionObject);

FunctionObject::FunctionObject(ExecutionContext *scope, String *name, bool createProto)
    : Object(createProto ? scope->engine->functionWithProtoClass : scope->engine->functionClass)
    , scope(scope)
    , name(name)
    , formalParameterList(0)
    , varList(0)
    , formalParameterCount(0)
    , varCount(0)
    , function(0)
{
    vtbl = &static_vtbl;

    type = Type_FunctionObject;
    needsActivation = true;
    usesArgumentsObject = false;
    strictMode = false;
#ifndef QT_NO_DEBUG
     assert(scope->next != (ExecutionContext *)0x1);
#endif

     if (createProto) {
         Object *proto = scope->engine->newObject(scope->engine->protoClass);
         proto->memberData[Index_ProtoConstructor].value = Value::fromObject(this);
         memberData[Index_Prototype].value = Value::fromObject(proto);
     }

     if (name)
         defineReadonlyProperty(scope->engine->id_name, Value::fromString(name));
}

FunctionObject::FunctionObject(InternalClass *ic)
    : Object(ic)
    , scope(ic->engine->rootContext)
    , name(name)
    , formalParameterList(0)
    , varList(0)
    , formalParameterCount(0)
    , varCount(0)
    , function(0)
{
    vtbl = &static_vtbl;

    type = Type_FunctionObject;
}

FunctionObject::~FunctionObject()
{
    if (function)
        function->compilationUnit->deref();
}

Value FunctionObject::newInstance()
{
    ScopedCallData callData(engine(), 0);
    return construct(callData);
}

bool FunctionObject::hasInstance(Managed *that, const Value &value)
{
    FunctionObject *f = static_cast<FunctionObject *>(that);

    Object *v = value.asObject();
    if (!v)
        return false;

    ExecutionContext *ctx = f->engine()->current;
    Object *o = f->get(ctx->engine->id_prototype).asObject();
    if (!o)
        ctx->throwTypeError();

    while (v) {
        v = v->prototype();

        if (! v)
            break;
        else if (o == v)
            return true;
    }

    return false;
}

Value FunctionObject::construct(Managed *that, CallData *)
{
    FunctionObject *f = static_cast<FunctionObject *>(that);
    ExecutionEngine *v4 = f->engine();

    InternalClass *ic = v4->objectClass;
    Value proto = f->get(v4->id_prototype);
    if (proto.isObject())
        ic = v4->emptyClass->changePrototype(proto.objectValue());
    Object *obj = v4->newObject(ic);
    return Value::fromObject(obj);
}

Value FunctionObject::call(Managed *, CallData *)
{
    return Value::undefinedValue();
}

void FunctionObject::markObjects(Managed *that)
{
    FunctionObject *o = static_cast<FunctionObject *>(that);
    if (o->name)
        o->name->mark();
    // these are marked in VM::Function:
//    for (uint i = 0; i < formalParameterCount; ++i)
//        formalParameterList[i]->mark();
//    for (uint i = 0; i < varCount; ++i)
//        varList[i]->mark();
    o->scope->mark();
    if (o->function)
        o->function->mark();

    Object::markObjects(that);
}

FunctionObject *FunctionObject::creatScriptFunction(ExecutionContext *scope, Function *function)
{
    if (function->needsActivation() || function->compiledFunction->nFormals > QV4::Global::ReservedArgumentCount)
        return new (scope->engine->memoryManager) ScriptFunction(scope, function);
    return new (scope->engine->memoryManager) SimpleScriptFunction(scope, function);
}


DEFINE_MANAGED_VTABLE(FunctionCtor);

FunctionCtor::FunctionCtor(ExecutionContext *scope)
    : FunctionObject(scope, scope->engine->newIdentifier(QStringLiteral("Function")))
{
    vtbl = &static_vtbl;
}

// 15.3.2
Value FunctionCtor::construct(Managed *that, CallData *callData)
{
    FunctionCtor *f = static_cast<FunctionCtor *>(that);
    MemoryManager::GCBlocker gcBlocker(f->engine()->memoryManager);

    ExecutionContext *ctx = f->engine()->current;
    QString arguments;
    QString body;
    if (callData->argc > 0) {
        for (uint i = 0; i < callData->argc - 1; ++i) {
            if (i)
                arguments += QLatin1String(", ");
            arguments += callData->args[i].toString(ctx)->toQString();
        }
        body = callData->args[callData->argc - 1].toString(ctx)->toQString();
    }

    QString function = QLatin1String("function(") + arguments + QLatin1String("){") + body + QLatin1String("}");

    QQmlJS::Engine ee, *engine = &ee;
    QQmlJS::Lexer lexer(engine);
    lexer.setCode(function, 1, false);
    QQmlJS::Parser parser(engine);

    const bool parsed = parser.parseExpression();

    if (!parsed)
        f->engine()->current->throwSyntaxError(0);

    using namespace QQmlJS::AST;
    FunctionExpression *fe = QQmlJS::AST::cast<FunctionExpression *>(parser.rootNode());
    ExecutionEngine *v4 = f->engine();
    if (!fe)
        v4->current->throwSyntaxError(0);

    QQmlJS::V4IR::Module module;

    QQmlJS::RuntimeCodegen cg(v4->current, f->strictMode);
    cg.generateFromFunctionExpression(QString(), function, fe, &module);

    QV4::Compiler::JSUnitGenerator jsGenerator(&module);
    QScopedPointer<QQmlJS::EvalInstructionSelection> isel(v4->iselFactory->create(v4->executableAllocator, &module, &jsGenerator));
    QV4::CompiledData::CompilationUnit *compilationUnit = isel->compile();
    QV4::Function *vmf = compilationUnit->linkToEngine(v4);

    return Value::fromObject(FunctionObject::creatScriptFunction(v4->rootContext, vmf));
}

// 15.3.1: This is equivalent to new Function(...)
Value FunctionCtor::call(Managed *that, CallData *callData)
{
    return construct(that, callData);
}

FunctionPrototype::FunctionPrototype(InternalClass *ic)
    : FunctionObject(ic)
{
}

void FunctionPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));

    defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(0));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(ctx, QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(ctx, QStringLiteral("bind"), method_bind, 1);

}

Value FunctionPrototype::method_toString(SimpleCallContext *ctx)
{
    FunctionObject *fun = ctx->thisObject.asFunctionObject();
    if (!fun)
        ctx->throwTypeError();

    return Value::fromString(ctx, QStringLiteral("function() { [code] }"));
}

Value FunctionPrototype::method_apply(SimpleCallContext *ctx)
{
    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    Value thisArg = ctx->argument(0);
    Value arg = ctx->argument(1);

    Object *arr = arg.asObject();

    quint32 len;
    if (!arr) {
        len = 0;
        if (!arg.isNullOrUndefined()) {
            ctx->throwTypeError();
            return Value::undefinedValue();
        }
    } else {
        len = ArrayPrototype::getLength(ctx, arr);
    }

    ScopedCallData callData(ctx->engine, len);

    if (len) {
        if (arr->protoHasArray() || arr->hasAccessorProperty) {
            for (quint32 i = 0; i < len; ++i)
                callData->args[i] = arr->getIndexed(i);
        } else {
            int alen = qMin(len, arr->arrayDataLen);
            for (quint32 i = 0; i < alen; ++i)
                callData->args[i] = arr->arrayData[i].value;
            for (quint32 i = alen; i < len; ++i)
                callData->args[i] = Value::undefinedValue();
        }
    }

    callData->thisObject = thisArg;
    return o->call(callData);
}

Value FunctionPrototype::method_call(SimpleCallContext *ctx)
{
    Value thisArg = ctx->argument(0);

    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    ScopedCallData callData(ctx->engine, ctx->argumentCount ? ctx->argumentCount - 1 : 0);
    if (ctx->argumentCount)
        qCopy(ctx->arguments + 1,
              ctx->arguments + ctx->argumentCount, callData->args);
    callData->thisObject = thisArg;
    return o->call(callData);
}

Value FunctionPrototype::method_bind(SimpleCallContext *ctx)
{
    FunctionObject *target = ctx->thisObject.asFunctionObject();
    if (!target)
        ctx->throwTypeError();

    Value boundThis = ctx->argument(0);
    QVector<Value> boundArgs;
    for (uint i = 1; i < ctx->argumentCount; ++i)
        boundArgs += ctx->argument(i);


    BoundFunction *f = ctx->engine->newBoundFunction(ctx->engine->rootContext, target, boundThis, boundArgs);
    return Value::fromObject(f);
}


static Value throwTypeError(SimpleCallContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

DEFINE_MANAGED_VTABLE(ScriptFunction);

ScriptFunction::ScriptFunction(ExecutionContext *scope, Function *function)
    : FunctionObject(scope, function->name, true)
{
    vtbl = &static_vtbl;
    this->function = function;
    this->function->compilationUnit->ref();
    Q_ASSERT(function);
    Q_ASSERT(function->codePtr);

    // global function
    if (!scope)
        return;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    needsActivation = function->needsActivation();
    usesArgumentsObject = function->usesArgumentsObject();
    strictMode = function->isStrict();
    formalParameterCount = function->formals.size();
    formalParameterList = function->formals.constData();
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(formalParameterCount));

    varCount = function->locals.size();
    varList = function->locals.constData();

    if (scope->strictMode) {
        FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, throwTypeError);
        Property pd = Property::fromAccessor(thrower, thrower);
        __defineOwnProperty__(scope, QStringLiteral("caller"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
        __defineOwnProperty__(scope, QStringLiteral("arguments"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
    }
}

Value ScriptFunction::construct(Managed *that, CallData *callData)
{
    ScriptFunction *f = static_cast<ScriptFunction *>(that);
    SAVE_JS_STACK(f->scope);
    ExecutionEngine *v4 = f->engine();

    InternalClass *ic = v4->objectClass;
    Value proto = f->memberData[Index_Prototype].value;
    if (proto.isObject())
        ic = v4->emptyClass->changePrototype(proto.objectValue());
    Object *obj = v4->newObject(ic);

    ExecutionContext *context = v4->current;
    callData->thisObject = Value::fromObject(obj);
    ExecutionContext *ctx = context->newCallContext(f, callData);

    Value result;
    try {
        result = f->function->code(ctx, f->function->codeData);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }
    ctx->engine->popContext();

    CHECK_JS_STACK(f->scope);
    if (result.isObject())
        return result;
    return Value::fromObject(obj);
}

Value ScriptFunction::call(Managed *that, CallData *callData)
{
    ScriptFunction *f = static_cast<ScriptFunction *>(that);
    SAVE_JS_STACK(f->scope);
    void *stackSpace;
    ExecutionContext *context = f->engine()->current;
    CallContext *ctx = context->newCallContext(f, callData);

    if (!f->strictMode && !callData->thisObject.isObject()) {
        if (callData->thisObject.isNullOrUndefined()) {
            ctx->thisObject = Value::fromObject(f->engine()->globalObject);
        } else {
            ctx->thisObject = Value::fromObject(callData->thisObject.toObject(context));
        }
    }

    Value result;
    try {
        result = f->function->code(ctx, f->function->codeData);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }
    ctx->engine->popContext();
    CHECK_JS_STACK(f->scope);
    return result;
}

DEFINE_MANAGED_VTABLE(SimpleScriptFunction);

SimpleScriptFunction::SimpleScriptFunction(ExecutionContext *scope, Function *function)
    : FunctionObject(scope, function->name, true)
{
    vtbl = &static_vtbl;
    this->function = function;
    this->function->compilationUnit->ref();
    Q_ASSERT(function);
    Q_ASSERT(function->codePtr);

    // global function
    if (!scope)
        return;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    needsActivation = function->needsActivation();
    usesArgumentsObject = function->usesArgumentsObject();
    strictMode = function->isStrict();
    formalParameterCount = function->formals.size();
    formalParameterList = function->formals.constData();
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(formalParameterCount));

    varCount = function->locals.size();
    varList = function->locals.constData();

    if (scope->strictMode) {
        FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, throwTypeError);
        Property pd = Property::fromAccessor(thrower, thrower);
        __defineOwnProperty__(scope, QStringLiteral("caller"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
        __defineOwnProperty__(scope, QStringLiteral("arguments"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
    }
}

Value SimpleScriptFunction::construct(Managed *that, CallData *callData)
{
    SimpleScriptFunction *f = static_cast<SimpleScriptFunction *>(that);
    SAVE_JS_STACK(f->scope);
    ExecutionEngine *v4 = f->engine();

    InternalClass *ic = v4->objectClass;
    Value proto = f->memberData[Index_Prototype].value;
    if (proto.isObject())
        ic = v4->emptyClass->changePrototype(proto.objectValue());
    Object *obj = v4->newObject(ic);

    ExecutionContext *context = v4->current;
    void *stackSpace = alloca(requiredMemoryForExecutionContectSimple(f));
    callData->thisObject = Value::fromObject(obj);
    ExecutionContext *ctx = context->newCallContext(stackSpace, f, callData);

    Value result;
    try {
        result = f->function->code(ctx, f->function->codeData);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }
    ctx->engine->popContext();

    CHECK_JS_STACK(f->scope);
    if (result.isObject())
        return result;
    return Value::fromObject(obj);
}

Value SimpleScriptFunction::call(Managed *that, CallData *callData)
{
    SimpleScriptFunction *f = static_cast<SimpleScriptFunction *>(that);
    SAVE_JS_STACK(f->scope);
    void *stackSpace = alloca(requiredMemoryForExecutionContectSimple(f));
    ExecutionContext *context = f->engine()->current;
    ExecutionContext *ctx = context->newCallContext(stackSpace, f, callData);

    if (!f->strictMode && !callData->thisObject.isObject()) {
        if (callData->thisObject.isNullOrUndefined()) {
            ctx->thisObject = Value::fromObject(f->engine()->globalObject);
        } else {
            ctx->thisObject = Value::fromObject(callData->thisObject.toObject(context));
        }
    }

    Value result;
    try {
        result = f->function->code(ctx, f->function->codeData);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }
    ctx->engine->popContext();
    CHECK_JS_STACK(f->scope);
    return result;
}




DEFINE_MANAGED_VTABLE(BuiltinFunctionOld);

BuiltinFunctionOld::BuiltinFunctionOld(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *))
    : FunctionObject(scope, name)
    , code(code)
{
    vtbl = &static_vtbl;
    isBuiltinFunction = true;
}

Value BuiltinFunctionOld::construct(Managed *f, CallData *)
{
    f->engine()->current->throwTypeError();
    return Value::undefinedValue();
}

Value BuiltinFunctionOld::call(Managed *that, CallData *callData)
{
    BuiltinFunctionOld *f = static_cast<BuiltinFunctionOld *>(that);
    ExecutionEngine *v4 = f->engine();
    ExecutionContext *context = v4->current;

    SimpleCallContext ctx;
    ctx.initSimpleCallContext(f->scope->engine);
    ctx.strictMode = f->scope->strictMode; // ### needed? scope or parent context?
    ctx.thisObject = callData->thisObject;
    // ### const_cast
    ctx.arguments = const_cast<Value *>(callData->args);
    ctx.argumentCount = callData->argc;
    v4->pushContext(&ctx);

    Value result;
    try {
        result = f->code(&ctx);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }

    context->engine->popContext();
    return result;
}

Value IndexedBuiltinFunction::call(Managed *that, CallData *callData)
{
    IndexedBuiltinFunction *f = static_cast<IndexedBuiltinFunction *>(that);
    ExecutionEngine *v4 = f->engine();
    ExecutionContext *context = v4->current;

    SimpleCallContext ctx;
    ctx.initSimpleCallContext(f->scope->engine);
    ctx.strictMode = f->scope->strictMode; // ### needed? scope or parent context?
    ctx.thisObject = callData->thisObject;
    // ### const_cast
    ctx.arguments = const_cast<Value *>(callData->args);
    ctx.argumentCount = callData->argc;
    v4->pushContext(&ctx);

    Value result;
    try {
        result = f->code(&ctx, f->index);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }

    context->engine->popContext();
    return result;
}

DEFINE_MANAGED_VTABLE(IndexedBuiltinFunction);

DEFINE_MANAGED_VTABLE(BoundFunction);

BoundFunction::BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs)
    : FunctionObject(scope, 0)
    , target(target)
    , boundThis(boundThis)
    , boundArgs(boundArgs)
{
    vtbl = &static_vtbl;
    int len = target->get(scope->engine->id_length).toUInt32();
    len -= boundArgs.size();
    if (len < 0)
        len = 0;
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(len));

    FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, throwTypeError);
    Property pd = Property::fromAccessor(thrower, thrower);
    *insertMember(scope->engine->id_arguments, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable) = pd;
    *insertMember(scope->engine->id_caller, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable) = pd;
}

void BoundFunction::destroy(Managed *that)
{
    static_cast<BoundFunction *>(that)->~BoundFunction();
}

Value BoundFunction::call(Managed *that, CallData *dd)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);

    ScopedCallData callData(f->scope->engine, f->boundArgs.size() + dd->argc);
    callData->thisObject = f->boundThis;
    memcpy(callData->args, f->boundArgs.constData(), f->boundArgs.size()*sizeof(Value));
    memcpy(callData->args + f->boundArgs.size(), dd->args, dd->argc*sizeof(Value));
    return f->target->call(callData);
}

Value BoundFunction::construct(Managed *that, CallData *dd)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    ScopedCallData callData(f->scope->engine, f->boundArgs.size() + dd->argc);
    memcpy(callData->args, f->boundArgs.constData(), f->boundArgs.size()*sizeof(Value));
    memcpy(callData->args + f->boundArgs.size(), dd->args, dd->argc*sizeof(Value));
    return f->target->construct(callData);
}

bool BoundFunction::hasInstance(Managed *that, const Value &value)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    return FunctionObject::hasInstance(f->target, value);
}

void BoundFunction::markObjects(Managed *that)
{
    BoundFunction *o = static_cast<BoundFunction *>(that);
    o->target->mark();
    if (Managed *m = o->boundThis.asManaged())
        m->mark();
    for (int i = 0; i < o->boundArgs.size(); ++i)
        if (Managed *m = o->boundArgs.at(i).asManaged())
            m->mark();
    FunctionObject::markObjects(that);
}
