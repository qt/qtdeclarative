
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

Value Object::getProperty(Context *ctx, String *name, PropertyAttributes *attributes)
{
    if (name->isEqualTo(ctx->engine->id___proto__))
        return Value::fromObject(prototype);
    else if (Value *v = getPropertyDescriptor(ctx, name, attributes))
        return *v;
    return Value::undefinedValue();
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

bool Object::hasProperty(Context *ctx, String *name) const
{
    if (members)
        return members->find(name) != 0;

    return prototype ? prototype->hasProperty(ctx, name) : false;
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

bool FunctionObject::hasInstance(Context *ctx, const Value &value)
{
    if (! value.isObject()) {
        ctx->throwTypeError();
        return false;
    }

    Value o = getProperty(ctx, ctx->engine->id_prototype);
    if (! o.isObject()) {
        ctx->throwTypeError();
        return false;
    }

    Object *v = value.objectValue;
    while (v) {
        v = v->prototype;

        if (! v)
            break;
        else if (o.objectValue == v)
            return true;
    }

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
    Object *obj = ctx->engine->newObject();
    Value proto = getProperty(ctx, ctx->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue;
    __qmljs_init_object(&ctx->thisObject, obj);
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
    id_constructor = identifier(QStringLiteral("constructor"));
    id___proto__ = identifier(QStringLiteral("__proto__"));

    objectPrototype = new ObjectPrototype();
    stringPrototype = new StringPrototype(rootContext);
    numberPrototype = new NumberPrototype();
    booleanPrototype = new BooleanPrototype();
    arrayPrototype = new ArrayPrototype();
    datePrototype = new DatePrototype();
    functionPrototype = new FunctionPrototype(rootContext);
    regExpPrototype = new RegExpPrototype();

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;
    regExpPrototype->prototype = objectPrototype;

    objectCtor = Value::fromObject(new ObjectCtor(rootContext));
    stringCtor = Value::fromObject(new StringCtor(rootContext));
    numberCtor = Value::fromObject(new NumberCtor(rootContext));
    booleanCtor = Value::fromObject(new BooleanCtor(rootContext));
    arrayCtor = Value::fromObject(new ArrayCtor(rootContext));
    functionCtor = Value::fromObject(new FunctionCtor(rootContext));
    dateCtor = Value::fromObject(new DateCtor(rootContext));
    regExpCtor = Value::fromObject(new RegExpCtor(rootContext));

    stringCtor.objectValue->prototype = stringPrototype;
    numberCtor.objectValue->prototype = numberPrototype;
    booleanCtor.objectValue->prototype = booleanPrototype;
    arrayCtor.objectValue->prototype = arrayPrototype;
    functionCtor.objectValue->prototype = functionPrototype;
    dateCtor.objectValue->prototype = datePrototype;
    regExpCtor.objectValue->prototype = regExpPrototype;

    objectPrototype->init(rootContext, objectCtor);
    stringPrototype->init(rootContext, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);
    regExpPrototype->init(rootContext, regExpCtor);

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
    glo->setProperty(rootContext, identifier(QStringLiteral("RegExp")), regExpCtor);
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
    Object *proto = scope->engine->newObject();
    proto->setProperty(scope, scope->engine->id_constructor, Value::fromObject(f));
    f->setProperty(scope, scope->engine->id_prototype, Value::fromObject(proto));
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

Object *ExecutionEngine::newRegExpObject(const Value &value)
{
    Object *object = new RegExpObject(value);
    object->prototype = regExpPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newRegExpCtor(Context *ctx)
{
    return new RegExpCtor(ctx);
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
    if (varCount)
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
    thisObject.objectValue->prototype = proto.objectValue;
    if (! thisObject.isObject())
        thisObject.objectValue->prototype = engine->objectPrototype;

    leaveCallContext(f, returnValue);
}
