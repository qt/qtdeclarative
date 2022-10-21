/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#ifndef QV4JSCALL_H
#define QV4JSCALL_H

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
#include "qv4functionobject_p.h"
#include "qv4context_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4stackframe_p.h"
#include <private/qv4alloca_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

template<typename Args>
CallData *callDatafromJS(const Scope &scope, const Args *args, const FunctionObject *f = nullptr)
{
    int size = int(offsetof(QV4::CallData, args)/sizeof(QV4::Value)) + args->argc;
    CallData *ptr = reinterpret_cast<CallData *>(scope.alloc<Scope::Uninitialized>(size));
    ptr->function = Encode::undefined();
    ptr->context = Encode::undefined();
    ptr->accumulator = Encode::undefined();
    ptr->thisObject = args->thisObject ? args->thisObject->asReturnedValue() : Encode::undefined();
    ptr->newTarget = Encode::undefined();
    ptr->setArgc(args->argc);
    if (args->argc)
        memcpy(ptr->args, args->args, args->argc*sizeof(Value));
    if (f)
        ptr->function = f->asReturnedValue();
    return ptr;
}

struct JSCallArguments
{
    JSCallArguments(const Scope &scope, int argc = 0)
        : thisObject(scope.alloc()), args(scope.alloc(argc)), argc(argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    Value *thisObject;
    Value *args;
    const int argc;
};

struct JSCallData
{
    JSCallData(const Value *thisObject, const Value *argv, int argc)
        : thisObject(thisObject), args(argv), argc(argc)
    {
    }

    Q_IMPLICIT JSCallData(const JSCallArguments &args)
        : thisObject(args.thisObject), args(args.args), argc(args.argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    const Value *thisObject;
    const Value *args;
    const int argc;
};

inline
ReturnedValue FunctionObject::callAsConstructor(const JSCallData &data) const
{
    return callAsConstructor(data.args, data.argc, this);
}

inline
ReturnedValue FunctionObject::call(const JSCallData &data) const
{
    return call(data.thisObject, data.args, data.argc);
}

void populateJSCallArguments(ExecutionEngine *v4, JSCallArguments &jsCall, int argc,
                             void **args, const QMetaType *types);

struct ScopedStackFrame {
    Scope &scope;
    CppStackFrame frame;

    ScopedStackFrame(Scope &scope, Heap::ExecutionContext *context)
        : scope(scope)
    {
        frame.setParentFrame(scope.engine->currentStackFrame);
        if (!context)
            return;
        frame.jsFrame = reinterpret_cast<CallData *>(scope.alloc(sizeof(CallData)/sizeof(Value)));
        frame.jsFrame->context = context;
        if (auto *parent = frame.parentFrame())
            frame.v4Function = parent->v4Function;
        else
            frame.v4Function = nullptr;
        scope.engine->currentStackFrame = &frame;
    }
    ~ScopedStackFrame() {
        scope.engine->currentStackFrame = frame.parentFrame();
    }
};

template<typename Callable>
ReturnedValue convertAndCall(
        ExecutionEngine *engine, const QQmlPrivate::AOTCompiledFunction *aotFunction,
        const Value *thisObject, const Value *argv, int argc, Callable call)
{
    const qsizetype numFunctionArguments = aotFunction->argumentTypes.size();
    Q_ALLOCA_VAR(void *, values, (numFunctionArguments + 1) * sizeof(void *));
    Q_ALLOCA_VAR(QMetaType, types, (numFunctionArguments + 1) * sizeof(QMetaType));

    for (qsizetype i = 0; i < numFunctionArguments; ++i) {
        const QMetaType argumentType = aotFunction->argumentTypes[i];
        types[i + 1] = argumentType;
        if (const qsizetype argumentSize = argumentType.sizeOf()) {
            Q_ALLOCA_VAR(void, argument, argumentSize);
            argumentType.construct(argument);
            if (i < argc)
                engine->metaTypeFromJS(argv[i], argumentType, argument);
            values[i + 1] = argument;
        } else {
            values[i + 1] = nullptr;
        }
    }

    Q_ALLOCA_DECLARE(void, returnValue);
    types[0] = aotFunction->returnType;
    if (const qsizetype returnSize = types[0].sizeOf()) {
        Q_ALLOCA_ASSIGN(void, returnValue, returnSize);
        values[0] = returnValue;
    } else {
        values[0] = nullptr;
    }

    call(thisObject, values, types, argc);

    ReturnedValue result;
    if (values[0]) {
        result = engine->metaTypeToJS(types[0], values[0]);
        types[0].destruct(values[0]);
    } else {
        result = Encode::undefined();
    }

    for (qsizetype i = 1, end = numFunctionArguments + 1; i < end; ++i)
        types[i].destruct(values[i]);

    return result;
}

template<typename Callable>
bool convertAndCall(ExecutionEngine *engine, const Value *thisObject,
                    void **a, const QMetaType *types, int argc, Callable call)
{
    Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, argc);

    for (int ii = 0; ii < argc; ++ii)
        jsCallData.args[ii] = engine->metaTypeToJS(types[ii + 1], a[ii + 1]);

    ScopedValue jsResult(scope, call(thisObject, jsCallData.args, argc));
    void *result = a[0];
    if (!result)
        return !jsResult->isUndefined();

    const QMetaType resultType = types[0];
    if (scope.hasException()) {
        // Clear the return value
        resultType.construct(result);
    } else {
        // When the return type is QVariant, JS objects are to be returned as
        // QJSValue wrapped in QVariant. metaTypeFromJS unwraps them, unfortunately.
        if (resultType == QMetaType::fromType<QVariant>()) {
            new (result) QVariant(scope.engine->toVariant(jsResult, QMetaType {}));
        } else {
            resultType.construct(result);
            scope.engine->metaTypeFromJS(jsResult, resultType, result);
        }
    }
    return !jsResult->isUndefined();
}

}

QT_END_NAMESPACE

#endif // QV4JSCALL_H
