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
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include "qv4mm.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <alloca.h>

using namespace QQmlJS::VM;


Function::~Function()
{
    delete[] codeData;
}

FunctionObject::FunctionObject(ExecutionContext *scope)
    : scope(scope)
    , name(0)
    , formalParameterList(0)
    , varList(0)
    , formalParameterCount(0)
    , varCount(0)
{
    prototype = scope->engine->functionPrototype;

    type = Type_FunctionObject;
    needsActivation = false;
    usesArgumentsObject = false;
    strictMode = false;
}

bool FunctionObject::hasInstance(ExecutionContext *ctx, const Value &value)
{
    if (! value.isObject())
        return false;

    Value o = __get__(ctx, ctx->engine->id_prototype);
    if (! o.isObject()) {
        ctx->throwTypeError();
        return false;
    }

    Object *v = value.objectValue();
    while (v) {
        v = v->prototype;

        if (! v)
            break;
        else if (o.objectValue() == v)
            return true;
    }

    return false;
}

Value FunctionObject::construct(ExecutionContext *context, Value *args, int argc)
{
    Object *obj = context->engine->newObject();
    Value proto = __get__(context, context->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();

    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    ctx->initCallContext(context, Value::fromObject(obj), this, args, argc);
    Value result = construct(ctx);
    ctx->leaveCallContext();

    if (result.isObject())
        return result;
    return Value::fromObject(obj);
}

Value FunctionObject::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    ctx->initCallContext(context, thisObject, this, args, argc);
    if (isBuiltinFunction) {
        // Built-in functions allow for the this object to be null or undefined. This overrides
        // the behaviour of changing thisObject to the global object if null/undefined and allows
        // the built-in functions for example to throw a type error if null is passed.
        if (thisObject.isNull() || thisObject.isUndefined())
            ctx->thisObject = thisObject;
    }
    Value result = call(ctx);
    ctx->leaveCallContext();
    return result;
}

Value FunctionObject::call(ExecutionContext *ctx)
{
    Q_UNUSED(ctx);
    return Value::undefinedValue();
}

Value FunctionObject::construct(ExecutionContext *ctx)
{
    return call(ctx);
}


FunctionCtor::FunctionCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
}

// 15.3.2
Value FunctionCtor::construct(ExecutionContext *ctx)
{
    MemoryManager::GCBlocker gcBlocker(ctx->engine->memoryManager);

    QString args;
    QString body;
    if (ctx->argumentCount > 0) {
        for (uint i = 0; i < ctx->argumentCount - 1; ++i) {
            if (i)
                args += QLatin1String(", ");
            args += ctx->argument(i).toString(ctx)->toQString();
        }
        body = ctx->argument(ctx->argumentCount - 1).toString(ctx)->toQString();
    }

    QString function = QLatin1String("function(") + args + QLatin1String("){") + body + QLatin1String("}");

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

    IR::Module module;

    Codegen cg(ctx, ctx->strictMode);
    IR::Function *irf = cg(QString(), fe, &module);

    QScopedPointer<EvalInstructionSelection> isel(ctx->engine->iselFactory->create(ctx->engine, &module));
    VM::Function *vmf = isel->vmFunction(irf);

    return Value::fromObject(ctx->engine->newScriptFunction(ctx->engine->rootContext, vmf));
}

// 15.3.1: This is equivalent to new Function(...)
Value FunctionCtor::call(ExecutionContext *ctx)
{
    return construct(ctx);
}

void FunctionPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(1));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(ctx, QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(ctx, QStringLiteral("bind"), method_bind, 1);

    defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(0));
}

Value FunctionPrototype::method_toString(ExecutionContext *ctx)
{
    FunctionObject *fun = ctx->thisObject.asFunctionObject();
    if (!fun)
        ctx->throwTypeError();

    return Value::fromString(ctx, QStringLiteral("function() { [code] }"));
}

Value FunctionPrototype::method_apply(ExecutionContext *ctx)
{
    Value thisArg = ctx->argument(0);

    Value arg = ctx->argument(1);
    QVector<Value> args;

    if (Object *arr = arg.asObject()) {
        quint32 len = arr->__get__(ctx, ctx->engine->id_length).toUInt32(ctx);
        for (quint32 i = 0; i < len; ++i) {
            Value a = arr->__get__(ctx, i);
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

Value FunctionPrototype::method_call(ExecutionContext *ctx)
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

Value FunctionPrototype::method_bind(ExecutionContext *ctx)
{
    FunctionObject *target = ctx->thisObject.asFunctionObject();
    if (!target)
        ctx->throwTypeError();

    Value boundThis = ctx->argument(0);
    QVector<Value> boundArgs;
    for (uint i = 1; i < ctx->argumentCount; ++i)
        boundArgs += ctx->argument(i);


    BoundFunction *f = ctx->engine->newBoundFunction(ctx, target, boundThis, boundArgs);
    return Value::fromObject(f);
}

ScriptFunction::ScriptFunction(ExecutionContext *scope, VM::Function *function)
    : FunctionObject(scope)
    , function(function)
{
    assert(function);
    assert(function->code);

    // global function
    if (!scope)
        return;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    if (!function->name.isEmpty())
        name = scope->engine->identifier(function->name);
    needsActivation = function->needsActivation();
    usesArgumentsObject = function->usesArgumentsObject;
    strictMode = function->isStrict;
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (unsigned int i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = scope->engine->identifier(function->formals.at(i));
        }
    }
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(formalParameterCount));

    varCount = function->locals.size();
    if (varCount) {
        varList = new String*[varCount];
        for (unsigned int i = 0; i < varCount; ++i) {
            varList[i] = scope->engine->identifier(function->locals.at(i));
        }
    }

    Object *proto = scope->engine->newObject();
    proto->defineDefaultProperty(scope->engine->id_constructor, Value::fromObject(this));
    PropertyDescriptor *pd = members->insert(scope->engine->id_prototype);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = Value::fromObject(proto);

    if (scope->strictMode) {
        FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, __qmljs_throw_type_error);
        PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
        pd.configurable = PropertyDescriptor::Disabled;
        pd.enumberable = PropertyDescriptor::Disabled;
        __defineOwnProperty__(scope, QStringLiteral("caller"), &pd);
        __defineOwnProperty__(scope, QStringLiteral("arguments"), &pd);
    }
}

ScriptFunction::~ScriptFunction()
{
    delete[] formalParameterList;
    delete[] varList;
}

Value ScriptFunction::call(VM::ExecutionContext *ctx)
{
    assert(function->code);
    return function->code(ctx, function->codeData);
}


BuiltinFunction::BuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *))
    : FunctionObject(scope)
    , code(code)
{
    this->name = name;
    isBuiltinFunction = true;
}

Value BuiltinFunction::construct(ExecutionContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

BoundFunction::BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs)
    : FunctionObject(scope)
    , target(target)
    , boundThis(boundThis)
    , boundArgs(boundArgs)
{
    int len = target->__get__(scope, scope->engine->id_length).toUInt32(scope);
    len -= boundArgs.size();
    if (len < 0)
        len = 0;
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(len));

    FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, __qmljs_throw_type_error);
    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
    pd.configurable = PropertyDescriptor::Disabled;
    pd.enumberable = PropertyDescriptor::Disabled;
    *members->insert(scope->engine->id_arguments) = pd;
    *members->insert(scope->engine->id_caller) = pd;
}

Value BoundFunction::call(ExecutionContext *context, Value, Value *args, int argc)
{
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(boundArgs.size() + argc)));
    memcpy(newArgs, boundArgs.constData(), boundArgs.size()*sizeof(Value));
    memcpy(newArgs + boundArgs.size(), args, argc*sizeof(Value));

    return target->call(context, boundThis, newArgs, boundArgs.size() + argc);
}

Value BoundFunction::construct(ExecutionContext *context, Value *args, int argc)
{
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(boundArgs.size() + argc)));
    memcpy(newArgs, boundArgs.constData(), boundArgs.size()*sizeof(Value));
    memcpy(newArgs + boundArgs.size(), args, argc*sizeof(Value));

    return target->construct(context, newArgs, boundArgs.size() + argc);
}

bool BoundFunction::hasInstance(ExecutionContext *ctx, const Value &value)
{
    return target->hasInstance(ctx, value);
}

void BoundFunction::markObjects()
{
    target->mark();
    if (Object *o = boundThis.asObject())
        o->mark();
    for (int i = 0; i < boundArgs.size(); ++i)
        if (Object *o = boundArgs.at(i).asObject())
            o->mark();
    FunctionObject::markObjects();
}
