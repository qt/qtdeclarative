/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QV4GENERATOROBJECT_P_H
#define QV4GENERATOROBJECT_P_H

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

#include "qv4functionobject_p.h"
#include "qv4stackframe_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

enum class GeneratorState {
    Undefined,
    SuspendedStart,
    SuspendedYield,
    Executing,
    Completed
};

namespace Heap {

struct GeneratorFunctionCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

struct GeneratorFunction : ArrowFunction {
    void init(QV4::ExecutionContext *scope, Function *function, QV4::String *name = nullptr) {
        ArrowFunction::init(scope, function, name);
    }
};

struct MemberGeneratorFunction : MemberFunction {
};

struct GeneratorPrototype : FunctionObject {
    void init();
};

#define GeneratorObjectMembers(class, Member) \
    Member(class, Pointer, ExecutionContext *, context) \
    Member(class, Pointer, GeneratorFunction *, function) \
    Member(class, NoMark, GeneratorState, state) \
    Member(class, NoMark, CppStackFrame, cppFrame) \
    Member(class, Pointer, ArrayObject *, values) \
    Member(class, Pointer, ArrayObject *, jsFrame)

DECLARE_HEAP_OBJECT(GeneratorObject, Object) {
    DECLARE_MARKOBJECTS(GeneratorObject);
};

}

struct GeneratorFunctionCtor : FunctionCtor
{
    V4_OBJECT2(GeneratorFunctionCtor, FunctionCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct GeneratorFunction : ArrowFunction
{
    V4_OBJECT2(GeneratorFunction, ArrowFunction)
    V4_INTERNALCLASS(GeneratorFunction)

    static Heap::FunctionObject *create(ExecutionContext *scope, Function *function);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct MemberGeneratorFunction : MemberFunction
{
    V4_OBJECT2(MemberGeneratorFunction, MemberFunction)
    V4_INTERNALCLASS(MemberGeneratorFunction)

    static Heap::FunctionObject *create(ExecutionContext *scope, Function *function, Object *homeObject, String *name);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct GeneratorPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_next(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_return(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_throw(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


struct GeneratorObject : Object {
    V4_OBJECT2(GeneratorObject, Object)
    Q_MANAGED_TYPE(GeneratorObject)
    V4_INTERNALCLASS(GeneratorObject)
    V4_PROTOTYPE(generatorPrototype)

    ReturnedValue resume(ExecutionEngine *engine, const Value &arg) const;
};

}

QT_END_NAMESPACE

#endif // QV4GENERATORFUNCTION_P_H

