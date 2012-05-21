
#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"
#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>

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
    put(ctx->engine->identifier(name), value);
}

void Object::setProperty(Context *ctx, const QString &name, void (*code)(Context *), int count)
{
    Q_UNUSED(count);
    setProperty(ctx, name, Value::fromObject(ctx->engine->newNativeFunction(ctx, code)));
}

bool Object::get(String *name, Value *result)
{
    if (Value *prop = getProperty(name)) {
        *result = *prop;
        return true;
    }

    __qmljs_init_undefined(result);
    return false;
}

Value *Object::getOwnProperty(String *name, PropertyAttributes *attributes)
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

Value *Object::getProperty(String *name, PropertyAttributes *attributes)
{
    if (Value *prop = getOwnProperty(name, attributes))
        return prop;
    else if (prototype)
        return prototype->getProperty(name, attributes);
    return 0;
}

void Object::put(String *name, const Value &value, bool flag)
{
    Q_UNUSED(flag);

    if (! members)
        members = new Table();

    members->insert(name, value);
}

bool Object::canPut(String *name)
{
    PropertyAttributes attrs = PropertyAttributes();
    if (getOwnProperty(name, &attrs)) {
        return attrs & WritableAttribute;
    } else if (! prototype) {
        return extensible;
    } else if (prototype->getProperty(name, &attrs)) {
        return attrs & WritableAttribute;
    } else {
        return extensible;
    }
    return true;
}

bool Object::hasProperty(String *name) const
{
    if (members)
        return members->find(name) != 0;

    return false;
}

bool Object::deleteProperty(String *name, bool flag)
{
    Q_UNUSED(flag);

    if (members)
        return members->remove(name);

    return false;
}

Value *ArrayObject::getOwnProperty(String *name, PropertyAttributes *attributes)
{
    if (name->toQString() == QLatin1String("length")) {
        length.numberValue = value.size();
        return &length;
    }

    return Object::getOwnProperty(name, attributes);
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

Value *ArgumentsObject::getProperty(String *name, PropertyAttributes *attributes)
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
    if (Value *prop = Object::getProperty(name, attributes))
        return prop;
    return 0;
}

ExecutionEngine::ExecutionEngine()
{
    rootContext = newContext();
    rootContext->init(this);

    objectPrototype.type = NULL_TYPE;
    stringPrototype.type = NULL_TYPE;
    numberPrototype.type = NULL_TYPE;
    booleanPrototype.type = NULL_TYPE;
    arrayPrototype.type = NULL_TYPE;
    datePrototype.type = NULL_TYPE;
    //functionPrototype.type = NULL_TYPE;

    //
    // set up the global object
    //
    String *prototype = identifier(QLatin1String("prototype"));

    VM::Object *glo = newArgumentsObject(rootContext);
    __qmljs_init_object(&globalObject, glo);
    __qmljs_init_object(&rootContext->activation, glo);

    objectCtor = ObjectCtor::create(this);
    objectCtor.objectValue->get(prototype, &objectPrototype);

    functionCtor = FunctionCtor::create(this);
    functionCtor.objectValue->get(prototype, &functionPrototype);

    stringCtor = StringCtor::create(this);
    numberCtor = NumberCtor::create(this);
    booleanCtor = BooleanCtor::create(this);
    arrayCtor = ArrayCtor::create(this);
    dateCtor = DateCtor::create(this);

    stringCtor.objectValue->get(prototype, &stringPrototype);
    numberCtor.objectValue->get(prototype, &numberPrototype);
    booleanCtor.objectValue->get(prototype, &booleanPrototype);
    arrayCtor.objectValue->get(prototype, &arrayPrototype);
    dateCtor.objectValue->get(prototype, &datePrototype);

    glo->put(identifier(QLatin1String("Object")), objectCtor);
    glo->put(identifier(QLatin1String("String")), stringCtor);
    glo->put(identifier(QLatin1String("Number")), numberCtor);
    glo->put(identifier(QLatin1String("Array")), arrayCtor);
    glo->put(identifier(QLatin1String("Function")), functionCtor);
    glo->put(identifier(QLatin1String("Date")), dateCtor);
    glo->put(identifier(QLatin1String("Math")), Value::fromObject(newMathObject(rootContext)));
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
    return new NativeFunction(scope, code);
}

FunctionObject *ExecutionEngine::newScriptFunction(Context *scope, IR::Function *function)
{
    return new ScriptFunction(scope, function);
}

Object *ExecutionEngine::newObject()
{
    Object *object = new Object();
    if (objectPrototype.isObject())
        object->prototype = objectPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newObjectCtor(Context *ctx)
{
    return new ObjectCtor(ctx);
}

Object *ExecutionEngine::newObjectPrototype(Context *ctx, FunctionObject *proto)
{
    return new ObjectPrototype(ctx, proto);
}

String *ExecutionEngine::newString(const QString &s)
{
    return new String(s);
}

Object *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new StringObject(value);
    if (stringPrototype.isObject())
        object->prototype = stringPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newStringCtor(Context *ctx)
{
    return new StringCtor(ctx);
}

Object *ExecutionEngine::newStringPrototype(Context *ctx, FunctionObject *proto)
{
    Object *stringProto = new StringPrototype(ctx, proto);
    stringProto->prototype = objectPrototype.objectValue;
    return stringProto;
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new NumberObject(value);
    if (numberPrototype.isObject())
        object->prototype = numberPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newNumberCtor(Context *ctx)
{
    return new NumberCtor(ctx);
}

Object *ExecutionEngine::newNumberPrototype(Context *ctx, FunctionObject *proto)
{
    Object *numberProto = new NumberPrototype(ctx, proto);
    numberProto->prototype = objectPrototype.objectValue;
    return numberProto;
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new BooleanObject(value);
    if (booleanPrototype.isObject())
        object->prototype = booleanPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newBooleanCtor(Context *ctx)
{
    return new BooleanCtor(ctx);
}

Object *ExecutionEngine::newBooleanPrototype(Context *ctx, FunctionObject *proto)
{
    Object *booleanProto = new BooleanPrototype(ctx, proto);
    booleanProto->prototype = objectPrototype.objectValue;
    return booleanProto;
}

Object *ExecutionEngine::newFunctionObject(Context *ctx)
{
    Object *object = new FunctionObject(ctx);
    if (functionPrototype.isObject())
        object->prototype = functionPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newFunctionCtor(Context *ctx)
{
    return new FunctionCtor(ctx);
}

Object *ExecutionEngine::newFunctionPrototype(Context *ctx, FunctionObject *proto)
{
    Object *functionProto = new FunctionPrototype(ctx, proto);
    functionProto->prototype = objectPrototype.objectValue;
    return functionProto;
}

Object *ExecutionEngine::newArrayObject()
{
    ArrayObject *object = new ArrayObject();
    if (arrayPrototype.isObject())
        object->prototype = arrayPrototype.objectValue;
    return object;
}

Object *ExecutionEngine::newArrayObject(const Array &value)
{
    ArrayObject *object = new ArrayObject(value);
    if (arrayPrototype.isObject())
        object->prototype = arrayPrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newArrayCtor(Context *ctx)
{
    return new ArrayCtor(ctx);
}

Object *ExecutionEngine::newArrayPrototype(Context *ctx, FunctionObject *proto)
{
    Object *arrayProto = new ArrayPrototype(ctx, proto);
    arrayProto->prototype = objectPrototype.objectValue;
    return arrayProto;
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new DateObject(value);
    if (datePrototype.isObject())
        object->prototype = datePrototype.objectValue;
    return object;
}

FunctionObject *ExecutionEngine::newDateCtor(Context *ctx)
{
    return new DateCtor(ctx);
}

Object *ExecutionEngine::newDatePrototype(Context *ctx, FunctionObject *proto)
{
    Object *dateProto = new DatePrototype(ctx, proto);
    dateProto->prototype = objectPrototype.objectValue;
    return dateProto;
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    return new ErrorObject(value);
}

Object *ExecutionEngine::newMathObject(Context *ctx)
{
    return new MathObject(ctx);
}

Object *ExecutionEngine::newArgumentsObject(Context *ctx)
{
    return new ArgumentsObject(ctx);
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

    Value proto;
    if (f->get(engine->identifier(QLatin1String("prototype")), &proto)) {
        if (proto.type == OBJECT_TYPE)
            thisObject.objectValue->prototype = proto.objectValue;
    }
    leaveCallContext(f, returnValue);
}
