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

QT_BEGIN_NAMESPACE

namespace QV4 {

struct JSCallData {
    JSCallData(const Scope &scope, int argc = 0, const Value *argv = nullptr, const Value *thisObject = nullptr)
        : scope(scope), argc(argc)
    {
        if (thisObject)
            this->thisObject = const_cast<Value *>(thisObject);
        else
            this->thisObject = scope.alloc();
        if (argv)
            this->args = const_cast<Value *>(argv);
        else
            this->args = scope.alloc(argc);
    }

    JSCallData *operator->() {
        return this;
    }

    CallData *callData(const FunctionObject *f = nullptr) const {
        int size = int(offsetof(QV4::CallData, args)/sizeof(QV4::Value)) + argc;
        CallData *ptr = reinterpret_cast<CallData *>(scope.alloc<Scope::Uninitialized>(size));
        ptr->function = Encode::undefined();
        ptr->context = Encode::undefined();
        ptr->accumulator = Encode::undefined();
        ptr->thisObject = thisObject->asReturnedValue();
        ptr->newTarget = Encode::undefined();
        ptr->setArgc(argc);
        if (argc)
            memcpy(ptr->args, args, argc*sizeof(Value));
        if (f)
            ptr->function = f->asReturnedValue();
        return ptr;
    }
    const Scope &scope;
    int argc;
    Value *args;
    Value *thisObject;
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


struct ScopedStackFrame {
    Scope &scope;
    CppStackFrame frame;

    ScopedStackFrame(Scope &scope, Heap::ExecutionContext *context)
        : scope(scope)
    {
        frame.parent = scope.engine->currentStackFrame;
        if (!context)
            return;
        frame.jsFrame = reinterpret_cast<CallData *>(scope.alloc(sizeof(CallData)/sizeof(Value)));
        frame.jsFrame->context = context;
        frame.v4Function = frame.parent ? frame.parent->v4Function : nullptr;
        scope.engine->currentStackFrame = &frame;
    }
    ~ScopedStackFrame() {
        scope.engine->currentStackFrame = frame.parent;
    }
};

}

QT_END_NAMESPACE

#endif // QV4JSCALL_H
