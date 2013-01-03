/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMLJS_OBJECTS_H
#define QMLJS_OBJECTS_H

#include "qmljs_runtime.h"
#include "qmljs_engine.h"
#include "qmljs_environment.h"
#include "qv4array_p.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

namespace QQmlJS {

namespace VM {

struct Value;
struct Function;
struct Object;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
struct RegExpObject;
struct ErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct RegExpPrototype;
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;

struct String {
    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        else if (other && hashValue() == other->hashValue())
            return toQString() == other->toQString();
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
    enum { InvalidArrayIndex = 0xffffffff };
    uint asArrayIndex() const;

private:
    friend class StringPool;
    String(const QString &text)
        : _text(text), _hashValue(0) {}

private:
    QString _text;
    mutable unsigned _hashValue;
};

struct PropertyDescriptor {
    enum Type {
        Generic,
        Data,
        Accessor
    };
    enum State {
        Undefined,
        Disabled,
        Enabled
    };
    union {
        Value value;
        struct {
            FunctionObject *get;
            FunctionObject *set;
        };
    };
    uint type : 8;
    uint writable : 8;
    uint enumberable : 8;
    uint configurable : 8;

    static inline PropertyDescriptor fromValue(Value v) {
        PropertyDescriptor pd;
        pd.value = v;
        pd.type = Data;
        pd.writable = Undefined;
        pd.enumberable = Undefined;
        pd.configurable = Undefined;
        return pd;
    }
    static inline PropertyDescriptor fromAccessor(FunctionObject *getter, FunctionObject *setter) {
        PropertyDescriptor pd;
        pd.get = getter;
        pd.set = setter;
        pd.type = Accessor;
        pd.writable = Undefined;
        pd.enumberable = Undefined;
        pd.configurable = Undefined;
        return pd;
    }

    // Section 8.10
    inline void fullyPopulated() {
        if (type == Generic) {
            type = Data;
            value = Value::undefinedValue();
        }
        if (type == Data) {
            if (writable == Undefined)
                writable = Disabled;
        } else {
            writable = Undefined;
        }
        if (enumberable == Undefined)
            enumberable = Disabled;
        if (configurable == Undefined)
            configurable = Disabled;
    }

    inline bool isData() const { return type == Data; }
    inline bool isAccessor() const { return type == Accessor; }
    inline bool isGeneric() const { return type == Generic; }

    inline bool isWritable() const { return writable == Enabled; }
    inline bool isEnumerable() const { return enumberable == Enabled; }
    inline bool isConfigurable() const { return configurable == Enabled; }

    inline bool isEmpty() {
        return type == Generic && writable == Undefined && enumberable == Undefined && configurable == Undefined;
    }
    inline bool isSubset(PropertyDescriptor *other) {
        if (type != other->type)
            return false;
        if (enumberable != Undefined && enumberable != other->enumberable)
            return false;
        if (configurable != Undefined && configurable != other->configurable)
            return false;
        if (writable != Undefined && writable != other->writable)
            return false;
        if (type == Data && !value.sameValue(other->value))
            return false;
        if (type == Accessor && (get != other->get || set != other->set))
            return false;
        return true;
    }
    inline void operator+=(const PropertyDescriptor &other) {
        type = other.type;
        if (other.enumberable != Undefined)
            enumberable = other.enumberable;
        if (other.configurable != Undefined)
            configurable = other.configurable;
        if (other.writable != Undefined)
            writable = other.writable;
        if (type == Accessor) {
            if (other.get)
                get = other.get;
            if (other.set)
                set = other.set;
        } else {
            value = other.value;
        }
    }
};

struct PropertyTableEntry {
    PropertyDescriptor descriptor;
    String *name;
    PropertyTableEntry *next;
    int index;

    inline PropertyTableEntry(String *name)
        : name(name),
          next(0),
          index(-1)
    { }

    inline bool hasName(String *n) const { return name->isEqualTo(n); }
    inline unsigned hashValue() const { return name->hashValue(); }
};

class PropertyTable
{
    Q_DISABLE_COPY(PropertyTable)

public:
    PropertyTable()
        : _properties(0)
        , _buckets(0)
        , _freeList(0)
        , _propertyCount(-1)
        , _bucketCount(0)
        , _primeIdx(-1)
        , _allocated(0)
    {}

    ~PropertyTable()
    {
        qDeleteAll(_properties, _properties + _propertyCount + 1);
        delete[] _properties;
        delete[] _buckets;
    }

    inline bool isEmpty() const { return _propertyCount == -1; }

    typedef PropertyTableEntry **iterator;
    inline iterator begin() const { return _properties; }
    inline iterator end() const { return _properties + (_propertyCount + 1); }

    void remove(PropertyTableEntry *prop)
    {
        PropertyTableEntry **bucket = _buckets + (prop->hashValue() % _bucketCount);
        if (*bucket == prop) {
            *bucket = prop->next;
        } else {
            for (PropertyTableEntry *it = *bucket; it; it = it->next) {
                if (it->next == prop) {
                    it->next = it->next->next;
                    break;
                }
            }
        }

        _properties[prop->index] = 0;
        prop->next = _freeList;
        _freeList = prop;
    }

    PropertyTableEntry *findEntry(String *name) const
    {
        if (_properties) {
            for (PropertyTableEntry *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
                if (prop && (prop->name == name || prop->hasName(name)))
                    return prop;
            }
        }

        return 0;
    }

    PropertyDescriptor *find(String *name) const
    {
        if (_properties) {
            for (PropertyTableEntry *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
                if (prop && (prop->name == name || prop->hasName(name)))
                    return &prop->descriptor;
            }
        }

        return 0;
    }

    PropertyDescriptor *insert(String *name)
    {
        if (PropertyTableEntry *prop = findEntry(name))
            return &prop->descriptor;

        if (++_propertyCount == _allocated) {
            if (! _allocated)
                _allocated = 4;
            else
                _allocated *= 2;

            PropertyTableEntry **properties = new PropertyTableEntry*[_allocated];
            std::copy(_properties, _properties + _propertyCount, properties);
            delete[] _properties;
            _properties = properties;
        }

        PropertyTableEntry *prop;
        if (_freeList) {
            prop = _freeList;
            _freeList = _freeList->next;
        } else {
            prop = new PropertyTableEntry(name);
        }

        prop->index = _propertyCount;
        _properties[_propertyCount] = prop;

        if (! _buckets || 3 * _propertyCount >= 2 * _bucketCount) {
            rehash();
        } else {
            PropertyTableEntry *&bucket = _buckets[prop->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }

        return &prop->descriptor;
    }

private:
    void rehash()
    {
        _bucketCount = nextPrime();

        delete[] _buckets;
        _buckets = new PropertyTableEntry *[_bucketCount];
        std::fill(_buckets, _buckets + _bucketCount, (PropertyTableEntry *) 0);

        for (int i = 0; i <= _propertyCount; ++i) {
            PropertyTableEntry *prop = _properties[i];
            PropertyTableEntry *&bucket = _buckets[prop->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }
    }

    inline int nextPrime()
    {
        // IMPORTANT: do not add more primes without checking if _primeIdx needs more bits!
        static const int primes[] = {
            11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877
        };

        if (_primeIdx < (int) (sizeof(primes)/sizeof(int)))
            return primes[++_primeIdx];
        else
            return _bucketCount * 2 + 1; // Yes, we're degrading here. But who needs more than about 68000 properties?
    }

private:
    friend struct ForEachIteratorObject;
    PropertyTableEntry **_properties;
    PropertyTableEntry **_buckets;
    PropertyTableEntry *_freeList;
    int _propertyCount;
    int _bucketCount;
    int _primeIdx: 5;
    int _allocated: 27;
};

struct Object: Managed {
    Object *prototype;
    QScopedPointer<PropertyTable> members;

    Object()
        : prototype(0) {}

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
    virtual ArgumentsObject *asArgumentsObject() { return 0; }

    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty = 0);
    virtual PropertyDescriptor *__getOwnProperty__(ExecutionContext *ctx, String *name);
    PropertyDescriptor *__getPropertyDescriptor__(ExecutionContext *ctx, String *name);
    virtual void __put__(ExecutionContext *ctx, String *name, Value value);
    virtual bool __canPut__(ExecutionContext *ctx, String *name);
    virtual bool __hasProperty__(const ExecutionContext *ctx, String *name) const;
    virtual bool __delete__(ExecutionContext *ctx, String *name);
    virtual bool __defineOwnProperty__(ExecutionContext *ctx, String *name, PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, PropertyDescriptor *desc);

    virtual Value call(ExecutionContext *context, Value, Value *, int);

    //
    // helpers
    //
    void __put__(ExecutionContext *ctx, const QString &name, const Value &value);

    Value getValue(ExecutionContext *ctx, PropertyDescriptor *p) const;
    bool inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx);
    virtual bool inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *), int count = 0);
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value);

protected:
    virtual void getCollectables(QVector<Object *> &objects);
};

struct ForEachIteratorObject: Object {
    Object *object;
    Object *current; // inside the prototype chain
    int tableIndex;
    ForEachIteratorObject(Object *o) : object(o), current(o), tableIndex(-1) {}
    virtual QString className() { return QStringLiteral("__ForEachIteratorObject"); }

    String *nextPropertyName();

protected:
    virtual void getCollectables(QVector<Object *> &objects);
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
    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty);

    virtual bool inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx);

protected:
    virtual void getCollectables(QVector<Object *> &objects);
};

struct Function {
    QString name;

    VM::Value (*code)(VM::ExecutionContext *, const uchar *);
    const uchar *codeData;
    JSC::MacroAssemblerCodeRef codeRef;

    QList<QString> formals;
    QList<QString> locals;
    QVector<Function *> nestedFunctions;

    bool hasDirectEval: 1;
    bool usesArgumentsObject : 1;
    bool isStrict: 1;

    Function(const QString &name)
        : name(name)
        , code(0)
        , codeData(0)
        , hasDirectEval(false)
        , usesArgumentsObject(false)
        , isStrict(false)
    {}
    ~Function();

    inline bool hasNestedFunctions() const { return !nestedFunctions.isEmpty(); }

    inline bool needsActivation() const { return hasNestedFunctions() || hasDirectEval || usesArgumentsObject; }
};

struct FunctionObject: Object {
    ExecutionContext *scope;
    String *name;
    String **formalParameterList;
    String **varList;
    unsigned int formalParameterCount;
    unsigned int varCount;

    FunctionObject(ExecutionContext *scope)
        : scope(scope)
        , name(0)
        , formalParameterList(0)
        , varList(0)
        , formalParameterCount(0)
        , varCount(0)
        { needsActivation = false;
          usesArgumentsObject = false;
          strictMode = false; }

    virtual QString className() { return QStringLiteral("Function"); }
    virtual FunctionObject *asFunctionObject() { return this; }
    virtual bool hasInstance(ExecutionContext *ctx, const Value &value);

    Value construct(ExecutionContext *context, Value *args, int argc);
    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);

    virtual struct ScriptFunction *asScriptFunction() { return 0; }

protected:
    virtual Value call(ExecutionContext *ctx);
    virtual Value construct(ExecutionContext *ctx);
};

struct NativeFunction: FunctionObject {
    Value (*code)(ExecutionContext *);

    NativeFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *));
    virtual Value call(ExecutionContext *ctx) { return code(ctx); }
};

struct ScriptFunction: FunctionObject {
    VM::Function *function;

    ScriptFunction(ExecutionContext *scope, VM::Function *function);
    virtual ~ScriptFunction();

    virtual Value call(ExecutionContext *ctx);

    virtual ScriptFunction *asScriptFunction() { return this; }
};

struct EvalFunction : FunctionObject
{
    EvalFunction(ExecutionContext *scope);

    static QQmlJS::VM::Function *parseSource(QQmlJS::VM::ExecutionContext *ctx,
                                             const QString &fileName,
                                             const QString &source,
                                             QQmlJS::Codegen::Mode mode);

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
};

struct IsNaNFunction: FunctionObject
{
    IsNaNFunction(ExecutionContext *scope);

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
};

struct IsFiniteFunction: FunctionObject
{
    IsFiniteFunction(ExecutionContext *scope);

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
};

struct RegExpObject: Object {
    QRegularExpression value;
    Value lastIndex;
    bool global;
    RegExpObject(const QRegularExpression &value, bool global): value(value), lastIndex(Value::fromInt32(0)), global(global) {}
    virtual QString className() { return QStringLiteral("RegExp"); }
    virtual RegExpObject *asRegExpObject() { return this; }
    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty);
};

struct ErrorObject: Object {
    Value value;
    ErrorObject(const Value &message): value(message) {}
    virtual QString className() { return QStringLiteral("Error"); }
    virtual ErrorObject *asErrorObject() { return this; }
    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty);

    virtual struct SyntaxErrorObject *asSyntaxError() { return 0; }

protected:
    void setNameProperty(ExecutionContext *ctx);
    virtual void getCollectables(QVector<Object *> &objects);
};

struct EvalErrorObject: ErrorObject {
    EvalErrorObject(ExecutionContext *ctx)
        : ErrorObject(ctx->argument(0)) { setNameProperty(ctx); }
    virtual QString className() { return QStringLiteral("EvalError"); }
};

struct RangeErrorObject: ErrorObject {
    RangeErrorObject(ExecutionContext *ctx)
        : ErrorObject(ctx->argument(0)) { setNameProperty(ctx); }
    virtual QString className() { return QStringLiteral("RangeError"); }
};

struct ReferenceErrorObject: ErrorObject {
    ReferenceErrorObject(ExecutionContext *ctx)
        : ErrorObject(ctx->argument(0)) { setNameProperty(ctx); }
    ReferenceErrorObject(ExecutionContext *ctx, const QString &msg)
        : ErrorObject(Value::fromString(ctx,msg)) { setNameProperty(ctx); }
    virtual QString className() { return QStringLiteral("ReferenceError"); }
};

struct SyntaxErrorObject: ErrorObject {
    SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *msg);
    ~SyntaxErrorObject() { delete msg; }
    virtual QString className() { return QStringLiteral("SyntaxError"); }

    virtual SyntaxErrorObject *asSyntaxError() { return this; }
    DiagnosticMessage *message() { return msg; }

private:
    DiagnosticMessage *msg;
};

struct TypeErrorObject: ErrorObject {
    TypeErrorObject(ExecutionContext *ctx)
        : ErrorObject(ctx->argument(0)) { setNameProperty(ctx); }
    TypeErrorObject(ExecutionContext *ctx, const QString &msg)
        : ErrorObject(Value::fromString(ctx,msg)) { setNameProperty(ctx); }
    virtual QString className() { return QStringLiteral("TypeError"); }
};

struct URIErrorObject: ErrorObject {
    URIErrorObject(ExecutionContext *ctx)
        : ErrorObject(ctx->argument(0)) { setNameProperty(ctx); }
    virtual QString className() { return QStringLiteral("URIError"); }
};

struct ArgumentsObject: Object {
    ExecutionContext *context;
    int currentIndex;
    ArgumentsObject(ExecutionContext *context);
    virtual QString className() { return QStringLiteral("Arguments"); }
    virtual ArgumentsObject *asArgumentsObject() { return this; }

    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty = 0);
    virtual void __put__(ExecutionContext *ctx, String *name, Value value);

    static Value method_getArg(ExecutionContext *ctx);
    static Value method_setArg(ExecutionContext *ctx);
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
