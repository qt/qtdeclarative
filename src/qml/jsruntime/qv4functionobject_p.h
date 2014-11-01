/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

#include "qv4object_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include "qv4mm_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_EXPORT FunctionObject: Object {
    struct Q_QML_PRIVATE_EXPORT Data : Object::Data {
        Data(ExecutionContext *scope, String *name, bool createProto = false);
        Data(ExecutionContext *scope, const QString &name = QString(), bool createProto = false);
        Data(ExecutionContext *scope, const ReturnedValue name);
        Data(InternalClass *ic);
        ~Data();

        ExecutionContext *scope;
        Function *function;
    };
    V4_OBJECT(Object)
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


    ExecutionContext *scope() { return d()->scope; }
    Function *function() { return d()->function; }

    ReturnedValue name();
    unsigned int formalParameterCount() { return function() ? function()->compiledFunction->nFormals : 0; }
    unsigned int varCount() { return function() ? function()->compiledFunction->nLocals : 0; }

    void init(String *name, bool createProto);

    ReturnedValue newInstance();

    using Object::construct;
    using Object::call;
    static ReturnedValue construct(Managed *that, CallData *);
    static ReturnedValue call(Managed *that, CallData *d);
    static void destroy(Managed *m) {
        static_cast<FunctionObject *>(m)->d()->~Data();
    }

    static FunctionObject *cast(const Value &v) {
        return v.asFunctionObject();
    }

    static Returned<FunctionObject> *createScriptFunction(ExecutionContext *scope, Function *function, bool createProto = true);

    ReturnedValue protoProperty() { return memberData()->data()[Index_Prototype].asReturnedValue(); }

    bool needsActivation() const { return d()->needsActivation; }
    bool strictMode() const { return d()->strictMode; }
    bool bindingKeyFlag() const { return d()->bindingKeyFlag; }

    static void markObjects(HeapObject *that, ExecutionEngine *e);
};

template<>
inline FunctionObject *value_cast(const Value &v) {
    return v.asFunctionObject();
}

struct FunctionCtor: FunctionObject
{
    struct Data : FunctionObject::Data {
        Data(ExecutionContext *scope);
    };

    V4_OBJECT(FunctionObject)

    static ReturnedValue construct(Managed *that, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct FunctionPrototype: FunctionObject
{
    struct Data : FunctionObject::Data {
        Data(InternalClass *ic);
    };
    V4_OBJECT(FunctionObject)

    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_toString(CallContext *ctx);
    static ReturnedValue method_apply(CallContext *ctx);
    static ReturnedValue method_call(CallContext *ctx);
    static ReturnedValue method_bind(CallContext *ctx);
};

struct Q_QML_EXPORT BuiltinFunction: FunctionObject {
    struct Q_QML_EXPORT Data : FunctionObject::Data {
        Data(ExecutionContext *scope, String *name, ReturnedValue (*code)(CallContext *));
        ReturnedValue (*code)(CallContext *);
    };
    V4_OBJECT(FunctionObject)

    static Returned<BuiltinFunction> *create(ExecutionContext *scope, String *name, ReturnedValue (*code)(CallContext *))
    {
        return scope->engine()->memoryManager->alloc<BuiltinFunction>(scope, name, code);
    }

    static ReturnedValue construct(Managed *, CallData *);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct IndexedBuiltinFunction: FunctionObject
{
    struct Data : FunctionObject::Data {
        Data(ExecutionContext *scope, uint index, ReturnedValue (*code)(CallContext *ctx, uint index))
            : FunctionObject::Data(scope),
              code(code)
            , index(index)
        {
            setVTable(staticVTable());
        }
        ReturnedValue (*code)(CallContext *, uint index);
        uint index;
    };
    V4_OBJECT(FunctionObject)

    static ReturnedValue construct(Managed *m, CallData *)
    {
        return m->engine()->throwTypeError();
    }

    static ReturnedValue call(Managed *that, CallData *callData);
};


struct SimpleScriptFunction: FunctionObject {
    struct Data : FunctionObject::Data {
        Data(ExecutionContext *scope, Function *function, bool createProto);
    };
    V4_OBJECT(FunctionObject)

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);

    InternalClass *internalClassForConstructor();
};

struct ScriptFunction: SimpleScriptFunction {
    struct Data : SimpleScriptFunction::Data {
        Data(ExecutionContext *scope, Function *function);
    };
    V4_OBJECT(FunctionObject)

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};


struct BoundFunction: FunctionObject {
    struct Data : FunctionObject::Data {
        Data(ExecutionContext *scope, FunctionObject *target, const ValueRef boundThis, MemberData *boundArgs);
        FunctionObject *target;
        Value boundThis;
        MemberData::Data *boundArgs;
    };
    V4_OBJECT(FunctionObject)

    static Returned<BoundFunction> *create(ExecutionContext *scope, FunctionObject *target, const ValueRef boundThis, QV4::MemberData *boundArgs)
    {
        return scope->engine()->memoryManager->alloc<BoundFunction>(scope, target, boundThis, boundArgs);
    }

    FunctionObject *target() { return d()->target; }
    Value boundThis() const { return d()->boundThis; }
    // ### GC
    MemberData::Data *boundArgs() const { return d()->boundArgs; }

    static ReturnedValue construct(Managed *, CallData *d);
    static ReturnedValue call(Managed *that, CallData *dd);

    static void markObjects(HeapObject *that, ExecutionEngine *e);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
