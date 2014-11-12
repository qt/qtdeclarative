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
#ifndef QMLJS_ENVIRONMENT_H
#define QMLJS_ENVIRONMENT_H

#include "qv4global_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4managed_p.h"
#include "qv4engine_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace CompiledData {
struct CompilationUnit;
struct Function;
}

struct CallContext;
struct CatchContext;
struct WithContext;

namespace Heap {

struct ExecutionContext : Base {
    enum ContextType {
        Type_GlobalContext = 0x1,
        Type_CatchContext = 0x2,
        Type_WithContext = 0x3,
        Type_SimpleCallContext = 0x4,
        Type_CallContext = 0x5,
        Type_QmlContext = 0x6
    };
    struct EvalCode
    {
        Function *function;
        EvalCode *next;
    };

    inline ExecutionContext(ExecutionEngine *engine, ContextType t);

    ContextType type;
    bool strictMode;

    CallData *callData;

    ExecutionEngine *engine;
    ExecutionContext *parent;
    ExecutionContext *outer;
    Lookup *lookups;
    CompiledData::CompilationUnit *compilationUnit;
    EvalCode *currentEvalCode;

    int lineNumber;

};

struct CallContext : ExecutionContext {
    CallContext(ExecutionEngine *engine, ContextType t = Type_SimpleCallContext)
        : ExecutionContext(engine, t)
    {
        function = 0;
        locals = 0;
        activation = 0;
    }
    CallContext(ExecutionEngine *engine, QV4::Object *qml, QV4::FunctionObject *function);

    FunctionObject *function;
    int realArgumentCount;
    Value *locals;
    Object *activation;
};

struct GlobalContext : ExecutionContext {
    GlobalContext(ExecutionEngine *engine);
    Object *global;
};

struct CatchContext : ExecutionContext {
    CatchContext(ExecutionEngine *engine, QV4::String *exceptionVarName, const ValueRef exceptionValue);
    StringValue exceptionVarName;
    Value exceptionValue;
};

struct WithContext : ExecutionContext {
    WithContext(ExecutionEngine *engine, QV4::Object *with);
    Object *withObject;
};


}

struct Q_QML_EXPORT ExecutionContext : public Managed
{
    enum {
        IsExecutionContext = true
    };

    V4_MANAGED(ExecutionContext, Managed)
    Q_MANAGED_TYPE(ExecutionContext)

    Heap::CallContext *newCallContext(FunctionObject *f, CallData *callData);
    Heap::WithContext *newWithContext(Object *with);
    Heap::CatchContext *newCatchContext(String *exceptionVarName, const ValueRef exceptionValue);
    Heap::CallContext *newQmlContext(FunctionObject *f, Object *qml);

    void createMutableBinding(String *name, bool deletable);

    void setProperty(String *name, const ValueRef value);
    ReturnedValue getProperty(String *name);
    ReturnedValue getPropertyAndBase(String *name, Object *&base);
    bool deleteProperty(String *name);

    inline CallContext *asCallContext();
    inline const CallContext *asCallContext() const;

    static void markObjects(Heap::Base *m, ExecutionEngine *e);
};

inline
Heap::ExecutionContext::ExecutionContext(ExecutionEngine *engine, ContextType t)
    : Heap::Base(engine->executionContextClass)
    , type(t)
    , strictMode(false)
    , engine(engine)
    , parent(engine->currentContext()->d())
    , outer(0)
    , lookups(0)
    , compilationUnit(0)
    , currentEvalCode(0)
    , lineNumber(-1)
{
    // ### GC
    engine->current = reinterpret_cast<QV4::ExecutionContext *>(this);
}


struct CallContext : public ExecutionContext
{
    V4_MANAGED(CallContext, ExecutionContext)

    // formals are in reverse order
    String * const *formals() const;
    unsigned int formalCount() const;
    String * const *variables() const;
    unsigned int variableCount() const;

    inline ReturnedValue argument(int i);
    bool needsOwnArguments() const;
};

inline ReturnedValue CallContext::argument(int i) {
    return i < d()->callData->argc ? d()->callData->args[i].asReturnedValue() : Primitive::undefinedValue().asReturnedValue();
}

struct GlobalContext : public ExecutionContext
{
    V4_MANAGED(GlobalContext, ExecutionContext)

};

struct CatchContext : public ExecutionContext
{
    V4_MANAGED(CatchContext, ExecutionContext)
};

struct WithContext : public ExecutionContext
{
    V4_MANAGED(WithContext, ExecutionContext)
};

inline CallContext *ExecutionContext::asCallContext()
{
    return d()->type >= Heap::ExecutionContext::Type_SimpleCallContext ? static_cast<CallContext *>(this) : 0;
}

inline const CallContext *ExecutionContext::asCallContext() const
{
    return d()->type >= Heap::ExecutionContext::Type_SimpleCallContext ? static_cast<const CallContext *>(this) : 0;
}


inline void ExecutionEngine::pushContext(CallContext *context)
{
    Q_ASSERT(current && current->d() && context && context->d());
    context->d()->parent = current->d();
    current = context;
    current->d()->currentEvalCode = 0;
}

inline ExecutionContext *ExecutionEngine::popContext()
{
    Q_ASSERT(current->d()->parent);
    // ### GC
    current = reinterpret_cast<ExecutionContext *>(current->d()->parent);
    Q_ASSERT(current && current->d());
    return current;
}

struct ExecutionContextSaver
{
    ExecutionEngine *engine;
    ExecutionContext *savedContext;

    ExecutionContextSaver(ExecutionContext *context)
        : engine(context->d()->engine)
        , savedContext(context)
    {
    }
    ~ExecutionContextSaver()
    {
        engine->current = savedContext;
    }
};

inline Scope::Scope(ExecutionContext *ctx)
    : engine(ctx->d()->engine)
#ifndef QT_NO_DEBUG
    , size(0)
#endif
{
    mark = engine->jsStackTop;
}

/* Function *f, int argc */
#define requiredMemoryForExecutionContect(f, argc) \
    ((sizeof(CallContext::Data) + 7) & ~7) + sizeof(Value) * (f->varCount() + qMax((uint)argc, f->formalParameterCount())) + sizeof(CallData)

} // namespace QV4

QT_END_NAMESPACE

#endif
