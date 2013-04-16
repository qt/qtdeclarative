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
#ifndef QV4ENGINE_H
#define QV4ENGINE_H

#include "qv4global_p.h"
#include "qv4isel_p.h"
#include "qv4object_p.h"
#include "qv4util_p.h"
#include "qv4context_p.h"
#include "qv4property_p.h"
#include <setjmp.h>

#include <wtf/BumpPointerAllocator.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

namespace Debugging {
class Debugger;
} // namespace Debugging

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
struct BoundFunction;
struct RegExpObject;
struct ErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;
class UnwindHelper;
class ExecutableAllocator;

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
struct EvalFunction;
struct Identifiers;
struct InternalClass;

class RegExp;
class RegExpCache;

typedef bool (*ExternalResourceComparison)(const VM::Value &a, const VM::Value &b);

struct Q_QML_EXPORT ExecutionEngine
{
    MemoryManager *memoryManager;
    ExecutableAllocator *executableAllocator;
    QScopedPointer<EvalISelFactory> iselFactory;

    ExecutionContext *current;
    GlobalContext *rootContext;

    WTF::BumpPointerAllocator bumperPointerAllocator; // Used by Yarr Regex engine.

    Identifiers *identifierCache;

    Debugging::Debugger *debugger;

    Object *globalObject;

    VM::Function *globalCode;

    Value objectCtor;
    Value stringCtor;
    Value numberCtor;
    Value booleanCtor;
    Value arrayCtor;
    Value functionCtor;
    Value dateCtor;
    Value regExpCtor;
    Value errorCtor;
    Value evalErrorCtor;
    Value rangeErrorCtor;
    Value referenceErrorCtor;
    Value syntaxErrorCtor;
    Value typeErrorCtor;
    Value uRIErrorCtor;

    ObjectPrototype *objectPrototype;
    StringPrototype *stringPrototype;
    NumberPrototype *numberPrototype;
    BooleanPrototype *booleanPrototype;
    ArrayPrototype *arrayPrototype;
    FunctionPrototype *functionPrototype;
    DatePrototype *datePrototype;
    RegExpPrototype *regExpPrototype;
    ErrorPrototype *errorPrototype;
    EvalErrorPrototype *evalErrorPrototype;
    RangeErrorPrototype *rangeErrorPrototype;
    ReferenceErrorPrototype *referenceErrorPrototype;
    SyntaxErrorPrototype *syntaxErrorPrototype;
    TypeErrorPrototype *typeErrorPrototype;
    URIErrorPrototype *uRIErrorPrototype;

    InternalClass *emptyClass;
    InternalClass *arrayClass;

    EvalFunction *evalFunction;

    QVector<Property> argumentsAccessors;

    String *id_undefined;
    String *id_null;
    String *id_true;
    String *id_false;
    String *id_boolean;
    String *id_number;
    String *id_string;
    String *id_object;
    String *id_function;
    String *id_length;
    String *id_prototype;
    String *id_constructor;
    String *id_arguments;
    String *id_caller;
    String *id_this;
    String *id___proto__;
    String *id_enumerable;
    String *id_configurable;
    String *id_writable;
    String *id_value;
    String *id_get;
    String *id_set;
    String *id_eval;

    QVector<Function *> functions;

    ExternalResourceComparison externalResourceComparison;

    RegExpCache *regExpCache;

    ExecutionEngine(EvalISelFactory *iselFactory = 0);
    ~ExecutionEngine();

    WithContext *newWithContext(Object *with);
    CatchContext *newCatchContext(String* exceptionVarName, const QQmlJS::VM::Value &exceptionValue);
    CallContext *newCallContext(FunctionObject *f, const QQmlJS::VM::Value &thisObject, QQmlJS::VM::Value *args, int argc);
    CallContext *newCallContext(void *stackSpace, FunctionObject *f, const QQmlJS::VM::Value &thisObject, QQmlJS::VM::Value *args, int argc);
    ExecutionContext *pushGlobalContext();
    void pushContext(SimpleCallContext *context);
    ExecutionContext *popContext();

    VM::Function *newFunction(const QString &name);

    FunctionObject *newBuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *));
    FunctionObject *newScriptFunction(ExecutionContext *scope, VM::Function *function);
    BoundFunction *newBoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);

    Object *newObject();

    String *newString(const QString &s);
    String *newIdentifier(const QString &text);

    Object *newStringObject(ExecutionContext *ctx, const Value &value);
    Object *newNumberObject(const Value &value);
    Object *newBooleanObject(const Value &value);
    Object *newFunctionObject(ExecutionContext *ctx);

    ArrayObject *newArrayObject(ExecutionContext *ctx);

    Object *newDateObject(const Value &value);

    RegExpObject *newRegExpObject(const QString &pattern, int flags);
    RegExpObject *newRegExpObject(RegExp* re, bool global);

    Object *newErrorObject(const Value &value);
    Object *newSyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message);
    Object *newReferenceErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newTypeErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newRangeErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newURIErrorObject(ExecutionContext *ctx, Value message);

    Object *newForEachIteratorObject(ExecutionContext *ctx, Object *o);

    void requireArgumentsAccessors(int n);

    void markObjects();

    Value run(VM::Function *function, ExecutionContext *ctx = 0);

    void initRootContext();
};

inline void ExecutionEngine::pushContext(SimpleCallContext *context)
{
    context->parent = current;
    current = context;
}

inline ExecutionContext *ExecutionEngine::popContext()
{
    CallContext *c = current->asCallContext();
    if (c && !c->needsOwnArguments()) {
        c->arguments = 0;
        c->argumentCount = 0;
    }

    current = current->parent;
    return current;
}


} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ENGINE_H
