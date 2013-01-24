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
#ifndef QMLJS_ENGINE_H
#define QMLJS_ENGINE_H

#include <qv4isel_p.h>
#include <qv4object.h>
#include <qmljs_environment.h>
#include <setjmp.h>

#include <wtf/PassRefPtr.h>
#include <wtf/BumpPointerAllocator.h>

namespace QQmlJS {

namespace Debugging {
class Debugger;
} // namespace Debugging

namespace VM {

struct Value;
class Array;
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

class RegExp;

struct ExecutionEngine
{
    MemoryManager *memoryManager;
    EvalISelFactory *iselFactory;
    ExecutionContext *current;
    ExecutionContext *rootContext;
    WTF::BumpPointerAllocator bumperPointerAllocator; // Used by Yarr Regex engine.

    Debugging::Debugger *debugger;

    Value globalObject;

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

    EvalFunction *evalFunction;

    QVector<PropertyDescriptor> argumentsAccessors;

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

    struct ExceptionHandler {
        ExecutionContext *context;
        const uchar *code; // Interpreter state
        int targetTempIndex; // Interpreter state
        jmp_buf stackFrame;
    };

    QVector<ExceptionHandler> unwindStack;
    Value exception;

    QScopedPointer<class StringPool> stringPool;
    QVector<Function *> functions;

    ExecutionEngine(EvalISelFactory *iselFactory);
    ~ExecutionEngine();

    ExecutionContext *newContext();

    String *identifier(const QString &s);

    VM::Function *newFunction(const QString &name);

    FunctionObject *newBuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *));
    FunctionObject *newScriptFunction(ExecutionContext *scope, VM::Function *function);
    BoundFunction *newBoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);

    Object *newObject();
    FunctionObject *newObjectCtor(ExecutionContext *ctx);

    String *newString(const QString &s);
    Object *newStringObject(ExecutionContext *ctx, const Value &value);
    FunctionObject *newStringCtor(ExecutionContext *ctx);

    Object *newNumberObject(const Value &value);
    FunctionObject *newNumberCtor(ExecutionContext *ctx);

    Object *newBooleanObject(const Value &value);
    FunctionObject *newBooleanCtor(ExecutionContext *ctx);

    Object *newFunctionObject(ExecutionContext *ctx);

    ArrayObject *newArrayObject(ExecutionContext *ctx);
    ArrayObject *newArrayObject(ExecutionContext *ctx, const Array &value);
    FunctionObject *newArrayCtor(ExecutionContext *ctx);

    Object *newDateObject(const Value &value);
    FunctionObject *newDateCtor(ExecutionContext *ctx);

    RegExpObject *newRegExpObject(const QString &pattern, int flags);
    RegExpObject *newRegExpObject(PassRefPtr<RegExp> re, bool global);
    FunctionObject *newRegExpCtor(ExecutionContext *ctx);

    Object *newErrorObject(const Value &value);
    Object *newSyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message);
    Object *newReferenceErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newTypeErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newRangeErrorObject(ExecutionContext *ctx, const QString &message);
    Object *newURIErrorObject(ExecutionContext *ctx, Value message);

    Object *newMathObject(ExecutionContext *ctx);
    Object *newActivationObject();

    Object *newForEachIteratorObject(ExecutionContext *ctx, Object *o);

    void requireArgumentsAccessors(int n);
};

} // namespace VM
} // namespace QQmlJS

#endif
