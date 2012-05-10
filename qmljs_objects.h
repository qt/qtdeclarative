#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qmljs_runtime.h"

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
struct FunctionObject;
struct ErrorObject;

struct String {
    String(const QString &text)
        : _text(text), _hashValue(0) {}

    inline const QString &text() const {
        return _text;
    }

    inline unsigned hashValue() const {
        if (! _hashValue)
            _hashValue = qHash(_text);

        return _hashValue;
    }

    static String *get(Context *ctx, const QString &s) {
        Q_UNUSED(ctx);
        return new String(s);
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
        } else if (name->hashValue() == n->hashValue() && name->text() == n->text()) {
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

    virtual FunctionObject *asFunctionObject() { return 0; }

    bool get(String *name, Value *result);

    virtual Value *getOwnProperty(String *name, PropertyAttributes *attributes = 0);
    virtual Value *getProperty(String *name, PropertyAttributes *attributes = 0);
    virtual void put(String *name, const Value &value, bool flag = 0);
    virtual bool canPut(String *name);
    virtual bool hasProperty(String *name) const;
    virtual bool deleteProperty(String *name, bool flag);
    virtual void defaultValue(Value *result, int typeHint);
    // ### TODO: defineOwnProperty(name, descriptor, boolean) -> boolean
};

struct BooleanObject: Object {
    Value value;
    BooleanObject(const Value &value): value(value) {}
    virtual void defaultValue(Value *result, int /*typehint*/) { *result = value; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(const Value &value): value(value) {}
    virtual void defaultValue(Value *result, int /*typehint*/) { *result = value; }
};

struct StringObject: Object {
    Value value;
    StringObject(const Value &value): value(value) {}
    virtual void defaultValue(Value *result, int /*typehint*/) { *result = value; }
};

struct ArrayObject: Object {
};

struct FunctionObject: Object {
    Object *scope;
    String **formalParameterList;
    size_t formalParameterCount;

    FunctionObject(Object *scope = 0): scope(scope), formalParameterList(0), formalParameterCount(0) {}
    virtual FunctionObject *asFunctionObject() { return this; }

    virtual bool hasInstance(const Value &value) const;
    virtual void call(Context *ctx);
    virtual void construct(Context *ctx);
};

struct ScriptFunction: FunctionObject {
    Context *context;
    IR::Function *function;

    ScriptFunction(Context *context, IR::Function *function);
    virtual ~ScriptFunction();

    virtual void call(Context *ctx);
};

struct ErrorObject: Object {
    String *message;
    ErrorObject(String *message): message(message) {}
};

struct ArgumentsObject: Object {
    Context *context;
    ArgumentsObject(Context *context): context(context) {}
    virtual Value *getProperty(String *name, PropertyAttributes *attributes);
};

struct Context {
    Context *parent;
    Value activation;
    Value thisObject;
    Object *scope;
    Value *arguments;
    size_t argumentCount;
    Value result;
    String **formals;
    size_t formalCount;

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
            __qmljs_init_undefined(this, result);
    }

    void init()
    {
        parent = 0;
        scope = 0;
        arguments = 0;
        argumentCount = 0;
        activation.type = NULL_TYPE;
        thisObject.type = NULL_TYPE;
        result.type = UNDEFINED_TYPE;
        formals = 0;
        formalCount = 0;
    }
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
