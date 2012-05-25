
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"
#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>

using namespace QQmlJS::VM;

//
// Object
//
Object::~Object()
{
    delete members;
}

void Object::setProperty(Context *ctx, const QString &name, const Value &value)
{
    setProperty(ctx, ctx->engine->identifier(name), value);
}

void Object::setProperty(Context *ctx, const QString &name, void (*code)(Context *), int count)
{
    Q_UNUSED(count);
    setProperty(ctx, name, Value::fromObject(ctx->engine->newNativeFunction(ctx, code)));
}

Value *Object::getOwnProperty(Context *, String *name, PropertyAttributes *attributes)
{
    if (members) {
        if (Property *prop = members->find(name)) {
            if (attributes)
                *attributes = prop->attributes;
            return &prop->value;
        }
    }
    return 0;
}

Value *Object::getPropertyDescriptor(Context *ctx, String *name, PropertyAttributes *attributes)
{
    if (Value *prop = getOwnProperty(ctx, name, attributes))
        return prop;
    else if (prototype)
        return prototype->getPropertyDescriptor(ctx, name, attributes);
    return 0;
}

void Object::setProperty(Context *, String *name, const Value &value, bool flag)
{
    Q_UNUSED(flag);

    if (! members)
        members = new Table();

    members->insert(name, value);
}

bool Object::canSetProperty(Context *ctx, String *name)
{
    PropertyAttributes attrs = PropertyAttributes();
    if (getOwnProperty(ctx, name, &attrs)) {
        return attrs & WritableAttribute;
    } else if (! prototype) {
        return extensible;
    } else if (prototype->getPropertyDescriptor(ctx, name, &attrs)) {
        return attrs & WritableAttribute;
    } else {
        return extensible;
    }
    return true;
}

bool Object::hasProperty(Context *, String *name) const
{
    if (members)
        return members->find(name) != 0;

    return false;
}

bool Object::deleteProperty(Context *, String *name, bool flag)
{
    Q_UNUSED(flag);

    if (members)
        return members->remove(name);

    return false;
}

void Object::defineOwnProperty(Context *ctx, const Value &getter, const Value &setter, bool flag)
{
    Q_UNUSED(getter);
    Q_UNUSED(setter);
    Q_UNUSED(flag);
    ctx->throwUnimplemented(QStringLiteral("defineOwnProperty"));
}

Value ArrayObject::getProperty(Context *ctx, String *name, PropertyAttributes *attributes)
{
    if (name->isEqualTo(ctx->engine->id_length))
        return Value::fromNumber(value.size());
    return Object::getProperty(ctx, name, attributes);
}

bool FunctionObject::hasInstance(const Value &value) const
{
    Q_UNUSED(value);
    return false;
}

void FunctionObject::call(Context *ctx)
{
    Q_UNUSED(ctx);
}

void FunctionObject::construct(Context *ctx)
{
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newObject());
    call(ctx);
}

ScriptFunction::ScriptFunction(Context *scope, IR::Function *function)
    : FunctionObject(scope)
    , function(function)
{
    needsActivation = function->needsActivation();
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (size_t i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = scope->engine->identifier(*function->formals.at(i));
        }
    }

    varCount = function->locals.size();
    if (varCount) {
        varList = new String*[varCount];
        for (size_t i = 0; i < varCount; ++i) {
            varList[i] = scope->engine->identifier(*function->locals.at(i));
        }
    }
}

ScriptFunction::~ScriptFunction()
{
    delete[] formalParameterList;
    delete[] varList;
}

void ScriptFunction::call(VM::Context *ctx)
{
    function->code(ctx);
}

void ScriptFunction::construct(VM::Context *ctx)
{
    __qmljs_init_object(&ctx->thisObject, ctx->engine->newObject());
    function->code(ctx);
}

Value *ArgumentsObject::getPropertyDescriptor(Context *ctx, String *name, PropertyAttributes *attributes)
{
    if (context) {
        for (size_t i = 0; i < context->varCount; ++i) {
            String *var = context->vars[i];
            if (__qmljs_string_equal(context, var, name)) {
                if (attributes)
                    *attributes = PropertyAttributes(*attributes | WritableAttribute);
                return &context->locals[i];
            }
        }
        for (size_t i = 0; i < context->formalCount; ++i) {
            String *formal = context->formals[i];
            if (__qmljs_string_equal(context, formal, name)) {
                if (attributes)
                    *attributes = PropertyAttributes(*attributes | WritableAttribute);
                return &context->arguments[i];
            }
        }
    }
    if (Value *prop = Object::getPropertyDescriptor(ctx, name, attributes))
        return prop;
    return 0;
}

ExecutionEngine::ExecutionEngine()
{
    rootContext = newContext();
    rootContext->init(this);

    id_length = identifier(QStringLiteral("length"));
    id_prototype = identifier(QStringLiteral("prototype"));

    objectPrototype = new ObjectPrototype();
    stringPrototype = new StringPrototype(rootContext);
    numberPrototype = new NumberPrototype();
    booleanPrototype = new BooleanPrototype();
    arrayPrototype = new ArrayPrototype();
    datePrototype = new DatePrototype();
    functionPrototype = new FunctionPrototype(rootContext);

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;

    Value objectCtor = Value::fromObject(new ObjectCtor(rootContext));
    Value stringCtor = Value::fromObject(new StringCtor(rootContext));
    Value numberCtor = Value::fromObject(new NumberCtor(rootContext));
    Value booleanCtor = Value::fromObject(new BooleanCtor(rootContext));
    Value arrayCtor = Value::fromObject(new ArrayCtor(rootContext));
    Value functionCtor = Value::fromObject(new FunctionCtor(rootContext));
    Value dateCtor = Value::fromObject(new DateCtor(rootContext));

    stringCtor.objectValue->prototype = functionPrototype;
    numberCtor.objectValue->prototype = functionPrototype;
    booleanCtor.objectValue->prototype = functionPrototype;
    arrayCtor.objectValue->prototype = functionPrototype;
    functionCtor.objectValue->prototype = functionPrototype;
    dateCtor.objectValue->prototype = functionPrototype;

    objectPrototype->init(rootContext, objectCtor);
    stringPrototype->init(rootContext, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);

    //
    // set up the global object
    //
    VM::Object *glo = newArgumentsObject(rootContext);
    __qmljs_init_object(&globalObject, glo);
    __qmljs_init_object(&rootContext->activation, glo);

    glo->setProperty(rootContext, identifier(QStringLiteral("Object")), objectCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("String")), stringCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Number")), numberCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Boolean")), booleanCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Array")), arrayCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Function")), functionCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Date")), dateCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Math")), Value::fromObject(newMathObject(rootContext)));
}

Context *ExecutionEngine::newContext()
{
    return new Context();
}

String *ExecutionEngine::identifier(const QString &s)
{
    String *&id = identifiers[s];
    if (! id)
        id = newString(s);
    return id;
}

FunctionObject *ExecutionEngine::newNativeFunction(Context *scope, void (*code)(Context *))
{
    NativeFunction *f = new NativeFunction(scope, code);
    f->prototype = scope->engine->functionPrototype;
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(Context *scope, IR::Function *function)
{
    ScriptFunction *f = new ScriptFunction(scope, function);
    f->prototype = scope->engine->functionPrototype;
    return f;
}

Object *ExecutionEngine::newObject()
{
    Object *object = new Object();
    object->prototype = objectPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newObjectCtor(Context *ctx)
{
    return new ObjectCtor(ctx);
}

String *ExecutionEngine::newString(const QString &s)
{
    return new String(s);
}

Object *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new StringObject(value);
    object->prototype = stringPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newStringCtor(Context *ctx)
{
    return new StringCtor(ctx);
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new NumberObject(value);
    object->prototype = numberPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newNumberCtor(Context *ctx)
{
    return new NumberCtor(ctx);
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new BooleanObject(value);
    object->prototype = booleanPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newBooleanCtor(Context *ctx)
{
    return new BooleanCtor(ctx);
}

Object *ExecutionEngine::newFunctionObject(Context *ctx)
{
    Object *object = new FunctionObject(ctx);
    object->prototype = functionPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newFunctionCtor(Context *ctx)
{
    return new FunctionCtor(ctx);
}

Object *ExecutionEngine::newArrayObject()
{
    ArrayObject *object = new ArrayObject();
    object->prototype = arrayPrototype;
    return object;
}

Object *ExecutionEngine::newArrayObject(const Array &value)
{
    ArrayObject *object = new ArrayObject(value);
    object->prototype = arrayPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newArrayCtor(Context *ctx)
{
    return new ArrayCtor(ctx);
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new DateObject(value);
    object->prototype = datePrototype;
    return object;
}

FunctionObject *ExecutionEngine::newDateCtor(Context *ctx)
{
    return new DateCtor(ctx);
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new ErrorObject(value);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newMathObject(Context *ctx)
{
    MathObject *object = new MathObject(ctx);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newArgumentsObject(Context *ctx)
{
    return new ArgumentsObject(ctx);
}

void Context::throwError(const Value &value)
{
    result = value;
    hasUncaughtException = true;
}

void Context::throwError(const QString &message)
{
    Value v = Value::fromString(this, message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwTypeError()
{
    Value v = Value::fromString(this, QStringLiteral("Type error"));
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwUnimplemented(const QString &message)
{
    Value v = Value::fromString(this, QStringLiteral("Unimplemented ") + message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void Context::throwReferenceError(const Value &value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newErrorObject(Value::fromString(this, msg))));
}

void Context::initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc)
{
    engine = e;
    parent = f->scope;

    if (f->needsActivation)
        __qmljs_init_object(&activation, engine->newArgumentsObject(this));
    else
        __qmljs_init_null(&activation);

    if (object)
        thisObject = *object;
    else
        __qmljs_init_null(&thisObject);

    formals = f->formalParameterList;
    formalCount = f->formalParameterCount;
    arguments = args;
    argumentCount = argc;
    if (argc && f->needsActivation) {
        arguments = new Value[argc];
        std::copy(args, args + argc, arguments);
    }
    vars = f->varList;
    varCount = f->varCount;
    locals = varCount ? new Value[varCount] : 0;
    hasUncaughtException = false;
    calledAsConstructor = false;
    std::fill(locals, locals + varCount, Value::undefinedValue());
}

void Context::leaveCallContext(FunctionObject *f, Value *returnValue)
{
    if (returnValue)
        __qmljs_copy(returnValue, &result);

    if (! f->needsActivation) {
        delete[] locals;
        locals = 0;
    }
}

void Context::initConstructorContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc)
{
    initCallContext(e, object, f, args, argc);
    calledAsConstructor = true;
}

void Context::leaveConstructorContext(FunctionObject *f, Value *returnValue)
{
    assert(thisObject.is(OBJECT_TYPE));
    result = thisObject;

    Value proto = f->getProperty(this, engine->id_prototype);
    if (proto.isObject())
        thisObject.objectValue->prototype = proto.objectValue;

    leaveCallContext(f, returnValue);
}
