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

#include "qv4object_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4function_p.h"
#include <private/qv4mm_p.h>

#include "qv4arrayobject_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4jscall_p.h"
#include "qv4argumentsobject_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qqmlengine_p.h>
#include <qv4runtimecodegen_p.h>
#include "private/qlocale_tools_p.h"
#include "private/qqmlbuiltinfunctions_p.h"

#include <QtCore/QDebug>
#include <algorithm>
#include "qv4alloca_p.h"
#include "qv4profiling_p.h"

using namespace QV4;


DEFINE_OBJECT_VTABLE(FunctionObject);

Q_STATIC_ASSERT((Heap::FunctionObject::markTable & Heap::Object::markTable) == Heap::Object::markTable);

void Heap::FunctionObject::init(QV4::ExecutionContext *scope, QV4::String *name, bool createProto)
{
    Object::init();
    function = nullptr;
    this->scope.set(scope->engine(), scope->d());
    Scope s(scope->engine());
    ScopedFunctionObject f(s, this);
    f->init(name, createProto);
}

void Heap::FunctionObject::init(QV4::ExecutionContext *scope, Function *function, bool createProto)
{
    Object::init();
    this->function = function;
    function->compilationUnit->addref();
    this->scope.set(scope->engine(), scope->d());
    Scope s(scope->engine());
    ScopedString name(s, function->name());
    ScopedFunctionObject f(s, this);
    f->init(name, createProto);
}

void Heap::FunctionObject::init(QV4::ExecutionContext *scope, const QString &name, bool createProto)
{
    Scope valueScope(scope);
    ScopedString s(valueScope, valueScope.engine->newString(name));
    init(scope, s, createProto);
}

void Heap::FunctionObject::init()
{
    Object::init();
    function = nullptr;
    this->scope.set(internalClass->engine, internalClass->engine->rootContext()->d());
    Q_ASSERT(internalClass && internalClass->find(internalClass->engine->id_prototype()) == Index_Prototype);
    setProperty(internalClass->engine, Index_Prototype, Primitive::undefinedValue());
}


void Heap::FunctionObject::destroy()
{
    if (function)
        function->compilationUnit->release();
    Object::destroy();
}

void FunctionObject::init(String *n, bool createProto)
{
    Scope s(internalClass()->engine);
    ScopedValue protectThis(s, this);

    Q_ASSERT(internalClass() && internalClass()->find(s.engine->id_prototype()) == Heap::FunctionObject::Index_Prototype);
    if (createProto) {
        ScopedObject proto(s, s.engine->newObject(s.engine->internalClasses[EngineBase::Class_ObjectProto], s.engine->objectPrototype()));
        Q_ASSERT(s.engine->internalClasses[EngineBase::Class_ObjectProto]->find(s.engine->id_constructor()) == Heap::FunctionObject::Index_ProtoConstructor);
        proto->setProperty(Heap::FunctionObject::Index_ProtoConstructor, d());
        setProperty(Heap::FunctionObject::Index_Prototype, proto);
    } else {
        setProperty(Heap::FunctionObject::Index_Prototype, Primitive::undefinedValue());
    }

    if (n)
        defineReadonlyConfigurableProperty(s.engine->id_name(), *n);
}

ReturnedValue FunctionObject::name() const
{
    return get(scope()->internalClass->engine->id_name());
}

ReturnedValue FunctionObject::construct(const Managed *that, CallData *)
{
    return that->engine()->throwTypeError();
}

ReturnedValue FunctionObject::call(const Managed *, CallData *)
{
    return Encode::undefined();
}

Heap::FunctionObject *FunctionObject::createScriptFunction(ExecutionContext *scope, Function *function)
{
    return scope->engine()->memoryManager->allocObject<ScriptFunction>(scope, function);
}

bool FunctionObject::isBinding() const
{
    return d()->vtable() == QQmlBindingFunction::staticVTable();
}

bool FunctionObject::isBoundFunction() const
{
    return d()->vtable() == BoundFunction::staticVTable();
}

QQmlSourceLocation FunctionObject::sourceLocation() const
{
    return d()->function->sourceLocation();
}

DEFINE_OBJECT_VTABLE(FunctionCtor);

void Heap::FunctionCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("Function"));
}

// 15.3.2
ReturnedValue FunctionCtor::construct(const Managed *that, CallData *callData)
{
    Scope scope(that->engine());
    Scoped<FunctionCtor> f(scope, static_cast<const FunctionCtor *>(that));

    QString arguments;
    QString body;
    if (callData->argc() > 0) {
        for (int i = 0, ei = callData->argc() - 1; i < ei; ++i) {
            if (i)
                arguments += QLatin1String(", ");
            arguments += callData->args[i].toQString();
        }
        body = callData->args[callData->argc() - 1].toQString();
    }
    if (scope.engine->hasException)
        return Encode::undefined();

    QString function = QLatin1String("function(") + arguments + QLatin1String("){") + body + QLatin1Char('}');

    QQmlJS::Engine ee, *engine = &ee;
    QQmlJS::Lexer lexer(engine);
    lexer.setCode(function, 1, false);
    QQmlJS::Parser parser(engine);

    const bool parsed = parser.parseExpression();

    if (!parsed)
        return scope.engine->throwSyntaxError(QLatin1String("Parse error"));

    QQmlJS::AST::FunctionExpression *fe = QQmlJS::AST::cast<QQmlJS::AST::FunctionExpression *>(parser.rootNode());
    if (!fe)
        return scope.engine->throwSyntaxError(QLatin1String("Parse error"));

    Compiler::Module module(scope.engine->debugger() != 0);

    Compiler::JSUnitGenerator jsGenerator(&module);
    RuntimeCodegen cg(scope.engine, &jsGenerator, false);
    cg.generateFromFunctionExpression(QString(), function, fe, &module);

    QQmlRefPointer<CompiledData::CompilationUnit> compilationUnit = cg.generateCompilationUnit();
    Function *vmf = compilationUnit->linkToEngine(scope.engine);

    ExecutionContext *global = scope.engine->rootContext();
    return Encode(FunctionObject::createScriptFunction(global, vmf));
}

// 15.3.1: This is equivalent to new Function(...)
ReturnedValue FunctionCtor::call(const Managed *that, CallData *callData)
{
    return construct(that, callData);
}

DEFINE_OBJECT_VTABLE(FunctionPrototype);

void Heap::FunctionPrototype::init()
{
    Heap::FunctionObject::init();
}

void FunctionPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);

    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Primitive::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));

    defineReadonlyConfigurableProperty(engine->id_length(), Primitive::fromInt32(0));
    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString(), method_toString, 0);
    defineDefaultProperty(QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(QStringLiteral("bind"), method_bind, 1);

}

ReturnedValue FunctionPrototype::method_toString(const BuiltinFunction *b, CallData *callData)
{
    ExecutionEngine *v4 = b->engine();
    FunctionObject *fun = callData->thisObject.as<FunctionObject>();
    if (!fun)
        return v4->throwTypeError();

    return Encode(v4->newString(QStringLiteral("function() { [code] }")));
}

ReturnedValue FunctionPrototype::method_apply(const BuiltinFunction *b, CallData *callData)
{
    ExecutionEngine *v4 = b->engine();
    Scope scope(v4);
    FunctionObject *o = callData->thisObject.as<FunctionObject>();
    if (!o)
        return v4->throwTypeError();

    ScopedValue arg(scope, callData->argument(1));

    ScopedObject arr(scope, arg);

    quint32 len;
    if (!arr) {
        len = 0;
        if (!arg->isNullOrUndefined())
            return v4->throwTypeError();
    } else {
        len = arr->getLength();
    }

    JSCall jsCall(scope, o, len);

    if (len) {
        if (ArgumentsObject::isNonStrictArgumentsObject(arr) && !arr->cast<ArgumentsObject>()->fullyCreated()) {
            QV4::ArgumentsObject *a = arr->cast<ArgumentsObject>();
            int l = qMin(len, (uint)a->d()->context->argc());
            memcpy(jsCall->args, a->d()->context->args(), l*sizeof(Value));
            for (quint32 i = l; i < len; ++i)
                jsCall->args[i] = Primitive::undefinedValue();
        } else if (arr->arrayType() == Heap::ArrayData::Simple && !arr->protoHasArray()) {
            auto sad = static_cast<Heap::SimpleArrayData *>(arr->arrayData());
            uint alen = sad ? sad->values.size : 0;
            if (alen > len)
                alen = len;
            for (uint i = 0; i < alen; ++i)
                jsCall->args[i] = sad->data(i);
            for (quint32 i = alen; i < len; ++i)
                jsCall->args[i] = Primitive::undefinedValue();
        } else {
            for (quint32 i = 0; i < len; ++i)
                jsCall->args[i] = arr->getIndexed(i);
        }
    }

    jsCall->thisObject = callData->argument(0);
    return jsCall.call();
}

ReturnedValue FunctionPrototype::method_call(const BuiltinFunction *b, CallData *callData)
{
    if (!callData->thisObject.isFunctionObject()) {
        ExecutionEngine *e = b->engine();
        return e->throwTypeError();
    }
    callData->function = callData->thisObject;
    callData->thisObject = callData->argc() ? callData->args[0] : Primitive::undefinedValue();
    if (callData->argc()) {
        callData->setArgc(callData->argc() - 1);
        for (int i = 0, ei = callData->argc(); i < ei; ++i)
            callData->args[i] = callData->args[i + 1];
    }
    return static_cast<FunctionObject &>(callData->function).call(callData);
}

ReturnedValue FunctionPrototype::method_bind(const BuiltinFunction *b, CallData *callData)
{
    QV4::Scope scope(b);
    FunctionObject *target = callData->thisObject.as<FunctionObject>();
    if (!target)
        return scope.engine->throwTypeError();

    ScopedValue boundThis(scope, callData->argument(0));
    Scoped<MemberData> boundArgs(scope, (Heap::MemberData *)0);
    if (callData->argc() > 1) {
        boundArgs = MemberData::allocate(scope.engine, callData->argc() - 1);
        boundArgs->d()->values.size = callData->argc() - 1;
        for (uint i = 0, ei = static_cast<uint>(callData->argc() - 1); i < ei; ++i)
            boundArgs->set(scope.engine, i, callData->args[i + 1]);
    }

    ExecutionContext *global = scope.engine->rootContext();
    return BoundFunction::create(global, target, boundThis, boundArgs)->asReturnedValue();
}

DEFINE_OBJECT_VTABLE(ScriptFunction);

ReturnedValue ScriptFunction::construct(const Managed *that, CallData *callData)
{
    ExecutionEngine *v4 = that->engine();
    const ScriptFunction *f = static_cast<const ScriptFunction *>(that);

    InternalClass *ic = f->classForConstructor();
    callData->thisObject = v4->memoryManager->allocObject<Object>(ic);

    QV4::Function *v4Function = f->function();
    Q_ASSERT(v4Function);

    callData->context = f->scope();
    ReturnedValue result = v4Function->call(callData);

    if (Q_UNLIKELY(v4->hasException))
        return Encode::undefined();
    else if (!Value::fromReturnedValue(result).isObject())
        return callData->thisObject.asReturnedValue();
    return result;
}

ReturnedValue ScriptFunction::call(const Managed *that, CallData *callData)
{
    const ScriptFunction *f = static_cast<const ScriptFunction *>(that);
    QV4::Function *v4Function = f->function();
    Q_ASSERT(v4Function);

    callData->context = f->scope();
    return v4Function->call(callData);
}

void Heap::ScriptFunction::init(QV4::ExecutionContext *scope, Function *function)
{
    FunctionObject::init();
    this->scope.set(scope->engine(), scope->d());

    this->function = function;
    function->compilationUnit->addref();
    Q_ASSERT(function);
    Q_ASSERT(function->code);

    Scope s(scope);
    ScopedFunctionObject f(s, this);

    ScopedString name(s, function->name());
    f->init(name, true);
    Q_ASSERT(internalClass && internalClass->find(s.engine->id_length()) == Index_Length);
    setProperty(s.engine, Index_Length, Primitive::fromInt32(f->formalParameterCount()));

    if (function->isStrict()) {
        ScopedProperty pd(s);
        pd->value = s.engine->thrower();
        pd->set = s.engine->thrower();
        f->insertMember(s.engine->id_caller(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        f->insertMember(s.engine->id_arguments(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    }
}

InternalClass *ScriptFunction::classForConstructor() const
{
    const Object *o = d()->protoProperty();
    InternalClass *ic = d()->cachedClassForConstructor;
    if (ic && ic->prototype == o->d())
        return ic;

    ic = engine()->internalClasses[EngineBase::Class_Object];
    if (o)
        ic = ic->changePrototype(o->d());
    d()->cachedClassForConstructor = ic;

    return ic;
}

DEFINE_OBJECT_VTABLE(BuiltinFunction);

void Heap::BuiltinFunction::init(QV4::ExecutionContext *scope, QV4::String *name, ReturnedValue (*code)(const QV4::BuiltinFunction *, CallData *))
{
    Heap::FunctionObject::init(scope, name);
    this->code = code;
}

ReturnedValue BuiltinFunction::construct(const Managed *f, CallData *)
{
    return f->engine()->throwTypeError();
}

ReturnedValue BuiltinFunction::call(const Managed *that, CallData *callData)
{
    const BuiltinFunction *f = static_cast<const BuiltinFunction *>(that);
    ExecutionEngine *v4 = f->engine();
    if (v4->hasException)
        return Encode::undefined();

    return f->d()->code(f, callData);
}

DEFINE_OBJECT_VTABLE(IndexedBuiltinFunction);

DEFINE_OBJECT_VTABLE(BoundFunction);

void Heap::BoundFunction::init(QV4::ExecutionContext *scope, QV4::FunctionObject *target,
                               const Value &boundThis, QV4::MemberData *boundArgs)
{
    Scope s(scope);
    Heap::FunctionObject::init(scope, QStringLiteral("__bound function__"));
    this->target.set(s.engine, target->d());
    this->boundArgs.set(s.engine, boundArgs ? boundArgs->d() : 0);
    this->boundThis.set(scope->engine(), boundThis);

    ScopedObject f(s, this);

    ScopedValue l(s, target->get(s.engine->id_length()));
    int len = l->toUInt32();
    if (boundArgs)
        len -= boundArgs->size();
    if (len < 0)
        len = 0;
    f->defineReadonlyConfigurableProperty(s.engine->id_length(), Primitive::fromInt32(len));

    ScopedProperty pd(s);
    pd->value = s.engine->thrower();
    pd->set = s.engine->thrower();
    f->insertMember(s.engine->id_arguments(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    f->insertMember(s.engine->id_caller(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
}

ReturnedValue BoundFunction::call(const Managed *that, CallData *dd)
{
    const BoundFunction *f = static_cast<const BoundFunction *>(that);
    Scope scope(f->engine());

    if (scope.hasException())
        return Encode::undefined();

    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    JSCall jsCall(scope, f->target(), (boundArgs ? boundArgs->size() : 0) + dd->argc());
    jsCall->thisObject = f->boundThis();
    Value *argp = jsCall->args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, dd->args, dd->argc()*sizeof(Value));
    return jsCall.call();
}

ReturnedValue BoundFunction::construct(const Managed *that, CallData *dd)
{
    const BoundFunction *f = static_cast<const BoundFunction *>(that);
    Scope scope(f->engine());

    if (scope.hasException())
        return Encode::undefined();

    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    JSCall jsCall(scope, f->target(), (boundArgs ? boundArgs->size() : 0) + dd->argc());
    Value *argp = jsCall->args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, dd->args, dd->argc()*sizeof(Value));
    return jsCall.callAsConstructor();
}
