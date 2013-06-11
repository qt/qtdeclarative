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
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"
#include "qv4object_p.h"
#include "qv4string_p.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4objectiterator_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Value;
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
    // Used with Managed::subType
    enum FunctionType {
        RegularFunction = 0,
        WrappedQtMethod = 1
    };

    ExecutionContext *scope;
    String *name;
    String * const *formalParameterList;
    String * const *varList;
    unsigned int formalParameterCount;
    unsigned int varCount;
    Function *function;

    FunctionObject(ExecutionContext *scope, String *name = 0);

    Value newInstance();

    static Value construct(Managed *that, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);
    inline Value construct(ExecutionContext *context, Value *args, int argc) {
        return vtbl->construct(this, context, args, argc);
    }
    inline Value construct(Value *args, int argc) {
        return vtbl->construct(this, engine()->current, args, argc);
    }
    inline Value call(ExecutionContext *context, const Value &thisObject, Value *args, int argc) {
        return vtbl->call(this, context, thisObject, args, argc);
    }
    inline Value call(const Value &thisObject, Value *args, int argc) {
        return vtbl->call(this, engine()->current, thisObject, args, argc);
    }

protected:
    static const ManagedVTable static_vtbl;
    static void markObjects(Managed *that);
    static bool hasInstance(Managed *that, ExecutionContext *ctx, const Value &value);
};

struct FunctionCtor: FunctionObject
{
    FunctionCtor(ExecutionContext *scope);

    static Value construct(Managed *that, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(ExecutionContext *ctx);
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(SimpleCallContext *ctx);
    static Value method_apply(SimpleCallContext *ctx);
    static Value method_call(SimpleCallContext *ctx);
    static Value method_bind(SimpleCallContext *ctx);
};

struct BuiltinFunctionOld: FunctionObject {
    Value (*code)(SimpleCallContext *);

    BuiltinFunctionOld(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *));

    static Value construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct IndexedBuiltinFunction: FunctionObject
{
    Q_MANAGED

    Value (*code)(SimpleCallContext *ctx, uint index);
    uint index;

    IndexedBuiltinFunction(ExecutionContext *scope, uint index, Value (*code)(SimpleCallContext *ctx, uint index))
        : FunctionObject(scope, name)
        , code(code)
        , index(index)
    {
        vtbl = &static_vtbl;
        isBuiltinFunction = true;
    }

    static Value construct(Managed *, ExecutionContext *ctx, Value *, int)
    {
        ctx->throwTypeError();
        return Value::undefinedValue();
    }

    static Value call(Managed *that, ExecutionContext *ctx, const Value &thisObject, Value *args, int argc);
};


struct ScriptFunction: FunctionObject {
    ScriptFunction(ExecutionContext *scope, Function *function);

    static Value construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct BoundFunction: FunctionObject {
    FunctionObject *target;
    Value boundThis;
    QVector<Value> boundArgs;

    BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);
    ~BoundFunction() {}


    static Value construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

    static const ManagedVTable static_vtbl;
    static void destroy(Managed *);
    static void markObjects(Managed *that);
    static bool hasInstance(Managed *that, ExecutionContext *ctx, const Value &value);
};

class Q_QML_EXPORT MemberAccessorGetterSetter : public FunctionObject
{
public:
    typedef Value (* GetterSetterFunction)(QV4::SimpleCallContext *ctx);

    MemberAccessorGetterSetter(ExecutionContext *scope, GetterSetterFunction getterSetter, int managedType)
        : FunctionObject(scope)
        , managedType(managedType)
    {
        this->vtbl = &static_vtbl;
        this->getterSetter = getterSetter;
    }

    static QV4::Value call(Managed *that, ExecutionContext *context, const QV4::Value &thisObject, QV4::Value *args, int argc);
protected:
    GetterSetterFunction getterSetter;
    const int managedType;
    static const ManagedVTable static_vtbl;
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
