// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4object_p.h"
#include "qv4function_p.h"
#include "qv4symbol_p.h"
#include <private/qv4mm_p.h>

#include "qv4scopedvalue_p.h"
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
#include <private/qv4jscall_p.h>
#include <private/qv4vme_moth_p.h>
#include <private/qv4alloca_p.h>

#include <QtCore/QDebug>
#include <algorithm>

using namespace QV4;


DEFINE_OBJECT_VTABLE(FunctionObject);

void Heap::FunctionObject::init(QV4::ExecutionEngine *engine, QV4::String *name)
{
    Object::init();
    Scope s(engine);
    ScopedFunctionObject f(s, this);
    if (name)
        f->setName(name);
}

void Heap::FunctionObject::init(QV4::ExecutionEngine *engine, const QString &name)
{
    Scope valueScope(engine);
    ScopedString s(valueScope, engine->newString(name));
    init(engine, s);
}

void Heap::FunctionObject::init()
{
    init(internalClass->engine, static_cast<QV4::String *>(nullptr));
}

void Heap::JavaScriptFunctionObject::init(
        QV4::ExecutionContext *scope, Function *function, QV4::String *n)
{
    Q_ASSERT(n || function);
    Scope s(scope->engine());
    ScopedString name(s, n ? n->d() : function->name());
    FunctionObject::init(s.engine, name);
    this->scope.set(s.engine, scope->d());
    setFunction(function);
}

void Heap::JavaScriptFunctionObject::setFunction(Function *f)
{
    if (f) {
        function = f;
        function->executableCompilationUnit()->addref();
    }
}
void Heap::JavaScriptFunctionObject::destroy()
{
    if (function)
        function->executableCompilationUnit()->release();
    FunctionObject::destroy();
}

void Heap::DynamicFunctionObject::init(
        QV4::ExecutionEngine *engine, QV4::String *name, VTable::Call call)
{
    FunctionObject::init(engine, name);
    jsCall = call;
}

void FunctionObject::createDefaultPrototypeProperty(uint protoConstructorSlot)
{
    Scope s(this);

    Q_ASSERT(s.engine->internalClasses(EngineBase::Class_ObjectProto)->verifyIndex(s.engine->id_constructor()->propertyKey(), protoConstructorSlot));

    ScopedObject proto(s, s.engine->newObject(s.engine->internalClasses(EngineBase::Class_ObjectProto)));
    proto->setProperty(protoConstructorSlot, d());
    defineDefaultProperty(s.engine->id_prototype(), proto, Attr_NotEnumerable|Attr_NotConfigurable);
}

ReturnedValue FunctionObject::name() const
{
    return get(engine()->id_name());
}

ReturnedValue FunctionObject::failCall() const
{
    return engine()->throwTypeError(QStringLiteral("Function can only be called with |new|."));
}

ReturnedValue FunctionObject::failCallAsConstructor() const
{
    return engine()->throwTypeError(QStringLiteral("Function is not a constructor."));
}

void FunctionObject::virtualConvertAndCall(
        const FunctionObject *f, QObject *thisObject,
        void **argv, const QMetaType *types, int argc)
{
    QV4::convertAndCall(f->engine(), thisObject, argv, types, argc,
                        [f](const Value *thisObject, const Value *argv, int argc) {
        return f->call(thisObject, argv, argc);
    });
}

Heap::FunctionObject *FunctionObject::createScriptFunction(ExecutionContext *scope, Function *function)
{
    if (function->isArrowFunction())
        return scope->engine()->memoryManager->allocate<ArrowFunction>(scope, function);
    return scope->engine()->memoryManager->allocate<ScriptFunction>(scope, function);
}

Heap::FunctionObject *FunctionObject::createConstructorFunction(
        ExecutionContext *scope, Function *function, Object *homeObject, bool isDerivedConstructor)
{
    QV4::ExecutionEngine *engine = scope->engine();
    if (!function) {
        Heap::DefaultClassConstructorFunction *c
                = engine->memoryManager->allocate<DefaultClassConstructorFunction>(scope);
        c->isDerivedConstructor = isDerivedConstructor;
        return c;
    }

    Heap::ConstructorFunction *c
            = engine->memoryManager->allocate<ConstructorFunction>(scope, function);
    c->homeObject.set(engine, homeObject->d());
    c->isDerivedConstructor = isDerivedConstructor;
    return c;
}

Heap::FunctionObject *FunctionObject::createMemberFunction(ExecutionContext *scope, Function *function, Object *homeObject, QV4::String *name)
{
    Heap::MemberFunction *m = scope->engine()->memoryManager->allocate<MemberFunction>(scope, function, name);
    m->homeObject.set(scope->engine(), homeObject->d());
    return m;
}

Heap::FunctionObject *FunctionObject::createBuiltinFunction(ExecutionEngine *engine, StringOrSymbol *nameOrSymbol, VTable::Call code, int argumentCount)
{
    Scope scope(engine);
    ScopedString name(scope, nameOrSymbol);
    if (!name)
        name = engine->newString(QChar::fromLatin1('[') + QStringView{nameOrSymbol->toQString()}.mid(1) + QChar::fromLatin1(']'));

    ScopedFunctionObject function(
            scope, engine->memoryManager->allocate<DynamicFunctionObject>(engine, name, code));
    function->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(argumentCount));
    return function->d();
}

ReturnedValue FunctionObject::getHomeObject() const
{
    const MemberFunction *m = as<MemberFunction>();
    if (m)
        return m->d()->homeObject->asReturnedValue();
    const ConstructorFunction *c = as<ConstructorFunction>();
    if (c)
        return c->d()->homeObject->asReturnedValue();
    return Encode::undefined();
}

DEFINE_OBJECT_VTABLE(JavaScriptFunctionObject);

QQmlSourceLocation JavaScriptFunctionObject::sourceLocation() const
{
    return d()->function->sourceLocation();
}

DEFINE_OBJECT_VTABLE(DynamicFunctionObject);

ReturnedValue DynamicFunctionObject::virtualCall(
        const FunctionObject *f, const Value *thisObject, const Value *argv, int argc) {
    Heap::DynamicFunctionObject *d = static_cast<const DynamicFunctionObject *>(f)->d();
    if (d->jsCall)
        return d->jsCall(f, thisObject, argv, argc);
    return d->internalClass->engine->throwTypeError(
            QStringLiteral("Function can only be called with |new|."));
}

DEFINE_OBJECT_VTABLE(FunctionCtor);

void Heap::FunctionCtor::init(QV4::ExecutionEngine *engine)
{
    Heap::FunctionObject::init(engine, QStringLiteral("Function"));
}

// 15.3.2
QQmlRefPointer<ExecutableCompilationUnit> FunctionCtor::parse(ExecutionEngine *engine, const Value *argv, int argc, Type t)
{
    QString arguments;
    QString body;
    if (argc > 0) {
        for (int i = 0, ei = argc - 1; i < ei; ++i) {
            if (i)
                arguments += QLatin1String(", ");
            arguments += argv[i].toQString();
        }
        body = argv[argc - 1].toQString();
    }
    if (engine->hasException)
        return nullptr;

    QString function = (t == Type_Function ? QLatin1String("function anonymous(") : QLatin1String("function* anonymous(")) + arguments + QLatin1String("\n){") + body + QLatin1String("\n}");

    QQmlJS::Engine ee;
    QQmlJS::Lexer lexer(&ee);
    lexer.setCode(function, 1, false);
    QQmlJS::Parser parser(&ee);

    const bool parsed = parser.parseExpression();

    if (!parsed) {
        engine->throwSyntaxError(QLatin1String("Parse error"));
        return nullptr;
    }

    QQmlJS::AST::FunctionExpression *fe = QQmlJS::AST::cast<QQmlJS::AST::FunctionExpression *>(parser.rootNode());
    if (!fe) {
        engine->throwSyntaxError(QLatin1String("Parse error"));
        return nullptr;
    }

    Compiler::Module module(QString(), QString(), engine->debugger() != nullptr);

    Compiler::JSUnitGenerator jsGenerator(&module);
    RuntimeCodegen cg(engine, &jsGenerator, false);
    cg.generateFromFunctionExpression(function, fe, &module);

    if (engine->hasException)
        return nullptr;

    return engine->insertCompilationUnit(cg.generateCompilationUnit());
}

ReturnedValue FunctionCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    ExecutionEngine *engine = f->engine();

    QQmlRefPointer<ExecutableCompilationUnit> compilationUnit
            = parse(engine, argv, argc, Type_Function);
    if (engine->hasException)
        return Encode::undefined();

    Function *vmf = compilationUnit->rootFunction();
    ExecutionContext *global = engine->scriptContext();
    ReturnedValue o = Encode(FunctionObject::createScriptFunction(global, vmf));

    if (!newTarget)
        return o;
    Scope scope(engine);
    ScopedObject obj(scope, o);
    obj->setProtoFromNewTarget(newTarget);
    return obj->asReturnedValue();
}

// 15.3.1: This is equivalent to new Function(...)
ReturnedValue FunctionCtor::virtualCall(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    return virtualCallAsConstructor(f, argv, argc, f);
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

    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));

    defineReadonlyConfigurableProperty(engine->id_name(), *engine->id_empty());
    defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(0));
    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(engine->id_toString(), method_toString, 0);
    defineDefaultProperty(QStringLiteral("apply"), method_apply, 2);
    defineDefaultProperty(QStringLiteral("call"), method_call, 1);
    defineDefaultProperty(QStringLiteral("bind"), method_bind, 1);
    defineDefaultProperty(engine->symbol_hasInstance(), method_hasInstance, 1, Attr_ReadOnly);
}

ReturnedValue FunctionPrototype::virtualCall(
        const FunctionObject *, const Value *, const Value *, int)
{
    return Encode::undefined();
}

ReturnedValue FunctionPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const FunctionObject *fun = thisObject->as<FunctionObject>();
    if (!fun)
        return v4->throwTypeError();

    const Scope scope(fun->engine());
    const ScopedString scopedFunctionName(scope, fun->name());
    const QString functionName(scopedFunctionName ? scopedFunctionName->toQString() : QString());
    QString functionAsString = QStringLiteral("function");

    // If fun->name() is empty, then there is no function name
    // to append because the function is anonymous.
    if (!functionName.isEmpty())
        functionAsString.append(QLatin1Char(' ') + functionName);

    functionAsString.append(QStringLiteral("() { [native code] }"));

    return Encode(v4->newString(functionAsString));
}

ReturnedValue FunctionPrototype::method_apply(const QV4::FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = b->engine();
    const FunctionObject *f = thisObject->as<FunctionObject>();
    if (!f)
        return v4->throwTypeError();
    thisObject = argc ? argv : nullptr;
    if (argc < 2 || argv[1].isNullOrUndefined())
        return checkedResult(v4, f->call(thisObject, argv, 0));

    Object *arr = argv[1].objectValue();
    if (!arr)
        return v4->throwTypeError();

    Scope scope(v4);
    const int len = v4->safeForAllocLength(arr->getLength());
    CHECK_EXCEPTION();

    Value *arguments = scope.alloc<Scope::Uninitialized>(len);
    if (len) {
        if (ArgumentsObject::isNonStrictArgumentsObject(arr) && !arr->cast<ArgumentsObject>()->fullyCreated()) {
            QV4::ArgumentsObject *a = arr->cast<ArgumentsObject>();
            int l = qMin(len, a->d()->context->argc());
            memcpy(arguments, a->d()->context->args(), l*sizeof(Value));
            for (int i = l; i < len; ++i)
                arguments[i] = Value::undefinedValue();
        } else if (arr->arrayType() == Heap::ArrayData::Simple && !arr->protoHasArray()) {
            auto sad = static_cast<Heap::SimpleArrayData *>(arr->arrayData());
            int alen = sad ? sad->values.size : 0;
            if (alen > len)
                alen = len;
            for (int i = 0; i < alen; ++i)
                arguments[i] = sad->data(i);
            for (int i = alen; i < len; ++i)
                arguments[i] = Value::undefinedValue();
        } else {
            // need to init the arguments array, as the get() calls below can have side effects
            memset(arguments, 0, len*sizeof(Value));
            for (int i = 0; i < len; ++i)
                arguments[i] = arr->get(i);
        }
    }

    return checkedResult(v4, f->call(thisObject, arguments, len));
}

ReturnedValue FunctionPrototype::method_call(const QV4::FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    QV4::ExecutionEngine *v4 = b->engine();
    if (!thisObject->isFunctionObject())
        return v4->throwTypeError();

    const FunctionObject *f = static_cast<const FunctionObject *>(thisObject);

    thisObject = argc ? argv : nullptr;
    if (argc) {
        ++argv;
        --argc;
    }
    return checkedResult(v4, f->call(thisObject, argv, argc));
}

ReturnedValue FunctionPrototype::method_bind(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    ScopedFunctionObject target(scope, thisObject);
    if (!target || target->isBinding())
        return scope.engine->throwTypeError();

    ScopedValue boundThis(scope, argc ? argv[0] : Value::undefinedValue());
    Scoped<MemberData> boundArgs(scope, (Heap::MemberData *)nullptr);

    int nArgs = (argc - 1 >= 0) ? argc - 1 : 0;
    if (target->isBoundFunction()) {
        BoundFunction *bound = static_cast<BoundFunction *>(target.getPointer());
        Scoped<MemberData> oldArgs(scope, bound->boundArgs());
        boundThis = bound->boundThis();
        int oldSize = !oldArgs ? 0 : oldArgs->size();
        if (oldSize + nArgs) {
            boundArgs = MemberData::allocate(scope.engine, oldSize + nArgs);
            boundArgs->d()->values.size = oldSize + nArgs;
            for (uint i = 0; i < static_cast<uint>(oldSize); ++i)
                boundArgs->set(scope.engine, i, oldArgs->data()[i]);
            for (uint i = 0; i < static_cast<uint>(nArgs); ++i)
                boundArgs->set(scope.engine, oldSize + i, argv[i + 1]);
        }
        target = bound->target();
    } else if (nArgs) {
        boundArgs = MemberData::allocate(scope.engine, nArgs);
        boundArgs->d()->values.size = nArgs;
        for (uint i = 0, ei = static_cast<uint>(nArgs); i < ei; ++i)
            boundArgs->set(scope.engine, i, argv[i + 1]);
    }

    if (target->isConstructor()) {
        return scope.engine->memoryManager->allocate<BoundConstructor>(target, boundThis, boundArgs)
                ->asReturnedValue();
    }

    return scope.engine->memoryManager->allocate<BoundFunction>(target, boundThis, boundArgs)
            ->asReturnedValue();
}

ReturnedValue FunctionPrototype::method_hasInstance(const FunctionObject *, const Value *thisObject, const Value *argv, int argc)
{
    if (!argc)
        return Encode(false);
    const Object *o = thisObject->as<Object>();
    if (!o)
        return Encode(false);

    return Object::virtualInstanceOf(o, argv[0]);
}

DEFINE_OBJECT_VTABLE(ScriptFunction);

ReturnedValue ScriptFunction::virtualCallAsConstructor(const FunctionObject *fo, const Value *argv, int argc, const Value *newTarget)
{
    ExecutionEngine *v4 = fo->engine();
    const ScriptFunction *f = static_cast<const ScriptFunction *>(fo);
    Q_ASSERT(newTarget->isFunctionObject());
    const FunctionObject *nt = static_cast<const FunctionObject *>(newTarget);

    Scope scope(v4);
    Scoped<InternalClass> ic(scope);
    if (nt->d() == f->d()) {
        ic = f->classForConstructor();
    } else {
        ScopedObject o(scope, nt->protoProperty());
        ic = scope.engine->internalClasses(EngineBase::Class_Object);
        if (o)
            ic = ic->changePrototype(o->d());
    }
    ScopedValue thisObject(scope, v4->memoryManager->allocObject<Object>(ic));

    JSTypesStackFrame frame;
    frame.init(f->function(), argv, argc);
    frame.setupJSFrame(v4->jsStackTop, *f, f->scope(),
                       thisObject,
                       newTarget ? *newTarget : Value::undefinedValue());

    frame.push(v4);
    v4->jsStackTop += frame.requiredJSStackFrameSize();

    ReturnedValue result = Moth::VME::exec(&frame, v4);

    frame.pop(v4);

    if (Q_UNLIKELY(v4->hasException))
        return Encode::undefined();
    else if (!Value::fromReturnedValue(result).isObject())
        return thisObject->asReturnedValue();
    return result;
}

DEFINE_OBJECT_VTABLE(ArrowFunction);

void ArrowFunction::virtualCallWithMetaTypes(const FunctionObject *fo, QObject *thisObject,
                                             void **a, const QMetaType *types, int argc)
{
    const ArrowFunction *self = static_cast<const ArrowFunction *>(fo);
    Function *function = self->function();
    if (function->kind != Function::AotCompiled) {
        QV4::convertAndCall(fo->engine(), thisObject, a, types, argc,
                            [fo](const Value *thisObject, const Value *argv, int argc) {
            return ArrowFunction::virtualCall(fo, thisObject, argv, argc);
        });
        return;
    }

    QV4::Scope scope(fo->engine());
    QV4::Scoped<ExecutionContext> context(scope, self->scope());
    MetaTypesStackFrame frame;
    frame.init(function, thisObject, context, a, types, argc);
    frame.push(scope.engine);
    Moth::VME::exec(&frame, scope.engine);
    frame.pop(scope.engine);
}

static ReturnedValue qfoDoCall(
        const QV4::JavaScriptFunctionObject *fo, const QV4::Value *thisObject,
        const QV4::Value *argv, int argc)
{
    ExecutionEngine *engine = fo->engine();
    JSTypesStackFrame frame;
    frame.init(fo->function(), argv, argc, true);
    frame.setupJSFrame(engine->jsStackTop, *fo, fo->scope(),
                       thisObject ? *thisObject : Value::undefinedValue());

    frame.push(engine);
    engine->jsStackTop += frame.requiredJSStackFrameSize();

    ReturnedValue result;

    do {
        frame.setPendingTailCall(false);
        result = Moth::VME::exec(&frame, engine);
        frame.setTailCalling(true);
    } while (frame.pendingTailCall());

    frame.pop(engine);

    return result;
}

ReturnedValue ArrowFunction::virtualCall(const QV4::FunctionObject *fo, const Value *thisObject,
                                         const QV4::Value *argv, int argc)
{
    const ArrowFunction *self = static_cast<const ArrowFunction *>(fo);
    Function *function = self->function();
    switch (function->kind) {
    case Function::AotCompiled:
        return QV4::convertAndCall(
                    fo->engine(), &function->aotCompiledFunction, thisObject, argv, argc,
                    [fo](QObject *thisObject, void **a, const QMetaType *types, int argc) {
            ArrowFunction::virtualCallWithMetaTypes(fo, thisObject, a, types, argc);
        });
    case Function::JsTyped:
        return QV4::coerceAndCall(
                fo->engine(), &function->jsTypedFunction, function->compiledFunction, argv, argc,
                [self, thisObject](const Value *argv, int argc) {
            return qfoDoCall(self, thisObject, argv, argc);
        });
    default:
        break;
    }

    return qfoDoCall(self, thisObject, argv, argc);
}

void Heap::ArrowFunction::init(QV4::ExecutionContext *scope, Function *function, QV4::String *n)
{
    Q_ASSERT(function);
    JavaScriptFunctionObject::init(scope, function, n);

    Scope s(scope);
    Q_ASSERT(internalClass && internalClass->verifyIndex(s.engine->id_length()->propertyKey(), Index_Length));
    setProperty(s.engine, Index_Length, Value::fromInt32(int(function->compiledFunction->length)));
}

void Heap::ScriptFunction::init(QV4::ExecutionContext *scope, Function *function)
{
    ArrowFunction::init(scope, function);
    Q_ASSERT(!function->isArrowFunction());

    Scope s(scope);
    ScopedFunctionObject f(s, this);
    f->createDefaultPrototypeProperty(Heap::FunctionObject::Index_ProtoConstructor);
}

void Heap::DefaultClassConstructorFunction::init(QV4::ExecutionContext *scope)
{
    Scope s(scope->engine());
    FunctionObject::init(s.engine, nullptr);
    this->scope.set(s.engine, scope->d());
}

Heap::InternalClass *ScriptFunction::classForConstructor() const
{
    Scope scope(engine());
    ScopedValue o(scope, protoProperty());
    if (d()->cachedClassForConstructor && d()->cachedClassForConstructor->prototype == o->heapObject())
        return d()->cachedClassForConstructor;

    Scoped<InternalClass> ic(scope, engine()->internalClasses(EngineBase::Class_Object));
    ScopedObject p(scope, o);
    if (p)
        ic = ic->changePrototype(p->d());
    d()->cachedClassForConstructor.set(scope.engine, ic->d());

    return ic->d();
}

DEFINE_OBJECT_VTABLE(ConstructorFunction);

ReturnedValue ConstructorFunction::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    const ConstructorFunction *c = static_cast<const ConstructorFunction *>(f);
    if (!c->d()->isDerivedConstructor)
        return ScriptFunction::virtualCallAsConstructor(f, argv, argc, newTarget);

    ExecutionEngine *v4 = f->engine();

    JSTypesStackFrame frame;
    frame.init(c->function(), argv, argc);
    frame.setupJSFrame(v4->jsStackTop, *f, c->scope(),
                       Value::emptyValue(),
                       newTarget ? *newTarget : Value::undefinedValue());

    frame.push(v4);
    v4->jsStackTop += frame.requiredJSStackFrameSize();

    ReturnedValue result = Moth::VME::exec(&frame, v4);
    ReturnedValue thisObject = frame.jsFrame->thisObject.asReturnedValue();

    frame.pop(v4);

    if (Q_UNLIKELY(v4->hasException))
        return Encode::undefined();
    else if (Value::fromReturnedValue(result).isObject())
        return result;
    else if (!Value::fromReturnedValue(result).isUndefined())
        return v4->throwTypeError();
    else if (Value::fromReturnedValue(thisObject).isEmpty()) {
        Scope scope(v4);
        ScopedString s(scope, v4->newString(QStringLiteral("this")));
        return v4->throwReferenceError(s);
    }
    return thisObject;
}

ReturnedValue ConstructorFunction::virtualCall(const FunctionObject *f, const Value *, const Value *, int)
{
    return f->engine()->throwTypeError(QStringLiteral("Cannot call a class constructor without |new|"));
}

DEFINE_OBJECT_VTABLE(MemberFunction);

DEFINE_OBJECT_VTABLE(DefaultClassConstructorFunction);

ReturnedValue DefaultClassConstructorFunction::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    const DefaultClassConstructorFunction *c = static_cast<const DefaultClassConstructorFunction *>(f);
    ExecutionEngine *v4 = f->engine();

    Scope scope(v4);

    if (!c->d()->isDerivedConstructor) {
        ScopedObject proto(scope, static_cast<const Object *>(newTarget)->get(scope.engine->id_prototype()));
        ScopedObject c(scope, scope.engine->newObject());
        c->setPrototypeUnchecked(proto);
        return c->asReturnedValue();
    }

    ScopedFunctionObject super(scope, f->getPrototypeOf());
    Q_ASSERT(super->isFunctionObject());

    JSTypesStackFrame frame;
    frame.init(nullptr, argv, argc);
    frame.setupJSFrame(v4->jsStackTop, *f, c->scope(),
                       Value::undefinedValue(),
                       newTarget ? *newTarget : Value::undefinedValue(), argc, argc);

    frame.push(v4);
    v4->jsStackTop += frame.requiredJSStackFrameSize(argc);

    // Do a super call
    ReturnedValue result = super->callAsConstructor(argv, argc, newTarget);
    ReturnedValue thisObject = frame.jsFrame->thisObject.asReturnedValue();

    frame.pop(v4);

    if (Q_UNLIKELY(v4->hasException))
        return Encode::undefined();
    else if (Value::fromReturnedValue(result).isObject())
        return result;
    else if (!Value::fromReturnedValue(result).isUndefined())
        return v4->throwTypeError();
    else if (Value::fromReturnedValue(thisObject).isEmpty()) {
        Scope scope(v4);
        ScopedString s(scope, v4->newString(QStringLiteral("this")));
        return v4->throwReferenceError(s);
    }

    return thisObject;
}

ReturnedValue DefaultClassConstructorFunction::virtualCall(const FunctionObject *f, const Value *, const Value *, int)
{
    return f->engine()->throwTypeError(QStringLiteral("Cannot call a class constructor without |new|"));
}

DEFINE_OBJECT_VTABLE(IndexedBuiltinFunction);

DEFINE_OBJECT_VTABLE(BoundFunction);

void Heap::BoundFunction::init(
        QV4::FunctionObject *target, const Value &boundThis, QV4::MemberData *boundArgs)
{
    ExecutionEngine *engine = target->engine();
    Scope s(engine);
    ScopedString name(s, engine->newString(QStringLiteral("__bound function__")));
    if (auto *js = target->as<QV4::JavaScriptFunctionObject>()) {
        ScopedContext ctx(s, js->scope());
        JavaScriptFunctionObject::init(ctx, js->function(), name);
    } else {
        Q_ASSERT(name);
        JavaScriptFunctionObject::init(engine->rootContext(), nullptr, name);
    }

    this->target.set(s.engine, target->d());
    this->boundArgs.set(s.engine, boundArgs ? boundArgs->d() : nullptr);
    this->boundThis.set(s.engine, boundThis);

    ScopedObject f(s, this);

    ScopedValue l(s, target->get(engine->id_length()));
    int len = l->toUInt32();
    if (boundArgs)
        len -= boundArgs->size();
    if (len < 0)
        len = 0;
    f->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(len));

    ScopedProperty pd(s);
    pd->value = engine->thrower();
    pd->set = engine->thrower();
    f->insertMember(engine->id_arguments(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    f->insertMember(engine->id_caller(), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
}

ReturnedValue BoundFunction::virtualCall(const FunctionObject *fo, const Value *, const Value *argv, int argc)
{
    QV4::ExecutionEngine *v4 = fo->engine();
    if (v4->hasException)
        return Encode::undefined();

    const BoundFunction *f = static_cast<const BoundFunction *>(fo);
    Scope scope(v4);
    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    ScopedFunctionObject target(scope, f->target());
    JSCallArguments jsCallData(scope, (boundArgs ? boundArgs->size() : 0) + argc);
    *jsCallData.thisObject = f->boundThis();
    Value *argp = jsCallData.args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, argv, argc*sizeof(Value));
    return checkedResult(v4, target->call(jsCallData));
}

DEFINE_OBJECT_VTABLE(BoundConstructor);

ReturnedValue BoundConstructor::virtualCallAsConstructor(
        const FunctionObject *fo, const Value *argv, int argc, const Value *)
{
    const BoundFunction *f = static_cast<const BoundFunction *>(fo);
    Scope scope(f->engine());

    if (scope.hasException())
        return Encode::undefined();

    Scoped<MemberData> boundArgs(scope, f->boundArgs());
    ScopedFunctionObject target(scope, f->target());
    JSCallArguments jsCallData(scope, (boundArgs ? boundArgs->size() : 0) + argc);
    Value *argp = jsCallData.args;
    if (boundArgs) {
        memcpy(argp, boundArgs->data(), boundArgs->size()*sizeof(Value));
        argp += boundArgs->size();
    }
    memcpy(argp, argv, argc*sizeof(Value));
    return target->callAsConstructor(jsCallData);
}
