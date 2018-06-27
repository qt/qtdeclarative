/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
#ifndef QV4STACKFRAME_H
#define QV4STACKFRAME_H

#include <private/qv4context_p.h>
#ifndef V4_BOOTSTRAP
#include <private/qv4function_p.h>
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

struct CallData
{
    enum Offsets {
        Function = 0,
        Context = 1,
        Accumulator = 2,
        This = 3,
        NewTarget = 4,
        Argc = 5
    };

    Value function;
    Value context;
    Value accumulator;
    Value thisObject;
    Value newTarget;
    Value _argc;

    int argc() const {
        Q_ASSERT(_argc.isInteger());
        return _argc.int_32();
    }

    void setArgc(int argc) {
        Q_ASSERT(argc >= 0);
        _argc.setInt_32(argc);
    }

    inline ReturnedValue argument(int i) const {
        return i < argc() ? args[i].asReturnedValue() : Primitive::undefinedValue().asReturnedValue();
    }

    Value args[1];

    static Q_DECL_CONSTEXPR int HeaderSize() { return offsetof(CallData, args) / sizeof(QV4::Value); }
};

Q_STATIC_ASSERT(std::is_standard_layout<CallData>::value);
Q_STATIC_ASSERT(offsetof(CallData, thisObject) == CallData::This*sizeof(Value));
Q_STATIC_ASSERT(offsetof(CallData, args) == 6*sizeof(Value));

struct Q_QML_EXPORT CppStackFrame {
    CppStackFrame *parent;
    Function *v4Function;
    CallData *jsFrame;
    const Value *originalArguments;
    int originalArgumentsCount;
    int instructionPointer;
    const char *yield;
    const char *unwindHandler;
    const char *unwindLabel;
    int unwindLevel;

#ifndef V4_BOOTSTRAP
    uint requiredJSStackFrameSize() {
        return CallData::HeaderSize() + v4Function->compiledFunction->nRegisters;
    }
#endif

    QString source() const;
    QString function() const;
    inline QV4::ExecutionContext *context() const {
        return static_cast<ExecutionContext *>(&jsFrame->context);
    }
    int lineNumber() const;

    inline QV4::Heap::CallContext *callContext() const {
        Heap::ExecutionContext *ctx = static_cast<ExecutionContext &>(jsFrame->context).d();\
        while (ctx->type != Heap::ExecutionContext::Type_CallContext)
            ctx = ctx->outer;
        return static_cast<Heap::CallContext *>(ctx);
    }
    ReturnedValue thisObject() const;
};

}

QT_END_NAMESPACE

#endif
