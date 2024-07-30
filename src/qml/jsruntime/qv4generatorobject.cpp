// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qv4generatorobject_p.h>
#include <qv4symbol_p.h>
#include <qv4iterator_p.h>
#include <qv4jscall_p.h>
#include <qv4vme_moth_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(GeneratorFunctionCtor);
DEFINE_OBJECT_VTABLE(GeneratorFunction);
DEFINE_OBJECT_VTABLE(GeneratorObject);

void Heap::GeneratorFunctionCtor::init(QV4::ExecutionEngine *engine)
{
    Heap::FunctionObject::init(engine, QStringLiteral("GeneratorFunction"));
}

ReturnedValue GeneratorFunctionCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    ExecutionEngine *engine = f->engine();

    QQmlRefPointer<ExecutableCompilationUnit> compilationUnit = parse(engine, argv, argc, Type_Generator);
    if (engine->hasException)
        return Encode::undefined();

    Function *vmf = compilationUnit->rootFunction();
    ExecutionContext *global = engine->scriptContext();
    ReturnedValue o = Encode(GeneratorFunction::create(global, vmf));

    if (!newTarget)
        return o;
    Scope scope(engine);
    ScopedObject obj(scope, o);
    obj->setProtoFromNewTarget(newTarget);
    return obj->asReturnedValue();
}

// 15.3.1: This is equivalent to new Function(...)
ReturnedValue GeneratorFunctionCtor::virtualCall(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    return virtualCallAsConstructor(f, argv, argc, f);
}

Heap::FunctionObject *GeneratorFunction::create(ExecutionContext *context, Function *function)
{
    Scope scope(context);
    Scoped<GeneratorFunction> g(scope, context->engine()->memoryManager->allocate<GeneratorFunction>(context, function));
    ScopedObject proto(scope, scope.engine->newObject());
    proto->setPrototypeOf(scope.engine->generatorPrototype());
    g->defineDefaultProperty(scope.engine->id_prototype(), proto, Attr_NotConfigurable|Attr_NotEnumerable);
    g->setPrototypeOf(ScopedObject(scope, scope.engine->generatorFunctionCtor()->get(scope.engine->id_prototype())));
    return g->d();
}

ReturnedValue GeneratorFunction::virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    const GeneratorFunction *gf = static_cast<const GeneratorFunction *>(f);
    Function *function = gf->function();
    ExecutionEngine *engine = gf->engine();

    Scope scope(gf);
    Scoped<GeneratorObject> g(scope, engine->memoryManager->allocManaged<GeneratorObject>(engine->classes[EngineBase::Class_GeneratorObject]));
    g->setPrototypeOf(ScopedObject(scope, gf->get(scope.engine->id_prototype())));

    // We need to set up a separate JSFrame for the generator, as it's being re-entered
    Heap::GeneratorObject *gp = g->d();
    gp->values.set(engine, engine->newArrayObject(argc));
    gp->jsFrame.set(engine, engine->newArrayObject(
                        JSTypesStackFrame::requiredJSStackFrameSize(function)));

    // copy original arguments
    for (int i = 0; i < argc; i++)
        gp->values->arrayData->setArrayData(engine, i, argv[i]);

    gp->cppFrame.init(function, gp->values->arrayData->values.values, argc);
    gp->cppFrame.setupJSFrame(gp->jsFrame->arrayData->values.values, *gf, gf->scope(),
                              thisObject ? *thisObject : Value::undefinedValue(),
                              Value::undefinedValue());

    gp->cppFrame.push(engine);

    CHECK_STACK_LIMITS(scope.engine)
    Moth::VME::interpret(&gp->cppFrame, engine, function->codeData);

    gp->state = GeneratorState::SuspendedStart;

    gp->cppFrame.pop(engine);
    return g->asReturnedValue();
}


void Heap::GeneratorPrototype::init()
{
    Heap::FunctionObject::init();
}


void GeneratorPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedValue v(scope);

    Scoped<InternalClass> ic(scope, engine->newInternalClass(
                                            Object::staticVTable(), engine->functionPrototype()));
    ScopedObject ctorProto(scope, engine->newObject(ic->d()));

    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), ctorProto);

    ctorProto->defineDefaultProperty(QStringLiteral("constructor"), (v = ctor), Attr_ReadOnly_ButConfigurable);
    ctorProto->defineDefaultProperty(engine->symbol_toStringTag(), (v = engine->newIdentifier(QStringLiteral("GeneratorFunction"))), Attr_ReadOnly_ButConfigurable);
    ctorProto->defineDefaultProperty(engine->id_prototype(), (v = this), Attr_ReadOnly_ButConfigurable);

    setPrototypeOf(engine->iteratorPrototype());
    defineDefaultProperty(QStringLiteral("constructor"), ctorProto, Attr_ReadOnly_ButConfigurable);
    defineDefaultProperty(QStringLiteral("next"), method_next, 1);
    defineDefaultProperty(QStringLiteral("return"), method_return, 1);
    defineDefaultProperty(QStringLiteral("throw"), method_throw, 1);
    defineDefaultProperty(engine->symbol_toStringTag(), (v = engine->newString(QStringLiteral("Generator"))), Attr_ReadOnly_ButConfigurable);
}

ReturnedValue GeneratorPrototype::method_next(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *engine = f->engine();
    const GeneratorObject *g = thisObject->as<GeneratorObject>();
    if (!g || g->d()->state == GeneratorState::Executing)
        return engine->throwTypeError();
    Heap::GeneratorObject *gp = g->d();

    if (gp->state == GeneratorState::Completed)
        return IteratorPrototype::createIterResultObject(engine, Value::undefinedValue(), true);

    return g->resume(engine, argc ? argv[0] : Value::undefinedValue(), {});
}

ReturnedValue GeneratorPrototype::method_return(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *engine = f->engine();
    const GeneratorObject *g = thisObject->as<GeneratorObject>();
    if (!g || g->d()->state == GeneratorState::Executing)
        return engine->throwTypeError();

    Heap::GeneratorObject *gp = g->d();

    if (gp->state == GeneratorState::SuspendedStart)
        gp->state = GeneratorState::Completed;

    if (gp->state == GeneratorState::Completed)
        return IteratorPrototype::createIterResultObject(engine, argc ? argv[0] : Value::undefinedValue(), true);

    return g->resume(engine, argc ? argv[0] : Value::undefinedValue(), Value::emptyValue());
}

ReturnedValue GeneratorPrototype::method_throw(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *engine = f->engine();
    const GeneratorObject *g = thisObject->as<GeneratorObject>();
    if (!g || g->d()->state == GeneratorState::Executing)
        return engine->throwTypeError();

    Heap::GeneratorObject *gp = g->d();


    if (gp->state == GeneratorState::SuspendedStart || gp->state == GeneratorState::Completed) {
        gp->state = GeneratorState::Completed;
        engine->throwError(argc ? argv[0] : Value::undefinedValue());
        return Encode::undefined();
    }

    return g->resume(engine, Value::undefinedValue(), argc ? argv[0] : Value::undefinedValue());
}

ReturnedValue GeneratorObject::resume(ExecutionEngine *engine, const Value &arg, std::optional<Value> exception) const
{
    Heap::GeneratorObject *gp = d();
    gp->state = GeneratorState::Executing;
    gp->cppFrame.setParentFrame(engine->currentStackFrame);
    engine->currentStackFrame = &gp->cppFrame;

    Q_ASSERT(gp->cppFrame.yield() != nullptr);
    const char *code = gp->cppFrame.yield();
    gp->cppFrame.setYield(nullptr);
    gp->cppFrame.jsFrame->accumulator = arg;
    gp->cppFrame.setYieldIsIterator(false);

    Scope scope(engine);

    CHECK_STACK_LIMITS(scope.engine)

    // A value to be thrown will be passed in by `method_throw` or
    // `method_return` when they need to resume the generator.
    // For `method_throw` this will be the value that was passed to
    // `throw` itself.
    // For `method_return` this will be an `emptyValue`.
    // The empty value will be used as a signal that `return` was
    // called and managed in the execution of a `Resume` instruction
    // during `interpret`.
    if (exception)
        engine->throwError(*exception);
    ScopedValue result(scope, Moth::VME::interpret(&gp->cppFrame, engine, code));

    engine->currentStackFrame = gp->cppFrame.parentFrame();

    bool done = (gp->cppFrame.yield() == nullptr);
    gp->state = done ? GeneratorState::Completed : GeneratorState::SuspendedYield;
    if (engine->hasException)
        return Encode::undefined();
    if (gp->cppFrame.yieldIsIterator())
        return result->asReturnedValue();
    return IteratorPrototype::createIterResultObject(engine, result, done);
}

DEFINE_OBJECT_VTABLE(MemberGeneratorFunction);

Heap::FunctionObject *MemberGeneratorFunction::create(ExecutionContext *context, Function *function, Object *homeObject, String *name)
{
    Scope scope(context);
    Scoped<MemberGeneratorFunction> g(scope, context->engine()->memoryManager->allocate<MemberGeneratorFunction>(context, function, name));
    g->d()->homeObject.set(scope.engine, homeObject->d());
    ScopedObject proto(scope, scope.engine->newObject());
    proto->setPrototypeOf(scope.engine->generatorPrototype());
    g->defineDefaultProperty(scope.engine->id_prototype(), proto, Attr_NotConfigurable|Attr_NotEnumerable);
    g->setPrototypeOf(ScopedObject(scope, scope.engine->generatorFunctionCtor()->get(scope.engine->id_prototype())));
    return g->d();
}

ReturnedValue MemberGeneratorFunction::virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc)
{
    return GeneratorFunction::virtualCall(f, thisObject, argv, argc);
}
