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
#include "qv4array.h"
#include "qv4string.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed.h"
#include "qv4propertydescriptor.h"
#include "qv4propertytable.h"
#include "qv4objectiterator.h"
#include "qv4regexp.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

namespace QQmlJS {

namespace VM {

struct Value;
struct Function;
struct Object;
struct ObjectIterator;
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


struct Object: Managed {
    Object *prototype;
    QScopedPointer<PropertyTable> members;
    Array array;

    Object()
        : prototype(0) {}
    Object(const Array &a)
        : prototype(0), array(a) {}

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

    PropertyDescriptor *__getOwnProperty__(ExecutionContext *ctx, String *name);
    PropertyDescriptor *__getOwnProperty__(ExecutionContext *ctx, uint index);
    PropertyDescriptor *__getPropertyDescriptor__(ExecutionContext *ctx, String *name);
    virtual PropertyDescriptor *__getPropertyDescriptor__(ExecutionContext *ctx, uint index);

    virtual Value __get__(ExecutionContext *ctx, String *name, bool *hasProperty = 0);
    virtual Value __get__(ExecutionContext *ctx, uint index, bool *hasProperty = 0);

    virtual void __put__(ExecutionContext *ctx, String *name, Value value);
    virtual void __put__(ExecutionContext *ctx, uint index, Value value);

    virtual bool __canPut__(ExecutionContext *ctx, String *name);
    virtual bool __canPut__(ExecutionContext *ctx, uint index);
    virtual bool __hasProperty__(const ExecutionContext *ctx, String *name) const;
    virtual bool __hasProperty__(const ExecutionContext *ctx, uint index) const;
    virtual bool __delete__(ExecutionContext *ctx, String *name);
    virtual bool __delete__(ExecutionContext *ctx, uint index);
    bool __defineOwnProperty__(ExecutionContext *ctx, PropertyDescriptor *current, PropertyDescriptor *desc);
    virtual bool __defineOwnProperty__(ExecutionContext *ctx, String *name, PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, uint index, PropertyDescriptor *desc);
    bool __defineOwnProperty__(ExecutionContext *ctx, const QString &name, PropertyDescriptor *desc);

    virtual Value call(ExecutionContext *context, Value, Value *, int);

    //
    // helpers
    //
    void __put__(ExecutionContext *ctx, const QString &name, const Value &value);

    Value getValue(ExecutionContext *ctx, const PropertyDescriptor *p) const;
    Value getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p) const;
    Value getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p, bool *exists) const;

    bool inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx);
    bool inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx);

    /* The spec default: Writable: true, Enumerable: false, Configurable: true */
    void defineDefaultProperty(String *name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value value);
    void defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *), int count = 0);
    /* Fixed: Writable: false, Enumerable: false, Configurable: false */
    void defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value);
    void defineReadonlyProperty(String *name, Value value);

protected:
    virtual void getCollectables(QVector<Object *> &objects);

    friend struct ObjectIterator;
};

struct ForEachIteratorObject: Object {
    ObjectIterator it;
    ForEachIteratorObject(ExecutionContext *ctx, Object *o)
        : it(ctx, o, ObjectIterator::EnumberableOnly|ObjectIterator::WithProtoChain) {}
    virtual QString className() { return QStringLiteral("__ForEachIteratorObject"); }

    Value nextPropertyName() { return it.nextPropertyNameAsString(); }

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
    PropertyDescriptor tmpProperty;
    StringObject(ExecutionContext *ctx, const Value &value);
    virtual QString className() { return QStringLiteral("String"); }
    virtual StringObject *asStringObject() { return this; }

    PropertyDescriptor *getIndex(ExecutionContext *ctx, uint index);
};

struct DateObject: Object {
    Value value;
    DateObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("Date"); }
    virtual DateObject *asDateObject() { return this; }
};

struct ArrayObject: Object {
    ArrayObject(ExecutionContext *ctx) { init(ctx); }
    ArrayObject(ExecutionContext *ctx, const Array &value): Object(value) { init(ctx); array.setLengthUnchecked(array.length()); }
    void init(ExecutionContext *context);
    virtual QString className() { return QStringLiteral("Array"); }
    virtual ArrayObject *asArrayObject() { return this; }
};

struct Function {
    QString name;

    VM::Value (*code)(VM::ExecutionContext *, const uchar *);
    const uchar *codeData;
    JSC::MacroAssemblerCodeRef codeRef;

    QList<QString> formals;
    QList<QString> locals;

    bool hasNestedFunctions  : 1;
    bool hasDirectEval       : 1;
    bool usesArgumentsObject : 1;
    bool isStrict            : 1;

    Function(const QString &name)
        : name(name)
        , code(0)
        , codeData(0)
        , hasNestedFunctions(0)
        , hasDirectEval(false)
        , usesArgumentsObject(false)
        , isStrict(false)
    {}
    ~Function();

    inline bool needsActivation() const { return hasNestedFunctions || hasDirectEval || usesArgumentsObject; }
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

    virtual Value construct(ExecutionContext *context, Value *args, int argc);
    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
    // Nothing to do in the default implementation, only _native_ functions might change context->thisObject.
    virtual void maybeAdjustThisObjectForDirectCall(ExecutionContext* /*context*/, Value /*thisArg*/) { }
    Value callDirect(ExecutionContext *context, Value thisObject, Value *args, int argc);

    virtual struct ScriptFunction *asScriptFunction() { return 0; }

protected:
    virtual Value call(ExecutionContext *ctx);
    virtual Value construct(ExecutionContext *ctx);
};

struct BuiltinFunction: FunctionObject {
    Value (*code)(ExecutionContext *);

    BuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *));
    virtual Value call(ExecutionContext *ctx) { return code(ctx); }
    virtual Value construct(ExecutionContext *ctx);
    virtual void maybeAdjustThisObjectForDirectCall(ExecutionContext *context, Value thisArg);
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

struct BoundFunction: FunctionObject {
    FunctionObject *target;
    Value boundThis;
    QVector<Value> boundArgs;

    BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);
    virtual ~BoundFunction() {}

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
    virtual Value construct(ExecutionContext *context, Value *args, int argc);
    virtual bool hasInstance(ExecutionContext *ctx, const Value &value);
    virtual void getCollectables(QVector<Object *> &objects);
};

struct ParseIntFunction: FunctionObject
{
    ParseIntFunction(ExecutionContext *scope);

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
};

struct ParseFloatFunction: FunctionObject
{
    ParseFloatFunction(ExecutionContext *scope);

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
    RefPtr<RegExp> value;
    Value lastIndex;
    bool global;
    RegExpObject(PassRefPtr<RegExp> value, bool global): value(value), lastIndex(Value::fromInt32(0)), global(global) {}
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
    EvalErrorObject(ExecutionContext *ctx);
    virtual QString className() { return QStringLiteral("EvalError"); }
};

struct RangeErrorObject: ErrorObject {
    RangeErrorObject(ExecutionContext *ctx);
    RangeErrorObject(ExecutionContext *ctx, const QString &msg);
    virtual QString className() { return QStringLiteral("RangeError"); }
};

struct ReferenceErrorObject: ErrorObject {
    ReferenceErrorObject(ExecutionContext *ctx);
    ReferenceErrorObject(ExecutionContext *ctx, const QString &msg);
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
    TypeErrorObject(ExecutionContext *ctx);
    TypeErrorObject(ExecutionContext *ctx, const QString &msg);
    virtual QString className() { return QStringLiteral("TypeError"); }
};

struct URIErrorObject: ErrorObject {
    URIErrorObject(ExecutionContext *ctx);
    virtual QString className() { return QStringLiteral("URIError"); }
};

struct ArgumentsObject: Object {
    ExecutionContext *context;
    int currentIndex;
    ArgumentsObject(ExecutionContext *context, int formalParameterCount, int actualParameterCount);
    virtual QString className() { return QStringLiteral("Arguments"); }
    virtual ArgumentsObject *asArgumentsObject() { return this; }

    virtual Value __get__(ExecutionContext *ctx, uint index, bool *hasProperty = 0);
    virtual void __put__(ExecutionContext *ctx, uint index, Value value);

    static Value method_getArg(ExecutionContext *ctx);
    static Value method_setArg(ExecutionContext *ctx);
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
