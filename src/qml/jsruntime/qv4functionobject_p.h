/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"
#include "qv4object_p.h"
#include "qv4string_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4function_p.h"
#include "qv4objectiterator_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Function;
struct Object;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
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
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;
struct InternalClass;
struct Lookup;

struct Q_QML_EXPORT FunctionObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(FunctionObject)
    enum {
        IsFunctionObject = true
    };
    // Used with Managed::subType
    enum FunctionType {
        RegularFunction = 0,
        WrappedQtMethod = 1,
        BoundFunction
    };

    enum {
        Index_Prototype = 0,
        Index_ProtoConstructor = 0
    };

    ExecutionContext *scope;
    ReturnedValue name();
    unsigned int formalParameterCount() { return function ? function->compiledFunction->nFormals : 0; }
    unsigned int varCount() { return function ? function->compiledFunction->nLocals : 0; }
    Function *function;

    FunctionObject(ExecutionContext *scope, const StringRef name, bool createProto = false);
    FunctionObject(ExecutionContext *scope, const QString &name = QString(), bool createProto = false);
    FunctionObject(ExecutionContext *scope, const ReturnedValue name);
    ~FunctionObject();

    void init(const StringRef name, bool createProto);

    ReturnedValue newInstance();

    using Object::construct;
    using Object::call;
    static ReturnedValue construct(Managed *that, CallData *);
    static ReturnedValue call(Managed *that, CallData *d);

    static FunctionObject *cast(const Value &v) {
        return v.asFunctionObject();
    }

    static FunctionObject *createScriptFunction(ExecutionContext *scope, Function *function, bool createProto = true);

    ReturnedValue protoProperty() { return memberData[Index_Prototype].asReturnedValue(); }

protected:
    FunctionObject(InternalClass *ic);

    static void markObjects(Managed *that, ExecutionEngine *e);
    static void destroy(Managed *that)
    { static_cast<FunctionObject*>(that)->~FunctionObject(); }
};

template<>
inline FunctionObject *value_cast(const Value &v) {
    return v.asFunctionObject();
}

DEFINE_REF(FunctionObject, Object);

struct FunctionCtor: FunctionObject
{
    V4_OBJECT
    FunctionCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *that, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(InternalClass *ic);
    void init(ExecutionEngine *engine, ObjectRef ctor);

    static ReturnedValue method_toString(CallContext *ctx);
    static ReturnedValue method_apply(CallContext *ctx);
    static ReturnedValue method_call(CallContext *ctx);
    static ReturnedValue method_bind(CallContext *ctx);
};

struct BuiltinFunction: FunctionObject {
    V4_OBJECT
    ReturnedValue (*code)(CallContext *);

    BuiltinFunction(ExecutionContext *scope, const StringRef name, ReturnedValue (*code)(CallContext *));

    static ReturnedValue construct(Managed *, CallData *);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct IndexedBuiltinFunction: FunctionObject
{
    V4_OBJECT

    ReturnedValue (*code)(CallContext *ctx, uint index);
    uint index;

    IndexedBuiltinFunction(ExecutionContext *scope, uint index, ReturnedValue (*code)(CallContext *ctx, uint index))
        : FunctionObject(scope)
        , code(code)
        , index(index)
    {
        setVTable(staticVTable());
    }

    static ReturnedValue construct(Managed *m, CallData *)
    {
        return m->engine()->currentContext()->throwTypeError();
    }

    static ReturnedValue call(Managed *that, CallData *callData);
};


struct SimpleScriptFunction: FunctionObject {
    V4_OBJECT
    SimpleScriptFunction(ExecutionContext *scope, Function *function, bool createProto);

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);

    InternalClass *internalClassForConstructor();
};

struct ScriptFunction: SimpleScriptFunction {
    V4_OBJECT
    ScriptFunction(ExecutionContext *scope, Function *function);

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};


struct BoundFunction: FunctionObject {
    V4_OBJECT
    FunctionObject *target;
    Value boundThis;
    QVector<Value> boundArgs;

    BoundFunction(ExecutionContext *scope, FunctionObjectRef target, const ValueRef boundThis, const QVector<Value> &boundArgs);
    ~BoundFunction() {}


    static ReturnedValue construct(Managed *, CallData *d);
    static ReturnedValue call(Managed *that, CallData *dd);

    static void destroy(Managed *);
    static void markObjects(Managed *that, ExecutionEngine *e);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
