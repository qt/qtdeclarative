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

#include <private/qv4context_p.h>
#include <private/qv4enginebase_p.h>
#include <private/qv4calldata_p.h>
#include <private/qv4function_p.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct CppStackFrame;
struct Q_QML_PRIVATE_EXPORT CppStackFrameBase
{
    enum class Kind : quint8 { Bare, JS, Meta };

    Value *savedStackTop;
    CppStackFrame *parent;
    Function *v4Function;
    CallData *jsFrame;
    int originalArgumentsCount;
    int instructionPointer;

    union {
        struct {
            const Value *originalArguments;
            const char *yield;
            const char *unwindHandler;
            const char *unwindLabel;
            int unwindLevel;
            bool yieldIsIterator;
            bool callerCanHandleTailCall;
            bool pendingTailCall;
            bool isTailCalling;
        };
        struct {
            const QMetaType *metaTypes;
            void **returnAndArgs;
            bool returnValueIsUndefined;
        };
    };

    Kind kind;
};

struct Q_QML_PRIVATE_EXPORT CppStackFrame : protected CppStackFrameBase
{
    // We want to have those public but we can't declare them as public without making the struct
    // non-standard layout. So we have this other struct with "using" in between.
    using CppStackFrameBase::instructionPointer;
    using CppStackFrameBase::v4Function;
    using CppStackFrameBase::jsFrame;

    void init(Function *v4Function, int argc, Kind kind = Kind::Bare) {
        this->v4Function = v4Function;
        originalArgumentsCount = argc;
        instructionPointer = 0;
        this->kind = kind;
    }

    bool isBareStackFrame() const { return kind == Kind::Bare; }
    bool isJSTypesFrame() const { return kind == Kind::JS; }
    bool isMetaTypesFrame() const { return kind == Kind::Meta; }

    static uint requiredJSStackFrameSize(uint nRegisters) {
        return CallData::HeaderSize() + nRegisters;
    }
    static uint requiredJSStackFrameSize(Function *v4Function) {
        return CallData::HeaderSize() + v4Function->compiledFunction->nRegisters;
    }
    uint requiredJSStackFrameSize() const {
        return requiredJSStackFrameSize(v4Function);
    }

    void setupJSFrame(Value *stackSpace, const Value &function, const Heap::ExecutionContext *scope,
                      const Value &thisObject, const Value &newTarget = Value::undefinedValue())
    {
        jsFrame = reinterpret_cast<CallData *>(stackSpace);
        jsFrame->function = function;
        jsFrame->context = scope->asReturnedValue();
        jsFrame->accumulator = Encode::undefined();
        jsFrame->thisObject = thisObject;
        jsFrame->newTarget = newTarget;
    }

    QString source() const;
    QString function() const;
    int lineNumber() const;
    ReturnedValue thisObject() const;

    ExecutionContext *context() const
    {
        return static_cast<ExecutionContext *>(&jsFrame->context);
    }

    void setContext(ExecutionContext *context)
    {
        jsFrame->context = context;
    }

    Heap::CallContext *callContext() const
    {
        Heap::ExecutionContext *ctx = static_cast<ExecutionContext &>(jsFrame->context).d();\
        while (ctx->type != Heap::ExecutionContext::Type_CallContext)
            ctx = ctx->outer;
        return static_cast<Heap::CallContext *>(ctx);
    }

    CppStackFrame *parentFrame() const { return parent; }
    void setParentFrame(CppStackFrame *parentFrame) { parent = parentFrame; }

    int argc() const { return originalArgumentsCount; }
    Value *framePointer() const { return savedStackTop; }

    void push(EngineBase *engine) {
        parent = engine->currentStackFrame;
        engine->currentStackFrame = this;
        savedStackTop = engine->jsStackTop;
    }

    void pop(EngineBase *engine) {
        engine->currentStackFrame = parent;
        engine->jsStackTop = savedStackTop;
    }
};

struct Q_QML_PRIVATE_EXPORT MetaTypesStackFrame : public CppStackFrame
{
    void init(Function *v4Function, void **a, const QMetaType *types, int argc)
    {
        CppStackFrame::init(v4Function, argc, Kind::Meta);
        metaTypes = types;
        returnAndArgs = a;
        returnValueIsUndefined = false;
    }

    QMetaType returnType() const { return metaTypes[0]; }
    void *returnValue() const { return returnAndArgs[0]; }

    bool isReturnValueUndefined() const { return CppStackFrameBase::returnValueIsUndefined; }
    void setReturnValueUndefined() { CppStackFrameBase::returnValueIsUndefined = true; }

    const QMetaType *argTypes() const { return metaTypes + 1; }
    void **argv() const { return returnAndArgs + 1; }
};

struct Q_QML_PRIVATE_EXPORT JSTypesStackFrame : public CppStackFrame
{
    // The JIT needs to poke directly into those using offsetof
    using CppStackFrame::unwindHandler;
    using CppStackFrame::unwindLabel;
    using CppStackFrame::unwindLevel;

    void init(Function *v4Function, const Value *argv, int argc,
              bool callerCanHandleTailCall = false)
    {
        CppStackFrame::init(v4Function, argc, Kind::JS);
        CppStackFrame::originalArguments = argv;
        CppStackFrame::yield = nullptr;
        CppStackFrame::unwindHandler = nullptr;
        CppStackFrame::yieldIsIterator = false;
        CppStackFrame::callerCanHandleTailCall = callerCanHandleTailCall;
        CppStackFrame::pendingTailCall = false;
        CppStackFrame::isTailCalling = false;
        CppStackFrame::unwindLabel = nullptr;
        CppStackFrame::unwindLevel = 0;
    }

    const Value *argv() const { return originalArguments; }

    void setupJSFrame(Value *stackSpace, const Value &function, const Heap::ExecutionContext *scope,
                      const Value &thisObject, const Value &newTarget = Value::undefinedValue()) {
        setupJSFrame(stackSpace, function, scope, thisObject, newTarget,
                     v4Function->compiledFunction->nFormals,
                     v4Function->compiledFunction->nRegisters);
    }

    void setupJSFrame(
            Value *stackSpace, const Value &function, const Heap::ExecutionContext *scope,
            const Value &thisObject, const Value &newTarget, uint nFormals, uint nRegisters)
    {
        CppStackFrame::setupJSFrame(stackSpace, function, scope, thisObject, newTarget);

        uint argc = uint(originalArgumentsCount);
        if (argc > nFormals)
            argc = nFormals;
        jsFrame->setArgc(argc);

        // memcpy requires non-null ptr, even if  argc * sizeof(Value) == 0
        if (originalArguments)
            memcpy(jsFrame->args, originalArguments, argc * sizeof(Value));
        Q_STATIC_ASSERT(Encode::undefined() == 0);
        memset(jsFrame->args + argc, 0, (nRegisters - argc) * sizeof(Value));

        if (v4Function && v4Function->compiledFunction) {
            const int firstDeadZoneRegister
                    = v4Function->compiledFunction->firstTemporalDeadZoneRegister;
            const int registerDeadZoneSize
                    = v4Function->compiledFunction->sizeOfRegisterTemporalDeadZone;

            const Value * tdzEnd = stackSpace + firstDeadZoneRegister + registerDeadZoneSize;
            for (Value *v = stackSpace + firstDeadZoneRegister; v < tdzEnd; ++v)
                *v = Value::emptyValue().asReturnedValue();
        }
    }

    bool isTailCalling() const { return CppStackFrame::isTailCalling; }
    void setTailCalling(bool tailCalling) { CppStackFrame::isTailCalling = tailCalling; }

    bool pendingTailCall() const { return CppStackFrame::pendingTailCall; }
    void setPendingTailCall(bool pending) { CppStackFrame::pendingTailCall = pending; }

    const char *yield() const { return CppStackFrame::yield; }
    void setYield(const char *yield) { CppStackFrame::yield = yield; }

    bool yieldIsIterator() const { return CppStackFrame::yieldIsIterator; }
    void setYieldIsIterator(bool isIter) { CppStackFrame::yieldIsIterator = isIter; }

    bool callerCanHandleTailCall() const { return CppStackFrame::callerCanHandleTailCall; }
};

Q_STATIC_ASSERT(sizeof(CppStackFrame) == sizeof(JSTypesStackFrame));
Q_STATIC_ASSERT(sizeof(CppStackFrame) == sizeof(MetaTypesStackFrame));
Q_STATIC_ASSERT(std::is_standard_layout_v<CppStackFrame>);
Q_STATIC_ASSERT(std::is_standard_layout_v<JSTypesStackFrame>);
Q_STATIC_ASSERT(std::is_standard_layout_v<MetaTypesStackFrame>);

}

QT_END_NAMESPACE

#endif
