/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4object_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include <private/qv4mm_p.h>

QT_BEGIN_NAMESPACE

struct QQmlSourceLocation;

namespace QV4 {

struct IndexedBuiltinFunction;
struct JSCallData;

namespace Heap {


#define FunctionObjectMembers(class, Member) \
    Member(class, Pointer, ExecutionContext *, scope) \
    Member(class, NoMark, Function *, function) \
    Member(class, NoMark, VTable::Call, jsCall) \
    Member(class, NoMark, VTable::CallAsConstructor, jsConstruct)

DECLARE_HEAP_OBJECT(FunctionObject, Object) {
    DECLARE_MARKOBJECTS(FunctionObject);
    enum {
        Index_ProtoConstructor = 0
    };

    bool isConstructor() const {
        return jsConstruct != nullptr;
    }

    Q_QML_PRIVATE_EXPORT void init(QV4::ExecutionContext *scope, QV4::String *name, VTable::Call call);
    void init(QV4::ExecutionContext *scope, QV4::String *name = nullptr);
    void init(QV4::ExecutionContext *scope, QV4::Function *function, QV4::String *n = nullptr);
    void init(QV4::ExecutionContext *scope, const QString &name);
    void init();
    void destroy();

    void setFunction(Function *f);

    unsigned int formalParameterCount() { return function ? function->nFormals : 0; }
    unsigned int varCount() { return function ? function->compiledFunction->nLocals : 0; }
};

struct FunctionCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

struct FunctionPrototype : FunctionObject {
    void init();
};

struct IndexedBuiltinFunction : FunctionObject {
    inline void init(QV4::ExecutionContext *scope, uint index, VTable::Call call);
    uint index;
};

#define ScriptFunctionMembers(class, Member) \
    Member(class, Pointer, InternalClass *, cachedClassForConstructor)

DECLARE_HEAP_OBJECT(ScriptFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(ScriptFunction)
    enum {
        Index_Name,
        Index_Length
    };
    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr, bool makeConstructor = true);
};

#define MemberFunctionMembers(class, Member) \
    Member(class, Pointer, Object *, homeObject)

DECLARE_HEAP_OBJECT(MemberFunction, ScriptFunction) {
    DECLARE_MARKOBJECTS(MemberFunction)

    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr) {
        ScriptFunction::init(scope, function, name, false);
    }
};

struct ConstructorFunction : MemberFunction
{
    bool isDerivedConstructor;
};

struct DefaultClassConstructorFunction : FunctionObject
{
    bool isDerivedConstructor;
};

#define BoundFunctionMembers(class, Member) \
    Member(class, Pointer, FunctionObject *, target) \
    Member(class, HeapValue, HeapValue, boundThis) \
    Member(class, Pointer, MemberData *, boundArgs)

DECLARE_HEAP_OBJECT(BoundFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(BoundFunction);

    void init(QV4::ExecutionContext *scope, QV4::FunctionObject *target, const Value &boundThis, QV4::MemberData *boundArgs);
};

}

struct Q_QML_EXPORT FunctionObject: Object {
    enum {
        IsFunctionObject = true
    };
    V4_OBJECT2(FunctionObject, Object)
    Q_MANAGED_TYPE(FunctionObject)
    V4_INTERNALCLASS(FunctionObject)
    V4_PROTOTYPE(functionPrototype)
    V4_NEEDS_DESTROY
    enum { NInlineProperties = 1 };

    Heap::ExecutionContext *scope() const { return d()->scope; }
    Function *function() const { return d()->function; }

    ReturnedValue name() const;
    unsigned int formalParameterCount() const { return d()->formalParameterCount(); }
    unsigned int varCount() const { return d()->varCount(); }

    void setName(String *name) {
        defineReadonlyConfigurableProperty(engine()->id_name(), *name);
    }
    void createDefaultPrototypeProperty(uint protoConstructorSlot);

    inline ReturnedValue callAsConstructor(const JSCallData &data) const;
    ReturnedValue callAsConstructor(const Value *argv, int argc, const Value *newTarget = nullptr) const {
        if (!d()->jsConstruct)
            return engine()->throwTypeError(QStringLiteral("Function is not a constructor."));
        return d()->jsConstruct(this, argv, argc, newTarget ? newTarget : this);
    }
    inline ReturnedValue call(const JSCallData &data) const;
    ReturnedValue call(const Value *thisObject, const Value *argv, int argc) const {
        if (!d()->jsCall)
            return engine()->throwTypeError(QStringLiteral("Function can only be called with |new|."));
        return d()->jsCall(this, thisObject, argv, argc);
    }
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    static Heap::FunctionObject *createScriptFunction(ExecutionContext *scope, Function *function);
    static Heap::FunctionObject *createConstructorFunction(ExecutionContext *scope, Function *function, Object *homeObject, bool isDerivedConstructor);
    static Heap::FunctionObject *createMemberFunction(ExecutionContext *scope, Function *function, Object *homeObject, String *name);
    static Heap::FunctionObject *createBuiltinFunction(ExecutionEngine *engine, StringOrSymbol *nameOrSymbol, VTable::Call code, int argumentCount);

    bool strictMode() const { return d()->function ? d()->function->isStrict() : false; }
    bool isBinding() const;
    bool isBoundFunction() const;
    bool isConstructor() const {
        return d()->isConstructor();
    }

    ReturnedValue protoProperty() const { return get(engine()->id_prototype()); }


    QQmlSourceLocation sourceLocation() const;
};

template<>
inline const FunctionObject *Value::as() const {
    return isManaged() && m()->internalClass->vtable->isFunctionObject ? reinterpret_cast<const FunctionObject *>(this) : nullptr;
}


struct FunctionCtor: FunctionObject
{
    V4_OBJECT2(FunctionCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
protected:
    enum Type {
        Type_Function,
        Type_Generator
    };
    static QQmlRefPointer<CompiledData::CompilationUnit> parse(ExecutionEngine *engine, const Value *argv, int argc, Type t = Type_Function);
};

struct FunctionPrototype: FunctionObject
{
    V4_OBJECT2(FunctionPrototype, FunctionObject)

    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_apply(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_call(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_bind(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_hasInstance(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct IndexedBuiltinFunction : FunctionObject
{
    V4_OBJECT2(IndexedBuiltinFunction, FunctionObject)
};

void Heap::IndexedBuiltinFunction::init(QV4::ExecutionContext *scope, uint index, VTable::Call call)
{
    Heap::FunctionObject::init(scope);
    this->jsCall = call;
    this->index = index;
}


struct ScriptFunction : FunctionObject {
    V4_OBJECT2(ScriptFunction, FunctionObject)
    V4_INTERNALCLASS(ScriptFunction)
    enum { NInlineProperties = 3 };

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    Heap::InternalClass *classForConstructor() const;
};

struct MemberFunction : ScriptFunction {
    V4_OBJECT2(MemberFunction, ScriptFunction)
    V4_INTERNALCLASS(MemberFunction)
};

struct ConstructorFunction : MemberFunction {
    V4_OBJECT2(ConstructorFunction, MemberFunction)
    V4_INTERNALCLASS(ConstructorFunction)
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct DefaultClassConstructorFunction : FunctionObject {
    V4_OBJECT2(DefaultClassConstructorFunction, FunctionObject)
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct BoundFunction: FunctionObject {
    V4_OBJECT2(BoundFunction, FunctionObject)

    static Heap::BoundFunction *create(ExecutionContext *scope, FunctionObject *target, const Value &boundThis, QV4::MemberData *boundArgs)
    {
        return scope->engine()->memoryManager->allocate<BoundFunction>(scope, target, boundThis, boundArgs);
    }

    Heap::FunctionObject *target() const { return d()->target; }
    Value boundThis() const { return d()->boundThis; }
    Heap::MemberData *boundArgs() const { return d()->boundArgs; }

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
