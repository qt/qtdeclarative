#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qmljs_runtime.h"
#include "qv4array_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <cassert>

namespace QQmlJS {

namespace IR {
struct Function;
}

namespace VM {

struct Value;
struct Object;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
struct ErrorObject;
struct ArgumentsObject;
struct Context;
struct ExecutionEngine;

struct String {
    String(const QString &text)
        : _text(text), _hashValue(0) {}

    inline const QString &toQString() const {
        return _text;
    }

    inline unsigned hashValue() const {
        if (! _hashValue)
            _hashValue = qHash(_text);

        return _hashValue;
    }

private:
    QString _text;
    mutable unsigned _hashValue;
};

struct Property {
    String *name;
    Value value;
    PropertyAttributes attributes;
    Property *next;
    int index;

    Property(String *name, const Value &value, PropertyAttributes flags = NoAttributes)
        : name(name), value(value), attributes(flags), next(0), index(-1) {}

    inline bool isWritable() const { return attributes & WritableAttribute; }
    inline bool isEnumerable() const { return attributes & EnumerableAttribute; }
    inline bool isConfigurable() const { return attributes & ConfigurableAttribute; }

    inline bool hasName(String *n) const {
        if (name == n) {
            return true;
        } else if (name->hashValue() == n->hashValue() && name->toQString() == n->toQString()) {
            return true;
        }
        return false;
    }

    inline unsigned hashValue() const {
        return name->hashValue();
    }
};

class Table
{
    Q_DISABLE_COPY(Table)

public:
    Table()
        : _properties(0)
        , _buckets(0)
        , _propertyCount(-1)
        , _bucketCount(11)
        , _allocated(0) {}

    ~Table()
    {
        qDeleteAll(_properties, _properties + _propertyCount + 1);
        delete[] _properties;
        delete[] _buckets;
    }

    inline bool isEmpty() const { return _propertyCount == -1; }

    typedef Property **iterator;
    inline iterator begin() const { return _properties; }
    inline iterator end() const { return _properties + (_propertyCount + 1); }

    bool remove(String *name)
    {
        Q_UNUSED(name);
        assert(!"TODO");
        return false;
    }

    Property *find(String *name) const
    {
        if (_properties) {
            for (Property *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
                if (prop->hasName(name))
                    return prop;
            }
        }

        return 0;
    }

    Property *insert(String *name, const Value &value)
    {
        if (Property *prop = find(name)) {
            prop->value = value;
            return prop;
        }

        if (++_propertyCount == _allocated) {
            if (! _allocated)
                _allocated = 4;
            else
                _allocated *= 2;

            Property **properties = new Property*[_allocated];
            std::copy(_properties, _properties + _propertyCount, properties);
            delete[] _properties;
            _properties = properties;
        }

        Property *prop = new Property(name, value);
        prop->index = _propertyCount;
        _properties[_propertyCount] = prop;

        if (! _buckets || 3 * _propertyCount >= 2 * _bucketCount) {
            rehash();
        } else {
            Property *&bucket = _buckets[prop->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }

        return prop;
    }

private:
    void rehash()
    {
        if (_bucketCount)
            _bucketCount *= 2; // ### next prime

        if (_buckets)
            delete[] _buckets;

        _buckets = new Property *[_bucketCount];
        std::fill(_buckets, _buckets + _bucketCount, (Property *) 0);

        for (int i = 0; i <= _propertyCount; ++i) {
            Property *prop = _properties[i];
            Property *&bucket = _buckets[prop->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }
    }

private:
    Property **_properties;
    Property **_buckets;
    int _propertyCount;
    int _bucketCount;
    int _allocated;
};

struct Object {
    Object *prototype;
    String *klass;
    Table *members;
    bool extensible;

    Object()
        : prototype(0)
        , klass(0)
        , members(0)
        , extensible(true) {}

    virtual ~Object();

    virtual BooleanObject *asBooleanObject() { return 0; }
    virtual NumberObject *asNumberObject() { return 0; }
    virtual StringObject *asStringObject() { return 0; }
    virtual DateObject *asDateObject() { return 0; }
    virtual ArrayObject *asArrayObject() { return 0; }
    virtual FunctionObject *asFunctionObject() { return 0; }
    virtual ErrorObject *asErrorObject() { return 0; }
    virtual ArgumentsObject *asArgumentsObject() { return 0; }

    bool get(String *name, Value *result);

    virtual Value *getOwnProperty(String *name, PropertyAttributes *attributes = 0);
    virtual Value *getProperty(String *name, PropertyAttributes *attributes = 0);
    virtual void put(String *name, const Value &value, bool flag = 0);
    virtual bool canPut(String *name);
    virtual bool hasProperty(String *name) const;
    virtual bool deleteProperty(String *name, bool flag);
    // ### TODO: defineOwnProperty(name, descriptor, boolean) -> boolean

    //
    // helpers
    //
    void setProperty(Context *ctx, const QString &name, const Value &value);
    void setProperty(Context *ctx, const QString &name, void (*code)(Context *), int count = 0);
};

struct BooleanObject: Object {
    Value value;
    BooleanObject(const Value &value): value(value) {}
    virtual BooleanObject *asBooleanObject() { return this; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(const Value &value): value(value) {}
    virtual NumberObject *asNumberObject() { return this; }
};

struct StringObject: Object {
    Value value;
    StringObject(const Value &value): value(value) {}
    virtual StringObject *asStringObject() { return this; }
};

struct DateObject: Object {
    Value value;
    DateObject(const Value &value): value(value) {}
    virtual DateObject *asDateObject() { return this; }
};

struct ArrayObject: Object {
    Array value;
    Value length;
    ArrayObject() { length.type = NUMBER_TYPE; }
    ArrayObject(const Array &value): value(value) { length.type = NUMBER_TYPE; }
    virtual ArrayObject *asArrayObject() { return this; }
    virtual Value *getOwnProperty(String *name, PropertyAttributes *attributes);
};

struct FunctionObject: Object {
    Context *scope;
    String **formalParameterList;
    size_t formalParameterCount;
    String **varList;
    size_t varCount;
    bool needsActivation;

    FunctionObject(Context *scope)
        : scope(scope)
        , formalParameterList(0)
        , formalParameterCount(0)
        , varList(0)
        , varCount(0)
        , needsActivation(true) {}

    virtual FunctionObject *asFunctionObject() { return this; }
    virtual bool hasInstance(const Value &value) const;
    virtual void call(Context *ctx);
    virtual void construct(Context *ctx);
};

struct NativeFunction: FunctionObject {
    void (*code)(Context *);

    NativeFunction(Context *scope, void (*code)(Context *)): FunctionObject(scope), code(code) {}
    virtual void call(Context *ctx) { code(ctx); }
    virtual void construct(Context *ctx) { code(ctx); }
};

struct ScriptFunction: FunctionObject {
    IR::Function *function;

    ScriptFunction(Context *scope, IR::Function *function);
    virtual ~ScriptFunction();

    virtual void call(Context *ctx);
    virtual void construct(Context *ctx);
};

struct ErrorObject: Object {
    Value message;
    ErrorObject(const Value &message): message(message) {}
    virtual ErrorObject *asErrorObject() { return this; }
};

struct ArgumentsObject: Object {
    Context *context;
    ArgumentsObject(Context *context): context(context) {}
    virtual ArgumentsObject *asArgumentsObject() { return this; }
    virtual Value *getProperty(String *name, PropertyAttributes *attributes);
};

struct Context {
    ExecutionEngine *engine;
    Context *parent;
    Value activation;
    Value thisObject;
    Value *arguments;
    size_t argumentCount;
    Value *locals;
    Value result;
    String **formals;
    size_t formalCount;
    String **vars;
    size_t varCount;
    bool calledAsConstructor;

    Value *lookup(String *name)
    {
        for (Context *ctx = this; ctx; ctx = ctx->parent) {
            if (ctx->activation.is(OBJECT_TYPE)) {
                if (Value *prop = ctx->activation.objectValue->getProperty(name)) {
                    return prop;
                }
            }
        }
        return 0;
    }

    inline Value argument(size_t index = 0)
    {
        Value arg;
        getArgument(&arg, index);
        return arg;
    }

    inline void getArgument(Value *result, size_t index)
    {
        if (index < argumentCount)
            *result = arguments[index];
        else
            __qmljs_init_undefined(result);
    }

    void init(ExecutionEngine *eng)
    {
        engine = eng;
        parent = 0;
        arguments = 0;
        argumentCount = 0;
        locals = 0;
        activation.type = NULL_TYPE;
        thisObject.type = NULL_TYPE;
        result.type = UNDEFINED_TYPE;
        formals = 0;
        formalCount = 0;
        vars = 0;
        varCount = 0;
        calledAsConstructor = false;
    }

    void initCallContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc);
    void leaveCallContext(FunctionObject *f, Value *r);

    void initConstructorContext(ExecutionEngine *e, const Value *object, FunctionObject *f, Value *args, int argc);
    void leaveConstructorContext(FunctionObject *f, Value *returnValue);
};

struct ExecutionEngine
{
    Context *rootContext;
    Value globalObject;

    Value objectCtor;
    Value stringCtor;
    Value numberCtor;
    Value booleanCtor;
    Value arrayCtor;
    Value dateCtor;

    Value objectPrototype;
    Value stringPrototype;
    Value numberPrototype;
    Value booleanPrototype;
    Value arrayPrototype;
    Value datePrototype;

    QHash<QString, String *> identifiers;

    ExecutionEngine();

    Context *newContext();

    String *identifier(const QString &s);

    FunctionObject *newNativeFunction(Context *scope, void (*code)(Context *));
    FunctionObject *newScriptFunction(Context *scope, IR::Function *function);

    Object *newObject();
    FunctionObject *newObjectCtor(Context *ctx);
    Object *newObjectPrototype(Context *ctx, FunctionObject *proto);

    String *newString(const QString &s);
    Object *newStringObject(const Value &value);
    FunctionObject *newStringCtor(Context *ctx);
    Object *newStringPrototype(Context *ctx, FunctionObject *proto);

    Object *newNumberObject(const Value &value);
    FunctionObject *newNumberCtor(Context *ctx);
    Object *newNumberPrototype(Context *ctx, FunctionObject *proto);

    Object *newBooleanObject(const Value &value);
    FunctionObject *newBooleanCtor(Context *ctx);
    Object *newBooleanPrototype(Context *ctx, FunctionObject *proto);

    Object *newArrayObject();
    Object *newArrayObject(const Array &value);
    FunctionObject *newArrayCtor(Context *ctx);
    Object *newArrayPrototype(Context *ctx, FunctionObject *proto);

    Object *newDateObject(const Value &value);
    FunctionObject *newDateCtor(Context *ctx);
    Object *newDatePrototype(Context *ctx, FunctionObject *proto);

    Object *newErrorObject(const Value &value);
    Object *newMathObject(Context *ctx);
    Object *newArgumentsObject(Context *ctx);
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
