#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qmljs_runtime.h"

#include <gc/gc_cpp.h>
#include <QtCore/QString>
#include <QtCore/QHash>

struct Value;
struct Object;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct FunctionObject;
struct ErrorObject;

struct String: gc_cleanup {
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
        return new (GC) String(s);
    }

private:
    QString _text;
    mutable unsigned _hashValue;
};

struct Property: gc {
    String *name;
    Value value;
    PropertyAttributes flags;
    Property *next;

    Property(String *name, const Value &value, PropertyAttributes flags = NoAttributes)
        : name(name), value(value), flags(flags), next(0) {}

    inline bool isWritable() const { return flags & WritableAttribute; }
    inline bool isEnumerable() const { return flags & EnumerableAttribute; }
    inline bool isConfigurable() const { return flags & ConfigurableAttribute; }

    inline bool hasName(String *n) const {
        if (name == n || (name->hashValue() == n->hashValue() && name->text() == n->text()))
            return true;
        return false;
    }

    inline unsigned hashValue() const {
        return name->hashValue();
    }
};

class Table: public gc
{
    Property **_properties;
    Property **_buckets;
    int _propertyCount;
    int _bucketCount;
    int _allocated;

public:
    Table()
        : _properties(0)
        , _buckets(0)
        , _propertyCount(-1)
        , _bucketCount(11)
        , _allocated(0) {}

    bool empty() const { return _propertyCount == -1; }
    unsigned size() const { return _propertyCount + 1; }

    typedef Property **iterator;
    iterator begin() const { return _properties; }
    iterator end() const { return _properties + (_propertyCount + 1); }

    bool remove(String *name)
    {
        if (_properties) {
            const unsigned hash = name->hashValue() % _bucketCount;

            if (Property *prop = _buckets[hash]) {
                if (prop->hasName(name)) {
                    _buckets[hash] = prop->next;
                    return true;
                }

                do {
                    Property *next = prop->next;

                    if (next && next->hasName(name)) {
                        prop->next = next->next;
                        return true;
                    }
                    prop = next;
                } while (prop);
            }
        }

        return false;
    }

    Property *find(String *name) const
    {
        if (! _properties)
            return 0;

        for (Property *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
            if (prop->hasName(name))
                return prop;
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

            Property **properties = new (GC) Property*[_allocated];
            std::copy(_properties, _properties + _propertyCount, properties);
            _properties = properties;
        }

        Property *prop = new (GC) Property(name, value);
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

        _buckets = new (GC) Property *[_bucketCount];
        std::fill(_buckets, _buckets + _bucketCount, (Property *) 0);

        for (int i = 0; i <= _propertyCount; ++i) {
            Property *prop = _properties[i];
            Property *&bucket = _buckets[prop->name->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }
    }
};

struct Object: gc_cleanup {
    Object *prototype;
    String *klass;
    Table *members;
    bool extensible;

    Object()
        : prototype(0)
        , klass(0)
        , members(0)
        , extensible(true) {}

    virtual ~Object() {}

    virtual FunctionObject *asFunctionObject() { return 0; }

    virtual bool get(String *name, Value *result);
    virtual Property *getOwnProperty(String *name);
    virtual Property *getProperty(String *name);
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
    virtual void defaultValue(Value *result, int typehint) { *result = value; }
};

struct NumberObject: Object {
    Value value;
    NumberObject(const Value &value): value(value) {}
    virtual void defaultValue(Value *result, int typehint) { *result = value; }
};

struct StringObject: Object {
    Value value;
    StringObject(const Value &value): value(value) {}
    virtual void defaultValue(Value *result, int typehint) { *result = value; }
};

struct ArrayObject: Object {
};

struct FunctionObject: Object {
    Object *scope;
    String **formalParameterList;

    FunctionObject(Object *scope = 0): scope(scope), formalParameterList(0) {}
    virtual FunctionObject *asFunctionObject() { return this; }

    virtual bool hasInstance(const Value &value) const;
    virtual Value call(const Value &thisObject, const Value args[], unsigned argc);
    virtual Value construct(const Value args[], unsigned argc);
};

struct ErrorObject: Object {
    String *message;
    ErrorObject(String *message): message(message) {}
};

struct ArgumentsObject: Object {
};

struct Context: gc {
    Value activation;
    Value thisObject;
    Object *scope;
    Value *arguments;
    unsigned argumentCount;

    Context()
        : scope(0)
        , arguments(0)
        , argumentCount(0)
    {
        activation.type = NULL_TYPE;
        thisObject.type = NULL_TYPE;
    }
};

#endif // QMLJS_OBJECTS_H
