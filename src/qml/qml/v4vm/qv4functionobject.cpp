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

#include "qv4object.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include "qv4mm.h"

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

using namespace QQmlJS::VM;


DEFINE_MANAGED_VTABLE(FunctionObject);

Function::~Function()
{
    delete[] codeData;
}

void Function::mark()
{
    if (name)
        name->mark();
    for (int i = 0; i < formals.size(); ++i)
        formals.at(i)->mark();
    for (int i = 0; i < locals.size(); ++i)
        locals.at(i)->mark();
    for (int i = 0; i < generatedValues.size(); ++i)
        if (Managed *m = generatedValues.at(i).asManaged())
            m->mark();
    for (int i = 0; i < identifiers.size(); ++i)
        identifiers.at(i)->mark();
}

FunctionObject::FunctionObject(ExecutionContext *scope)
    : Object(scope->engine)
    , scope(scope)
    , name(0)
    , formalParameterList(0)
    , varList(0)
    , formalParameterCount(0)
    , varCount(0)
    , function(0)
{
    vtbl = &static_vtbl;
    prototype = scope->engine->functionPrototype;

    type = Type_FunctionObject;
    needsActivation = true;
    usesArgumentsObject = false;
    strictMode = false;
#ifndef QT_NO_DEBUG
     assert(scope->next != (ExecutionContext *)0x1);
#endif
}

bool FunctionObject::hasInstance(Managed *that, ExecutionContext *ctx, const Value &value)
{
    FunctionObject *f = static_cast<FunctionObject *>(that);

    Object *v = value.asObject();
    if (!v)
        return false;

    Object *o = f->get(ctx, ctx->engine->id_prototype).asObject();
    if (!o)
        ctx->throwTypeError();

    while (v) {
        v = v->prototype;

        if (! v)
            break;
        else if (o == v)
            return true;
    }

    return false;
}

Value FunctionObject::construct(Managed *that, ExecutionContext *context, Value *, int)
{
    FunctionObject *f = static_cast<FunctionObject *>(that);

    Object *obj = context->engine->newObject();
    Value proto = f->get(context, context->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();
    return Value::fromObject(obj);
}

Value FunctionObject::call(Managed *, ExecutionContext *, const Value &, Value *, int)
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


DEFINE_MANAGED_VTABLE(FunctionCtor);

FunctionCtor::FunctionCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
    vtbl = &static_vtbl;
}

// 15.3.2
Value FunctionCtor::construct(Managed *that, ExecutionContext *ctx, Value *args, int argc)
{
    FunctionCtor *f = static_cast<FunctionCtor *>(that);
    MemoryManager::GCBlocker gcBlocker(ctx->engine->memoryManager);

    QString arguments;
    QString body;
    if (argc > 0) {
        for (uint i = 0; i < argc - 1; ++i) {
            if (i)
                arguments += QLatin1String(", ");
            arguments += args[i].toString(ctx)->toQString();
        }
        body = args[argc - 1].toString(ctx)->toQString();
    }

    QString function = QLatin1String("function(") + arguments + QLatin1String("){") + body + QLatin1String("}");

    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(function, 1, false);
    Parser parser(engine);

    const bool parsed = parser.parseExpression();

    if (!parsed)
        ctx->throwSyntaxError(0);

    using namespace AST;
    FunctionExpression *fe = AST::cast<FunctionExpression *>(parser.rootNode());
    if (!fe)
        ctx->throwSyntaxError(0);

    V4IR::Module module;

    Codegen cg(ctx, f->strictMode);
    V4IR::Function *irf = cg(QString(), function, fe, &module);

    QScopedPointer<EvalInstructionSelection> isel(ctx->engine->iselFactory->create(ctx->engine, &module));
    VM::Function *vmf = isel->vmFunction(irf);

    return Value::fromObject(ctx->engine->newScriptFunction(ctx->engine->rootContext, vmf));
}

// 15.3.1: This is equivalent to new Function(...)
Value FunctionCtor::call(Managed *that, ExecutionContext *context, const Value &thisObject, Value *args, int argc)
{
    return construct(that, context, args, argc);
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
    Value thisArg = ctx->argument(0);

    Value arg = ctx->argument(1);
    QVector<Value> args;

    if (Object *arr = arg.asObject()) {
        quint32 len = arr->get(ctx, ctx->engine->id_length).toUInt32();
        for (quint32 i = 0; i < len; ++i) {
            Value a = arr->getIndexed(ctx, i);
            args.append(a);
        }
    } else if (!(arg.isUndefined() || arg.isNull())) {
        ctx->throwTypeError();
        return Value::undefinedValue();
    }

    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    return o->call(ctx, thisArg, args.data(), args.size());
}

Value FunctionPrototype::method_call(SimpleCallContext *ctx)
{
    Value thisArg = ctx->argument(0);

    QVector<Value> args(ctx->argumentCount ? ctx->argumentCount - 1 : 0);
    if (ctx->argumentCount)
        qCopy(ctx->arguments + 1,
              ctx->arguments + ctx->argumentCount, args.begin());

    FunctionObject *o = ctx->thisObject.asFunctionObject();
    if (!o)
        ctx->throwTypeError();

    return o->call(ctx, thisArg, args.data(), args.size());
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
    : FunctionObject(scope)
{
    vtbl = &static_vtbl;
    this->function = function;
    assert(function);
    assert(function->code);

    // global function
    if (!scope)
        return;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    name = function->name;
    needsActivation = function->needsActivation();
    usesArgumentsObject = function->usesArgumentsObject;
    strictMode = function->isStrict;
    formalParameterCount = function->formals.size();
    formalParameterList = function->formals.constData();
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(formalParameterCount));

    varCount = function->locals.size();
    varList = function->locals.constData();

    Object *proto = scope->engine->newObject();
    proto->defineDefaultProperty(scope->engine->id_constructor, Value::fromObject(this));
    Property *pd = insertMember(scope->engine->id_prototype, Attr_NotEnumerable|Attr_NotConfigurable);
    pd->value = Value::fromObject(proto);

    if (scope->strictMode) {
        FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, throwTypeError);
        Property pd = Property::fromAccessor(thrower, thrower);
        __defineOwnProperty__(scope, QStringLiteral("caller"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
        __defineOwnProperty__(scope, QStringLiteral("arguments"), pd, Attr_Accessor|Attr_NotEnumerable|Attr_NotConfigurable);
    }
}

Value ScriptFunction::construct(Managed *that, ExecutionContext *context, Value *args, int argc)
{
    ScriptFunction *f = static_cast<ScriptFunction *>(that);
    assert(f->function->code);
    Object *obj = context->engine->newObject();
    Value proto = f->get(context, context->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();

    quintptr stackSpace[stackContextSize/sizeof(quintptr)];
    ExecutionContext *ctx = context->engine->newCallContext(stackSpace, f, Value::fromObject(obj), args, argc);

    Value result;
    try {
        result = f->function->code(ctx, f->function->codeData);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }
    ctx->engine->popContext();

    if (result.isObject())
        return result;
    return Value::fromObject(obj);
}

Value ScriptFunction::call(Managed *that, ExecutionContext *context, const Value &thisObject, Value *args, int argc)
{
    ScriptFunction *f = static_cast<ScriptFunction *>(that);
    assert(f->function->code);
    quintptr stackSpace[stackContextSize/sizeof(quintptr)];
    ExecutionContext *ctx = context->engine->newCallContext(stackSpace, f, thisObject, args, argc);

    if (!f->strictMode && !thisObject.isObject()) {
        if (thisObject.isUndefined() || thisObject.isNull()) {
            ctx->thisObject = Value::fromObject(context->engine->globalObject);
        } else {
            ctx->thisObject = Value::fromObject(thisObject.toObject(context));
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
    return result;
}



DEFINE_MANAGED_VTABLE(BuiltinFunctionOld);

BuiltinFunctionOld::BuiltinFunctionOld(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *))
    : FunctionObject(scope)
    , code(code)
{
    vtbl = &static_vtbl;
    this->name = name;
    isBuiltinFunction = true;
}

Value BuiltinFunctionOld::construct(Managed *, ExecutionContext *ctx, Value *, int)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

Value BuiltinFunctionOld::call(Managed *that, ExecutionContext *context, const Value &thisObject, Value *args, int argc)
{
    BuiltinFunctionOld *f = static_cast<BuiltinFunctionOld *>(that);
    SimpleCallContext ctx;
    ctx.type = ExecutionContext::Type_SimpleCallContext;
    ctx.strictMode = f->scope->strictMode; // ### needed? scope or parent context?
    ctx.marked = false;
    ctx.thisObject = thisObject;
    ctx.engine = f->scope->engine;
    ctx.arguments = args;
    ctx.argumentCount = argc;
    context->engine->pushContext(&ctx);

    if (!f->strictMode && !thisObject.isObject()) {
        // Built-in functions allow for the this object to be null or undefined. This overrides
        // the behaviour of changing thisObject to the global object if null/undefined and allows
        // the built-in functions for example to throw a type error if null is passed.
        if (!thisObject.isUndefined() && !thisObject.isNull())
            ctx.thisObject = Value::fromObject(thisObject.toObject(context));
    }

    Value result = Value::undefinedValue();
    try {
        result = f->code(&ctx);
    } catch (Exception &ex) {
        ex.partiallyUnwindContext(context);
        throw;
    }

    context->engine->popContext();
    return result;
}


DEFINE_MANAGED_VTABLE(BoundFunction);

BoundFunction::BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs)
    : FunctionObject(scope)
    , target(target)
    , boundThis(boundThis)
    , boundArgs(boundArgs)
{
    vtbl = &static_vtbl;
    int len = target->get(scope, scope->engine->id_length).toUInt32();
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

Value BoundFunction::call(Managed *that, ExecutionContext *context, const Value &, Value *args, int argc)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(f->boundArgs.size() + argc)));
    memcpy(newArgs, f->boundArgs.constData(), f->boundArgs.size()*sizeof(Value));
    memcpy(newArgs + f->boundArgs.size(), args, argc*sizeof(Value));

    return f->target->call(context, f->boundThis, newArgs, f->boundArgs.size() + argc);
}

Value BoundFunction::construct(Managed *that, ExecutionContext *context, Value *args, int argc)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(f->boundArgs.size() + argc)));
    memcpy(newArgs, f->boundArgs.constData(), f->boundArgs.size()*sizeof(Value));
    memcpy(newArgs + f->boundArgs.size(), args, argc*sizeof(Value));

    return f->target->construct(context, newArgs, f->boundArgs.size() + argc);
}

bool BoundFunction::hasInstance(Managed *that, ExecutionContext *ctx, const Value &value)
{
    BoundFunction *f = static_cast<BoundFunction *>(that);
    return FunctionObject::hasInstance(f->target, ctx, value);
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
