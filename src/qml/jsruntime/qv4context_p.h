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
#ifndef QMLJS_ENVIRONMENT_H
#define QMLJS_ENVIRONMENT_H

#include "qv4global_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4managed_p.h"
#include "qv4engine_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Object;
struct ExecutionEngine;
struct DeclarativeEnvironment;
struct Lookup;
struct Function;
struct ValueRef;

namespace CompiledData {
struct CompilationUnit;
struct Function;
}

struct CallContext;
struct CallContext;
struct CatchContext;
struct WithContext;

struct Q_QML_EXPORT ExecutionContext : public Managed
{
    V4_MANAGED
    Q_MANAGED_TYPE(ExecutionContext)
    enum {
        IsExecutionContext = true
    };

    enum ContextType {
        Type_GlobalContext = 0x1,
        Type_CatchContext = 0x2,
        Type_WithContext = 0x3,
        Type_SimpleCallContext = 0x4,
        Type_CallContext = 0x5,
        Type_QmlContext = 0x6
    };

    ExecutionContext(ExecutionEngine *engine, ContextType t)
        : Managed(engine->executionContextClass)
    {
        this->type = t;
        strictMode = false;
        this->engine = engine;
        this->parent = engine->currentContext();
        outer = 0;
        lookups = 0;
        compilationUnit = 0;
        currentEvalCode = 0;
        lineNumber = -1;
        engine->current = this;
    }

    ContextType type;
    bool strictMode;

    CallData *callData;

    ExecutionEngine *engine;
    ExecutionContext *parent;
    ExecutionContext *outer;
    Lookup *lookups;
    CompiledData::CompilationUnit *compilationUnit;

    struct EvalCode
    {
        Function *function;
        EvalCode *next;
    };
    EvalCode *currentEvalCode;

    int lineNumber;

    CallContext *newCallContext(FunctionObject *f, CallData *callData);
    WithContext *newWithContext(ObjectRef with);
    CatchContext *newCatchContext(const StringRef exceptionVarName, const ValueRef exceptionValue);
    CallContext *newQmlContext(FunctionObject *f, ObjectRef qml);

    void createMutableBinding(const StringRef name, bool deletable);

    ReturnedValue throwError(const QV4::ValueRef value);
    ReturnedValue throwError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message);
    ReturnedValue throwSyntaxError(const QString &message, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwTypeError();
    ReturnedValue throwTypeError(const QString &message);
    ReturnedValue throwReferenceError(const ValueRef value);
    ReturnedValue throwReferenceError(const QString &value, const QString &fileName, int lineNumber, int column);
    ReturnedValue throwRangeError(const ValueRef value);
    ReturnedValue throwRangeError(const QString &message);
    ReturnedValue throwURIError(const ValueRef msg);
    ReturnedValue throwUnimplemented(const QString &message);

    void setProperty(const StringRef name, const ValueRef value);
    ReturnedValue getProperty(const StringRef name);
    ReturnedValue getPropertyAndBase(const StringRef name, ObjectRef base);
    bool deleteProperty(const StringRef name);

    // Can only be called from within catch(...), rethrows if no JS exception.
    ReturnedValue catchException(StackTrace *trace = 0);

    inline CallContext *asCallContext();
    inline const CallContext *asCallContext() const;

    static void markObjects(Managed *m, ExecutionEngine *e);
};

struct CallContext : public ExecutionContext
{
    CallContext(ExecutionEngine *engine, ContextType t = Type_SimpleCallContext)
        : ExecutionContext(engine, t)
    {
        function = 0;
        locals = 0;
        activation = 0;
    }
    CallContext(ExecutionEngine *engine, ObjectRef qml, QV4::FunctionObject *function);

    FunctionObject *function;
    int realArgumentCount;
    Value *locals;
    Object *activation;

    // formals are in reverse order
    String * const *formals() const;
    unsigned int formalCount() const;
    String * const *variables() const;
    unsigned int variableCount() const;

    inline ReturnedValue argument(int i);
    bool needsOwnArguments() const;
};

inline ReturnedValue CallContext::argument(int i) {
    return i < callData->argc ? callData->args[i].asReturnedValue() : Primitive::undefinedValue().asReturnedValue();
}

struct GlobalContext : public ExecutionContext
{
    GlobalContext(ExecutionEngine *engine);

    Object *global;
};

struct CatchContext : public ExecutionContext
{
    CatchContext(ExecutionEngine *engine, const StringRef exceptionVarName, const ValueRef exceptionValue);

    StringValue exceptionVarName;
    Value exceptionValue;
};

struct WithContext : public ExecutionContext
{
    WithContext(ExecutionEngine *engine, ObjectRef with);
    Object *withObject;
};

inline CallContext *ExecutionContext::asCallContext()
{
    return type >= Type_SimpleCallContext ? static_cast<CallContext *>(this) : 0;
}

inline const CallContext *ExecutionContext::asCallContext() const
{
    return type >= Type_SimpleCallContext ? static_cast<const CallContext *>(this) : 0;
}


inline void ExecutionEngine::pushContext(CallContext *context)
{
    context->parent = current;
    current = context;
    current->currentEvalCode = 0;
}

inline ExecutionContext *ExecutionEngine::popContext()
{
    Q_ASSERT(current->parent);
    current = current->parent;
    return current;
}

struct ExecutionContextSaver
{
    ExecutionEngine *engine;
    ExecutionContext *savedContext;

    ExecutionContextSaver(ExecutionContext *context)
        : engine(context->engine)
        , savedContext(context)
    {
    }
    ~ExecutionContextSaver()
    {
        engine->current = savedContext;
    }
};

inline Scope::Scope(ExecutionContext *ctx)
    : engine(ctx->engine)
#ifndef QT_NO_DEBUG
    , size(0)
#endif
{
    mark = engine->jsStackTop;
}

/* Function *f, int argc */
#define requiredMemoryForExecutionContect(f, argc) \
    ((sizeof(CallContext) + 7) & ~7) + sizeof(Value) * (f->varCount() + qMax((uint)argc, f->formalParameterCount())) + sizeof(CallData)

} // namespace QV4

QT_END_NAMESPACE

#endif
