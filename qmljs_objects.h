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
struct RegExpObject;
struct ErrorObject;
struct ActivationObject;
struct ArgumentsObject;
struct Context;
struct ExecutionEngine;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct RegExpPrototype;

struct String {
    String(const QString &text)
        : _text(text), _hashValue(0) {}

    inline bool isEqualTo(const String *other) const {
        if (other && hashValue() == other->hashValue()) {
            if (this == other)
                return true;
            else
                return toQString() == other->toQString();
        }
        return false;
    }

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

    inline bool hasName(String *n) const { return name->isEqualTo(n); }
    inline unsigned hashValue() const { return name->hashValue(); }
};

class Table
{
    Q_DISABLE_COPY(Table)

public:
    Table()
        : _properties(0)
        , _buckets(0)
        , _propertyCount(-1)
        , _bucketCount(0)
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
                if (prop->name == name || prop->hasName(name))
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
        else
            _bucketCount = 11;

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

    virtual QString className() { return QStringLiteral("Object"); }
    virtual BooleanObject *asBooleanObject() { return 0; }
    virtual NumberObject *asNumberObject() { return 0; }
    virtual StringObject *asStringObject() { return 0; }
    virtual DateObject *asDateObject() { return 0; }
    virtual ArrayObject *asArrayObject() { return 0; }
    virtual FunctionObject *asFunctionObject() { return 0; }
    virtual RegExpObject *asRegExpObject() { return 0; }
    virtual ErrorObject *asErrorObject() { return 0; }
    virtual ActivationObject *asActivationObject() { return 0; }
    virtual ArgumentsObject *asArgumentsObject() { return 0; }

    virtual Value getProperty(Context *ctx, String *name, PropertyAttributes *attributes = 0);
    virtual Value *getOwnProperty(Context *ctx, String *name, PropertyAttributes *attributes = 0);
    virtual Value *getPropertyDescriptor(Context *ctx, String *name, PropertyAttributes *attributes = 0);
    virtual void setProperty(Context *ctx, String *name, const Value &value, bool flag = false);
    virtual bool canSetProperty(Context *ctx, String *name);
    virtual bool hasProperty(Context *ctx, String *name) const;
    virtual bool deleteProperty(Context *ctx, String *name, bool flag);
    virtual void defineOwnProperty(Context *ctx, const Value &getter, const Value &setter, bool flag = false);

    //
    // helpers
    //
    void setProperty(Context *ctx, const QString &name, const Value &value);
    void setProperty(Context *ctx, const QString &name, void (*code)(Context *), int count = 0);
};

struct BooleanObject: Object {
    Value value;
    BooleanObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("Boolean"); }
    virtual BooleanObject *asBooleanObject() { return this; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("Number"); }
    virtual NumberObject *asNumberObject() { return this; }
};

struct StringObject: Object {
    Value value;
    StringObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("String"); }
    virtual StringObject *asStringObject() { return this; }
};

struct DateObject: Object {
    Value value;
    DateObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("Date"); }
    virtual DateObject *asDateObject() { return this; }
};

struct ArrayObject: Object {
    Array value;
    ArrayObject() {}
    ArrayObject(const Array &value): value(value) {}
    virtual QString className() { return QStringLiteral("Array"); }
    virtual ArrayObject *asArrayObject() { return this; }
    virtual Value getProperty(Context *ctx, String *name, PropertyAttributes *attributes);
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

    virtual QString className() { return QStringLiteral("Function"); }
    virtual FunctionObject *asFunctionObject() { return this; }
    virtual bool hasInstance(Context *ctx, const Value &value);
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

struct RegExpObject: Object {
    Value value;
    RegExpObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("RegExp"); }
    virtual RegExpObject *asRegExpObject() { return this; }
};

struct ErrorObject: Object {
    Value value;
    ErrorObject(const Value &message): value(message) {}
    virtual QString className() { return QStringLiteral("Error"); }
    virtual ErrorObject *asErrorObject() { return this; }
};

struct ActivationObject: Object {
    Context *context;
    Value arguments;
    ActivationObject(Context *context): context(context), arguments(Value::undefinedValue()) {}
    virtual QString className() { return QStringLiteral("Activation"); }
    virtual ActivationObject *asActivationObject() { return this; }
    virtual Value *getPropertyDescriptor(Context *ctx, String *name, PropertyAttributes *attributes);
};

struct ArgumentsObject: Object {
    Context *context;
    ArgumentsObject(Context *context): context(context) {}
    virtual QString className() { return QStringLiteral("Arguments"); }
    virtual ArgumentsObject *asArgumentsObject() { return this; }
    virtual Value getProperty(Context *ctx, String *name, PropertyAttributes *attributes);
    virtual Value *getPropertyDescriptor(Context *ctx, String *name, PropertyAttributes *attributes);
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
    int calledAsConstructor;
    int hasUncaughtException;

    Value *lookupPropertyDescriptor(String *name)
    {
        for (Context *ctx = this; ctx; ctx = ctx->parent) {
            if (ctx->activation.is(OBJECT_TYPE)) {
                if (Value *prop = ctx->activation.objectValue->getPropertyDescriptor(this, name)) {
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
        hasUncaughtException = false;
    }

    void throwError(const Value &value);
    void throwError(const QString &message);
    void throwTypeError();
    void throwUnimplemented(const QString &message);
    void throwReferenceError(const Value &value);

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
    Value functionCtor;
    Value dateCtor;
    Value regExpCtor;

    ObjectPrototype *objectPrototype;
    StringPrototype *stringPrototype;
    NumberPrototype *numberPrototype;
    BooleanPrototype *booleanPrototype;
    ArrayPrototype *arrayPrototype;
    FunctionPrototype *functionPrototype;
    DatePrototype *datePrototype;
    RegExpPrototype *regExpPrototype;

    QHash<QString, String *> identifiers;

    String *id_length;
    String *id_prototype;
    String *id_constructor;
    String *id_arguments;
    String *id___proto__;

    ExecutionEngine();

    Context *newContext();

    String *identifier(const QString &s);

    FunctionObject *newNativeFunction(Context *scope, void (*code)(Context *));
    FunctionObject *newScriptFunction(Context *scope, IR::Function *function);

    Object *newObject();
    FunctionObject *newObjectCtor(Context *ctx);

    String *newString(const QString &s);
    Object *newStringObject(const Value &value);
    FunctionObject *newStringCtor(Context *ctx);

    Object *newNumberObject(const Value &value);
    FunctionObject *newNumberCtor(Context *ctx);

    Object *newBooleanObject(const Value &value);
    FunctionObject *newBooleanCtor(Context *ctx);

    Object *newFunctionObject(Context *ctx);
    FunctionObject *newFunctionCtor(Context *ctx);

    Object *newArrayObject();
    Object *newArrayObject(const Array &value);
    FunctionObject *newArrayCtor(Context *ctx);

    Object *newDateObject(const Value &value);
    FunctionObject *newDateCtor(Context *ctx);

    Object *newRegExpObject(const Value &value);
    FunctionObject *newRegExpCtor(Context *ctx);

    Object *newErrorObject(const Value &value);
    Object *newMathObject(Context *ctx);
    Object *newActivationObject(Context *ctx);
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
