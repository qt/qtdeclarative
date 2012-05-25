
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
    setProperty(ctx, ctx->engine->identifier(name), value);
}

void Object::setProperty(Context *ctx, const QString &name, void (*code)(Context *), int count)
{
    Q_UNUSED(count);
    setProperty(ctx, name, Value::fromObject(ctx->engine->newNativeFunction(ctx, code)));
}

Value *Object::getOwnProperty(Context *ctx, String *name, PropertyAttributes *attributes)
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

void Object::setProperty(Context *ctx, String *name, const Value &value, bool flag)
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

    return false;
}

bool Object::deleteProperty(Context *ctx, String *name, bool flag)
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

    objectPrototype.type = NULL_TYPE;
    stringPrototype.type = NULL_TYPE;
    numberPrototype.type = NULL_TYPE;
    booleanPrototype.type = NULL_TYPE;
    arrayPrototype.type = NULL_TYPE;
    datePrototype.type = NULL_TYPE;
    functionPrototype.type = NULL_TYPE;

    id_length = identifier(QStringLiteral("length"));
    id_prototype = identifier(QStringLiteral("prototype"));

    //
    // set up the global object
    //
    VM::Object *glo = newArgumentsObject(rootContext);
    __qmljs_init_object(&globalObject, glo);
    __qmljs_init_object(&rootContext->activation, glo);

    objectCtor = ObjectCtor::create(this);
    objectPrototype = objectCtor.property(rootContext, id_prototype);

    functionCtor = FunctionCtor::create(this);
    functionPrototype = functionCtor.property(rootContext, id_prototype);

    stringCtor = StringCtor::create(this);
    numberCtor = NumberCtor::create(this);
    booleanCtor = BooleanCtor::create(this);
    arrayCtor = ArrayCtor::create(this);
    dateCtor = DateCtor::create(this);

    stringPrototype = stringCtor.property(rootContext, id_prototype);
    numberPrototype = numberCtor.property(rootContext, id_prototype);
    booleanPrototype = booleanCtor.property(rootContext, id_prototype);
    arrayPrototype = arrayCtor.property(rootContext, id_prototype);
    datePrototype = dateCtor.property(rootContext, id_prototype);

    glo->setProperty(rootContext, identifier(QStringLiteral("Object")), objectCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("String")), stringCtor);
    glo->setProperty(rootContext, identifier(QStringLiteral("Number")), numberCtor);
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
    if (scope->engine->functionPrototype.isObject())
        f->prototype = scope->engine->functionPrototype.objectValue;
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(Context *scope, IR::Function *function)
{
    ScriptFunction *f = new ScriptFunction(scope, function);
    if (scope->engine->functionPrototype.isObject())
        f->prototype = scope->engine->functionPrototype.objectValue;
    return f;
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
    assert(objectPrototype.isObject());
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
    assert(objectPrototype.isObject());
    dateProto->prototype = objectPrototype.objectValue;
    return dateProto;
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new ErrorObject(value);
    assert(objectPrototype.isObject());
    object->prototype = objectPrototype.objectValue;
    return object;
}

Object *ExecutionEngine::newMathObject(Context *ctx)
{
    MathObject *object = new MathObject(ctx);
    assert(objectPrototype.isObject());
    object->prototype = objectPrototype.objectValue;
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
