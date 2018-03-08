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
#ifndef QV4COMPILERCONTROLFLOW_P_H
#define QV4COMPILERCONTROLFLOW_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4codegen_p.h>
#include <private/qqmljsast_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Compiler {

struct ControlFlow {
    using Reference = Codegen::Reference;
    using BytecodeGenerator = Moth::BytecodeGenerator;
    using Instruction = Moth::Instruction;

    enum Type {
        Loop,
        With,
        Finally,
        Catch
    };

    enum HandlerType {
        Invalid,
        Break,
        Continue,
        Return,
        Throw
    };

    struct Handler {
        HandlerType type;
        QString label;
        BytecodeGenerator::Label linkLabel;
        int tempIndex;
        int value;
    };

    Codegen *cg;
    ControlFlow *parent;
    Type type;
    bool needsLookupByName = false;

    ControlFlow(Codegen *cg, Type type)
        : cg(cg), parent(cg->_context->controlFlow), type(type)
    {
        cg->_context->controlFlow = this;
    }

    virtual ~ControlFlow() {
        cg->_context->controlFlow = parent;
    }

    void emitReturnStatement() const {
        if (cg->_returnAddress >= 0) {
            Instruction::LoadReg load;
            load.reg = Moth::StackSlot::createRegister(cg->_returnAddress);
            generator()->addInstruction(load);
        }
        Instruction::Ret ret;
        cg->bytecodeGenerator->addInstruction(ret);
    }

    void jumpToHandler(const Handler &h) {
        if (h.linkLabel.isReturn()) {
            emitReturnStatement();
        } else {
            if (h.tempIndex >= 0)
                Reference::storeConstOnStack(cg, QV4::Encode(h.value), h.tempIndex);
            cg->bytecodeGenerator->jump().link(h.linkLabel);
        }
    }

    bool returnRequiresUnwind() const {
        const ControlFlow *f = this;
        while (f) {
            if (f->type == Finally)
                return true;
            f = f->parent;
        }
        return false;
    }

    virtual QString label() const { return QString(); }

    bool isSimple() const {
        return type == Loop;
    }

    Handler getParentHandler(HandlerType type, const QString &label = QString()) {
        if (parent)
            return parent->getHandler(type, label);
        switch (type) {
        case Break:
        case Continue:
            return { Invalid, QString(), {}, -1, 0 };
        case Return:
        case Throw:
            return { type, QString(), BytecodeGenerator::Label::returnLabel(), -1, 0 };
        case Invalid:
            break;
        }
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) = 0;

    BytecodeGenerator::ExceptionHandler *parentExceptionHandler() {
        return parent ? parent->exceptionHandler() : nullptr;
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return parentExceptionHandler();
    }


    virtual void handleThrow(const Reference &expr) {
        Reference e = expr;
        Handler h = getHandler(ControlFlow::Throw);
        if (h.tempIndex >= 0) {
            e = e.storeOnStack();
            Reference::storeConstOnStack(cg, QV4::Encode(h.value), h.tempIndex);
        }
        e.loadInAccumulator();
        Instruction::ThrowException instr;
        generator()->addInstruction(instr);
    }

protected:
    QString loopLabel() const {
        QString label;
        if (cg->_labelledStatement) {
            label = cg->_labelledStatement->label.toString();
            cg->_labelledStatement = nullptr;
        }
        return label;
    }
    BytecodeGenerator *generator() const {
        return cg->bytecodeGenerator;
    }
};

struct ControlFlowLoop : public ControlFlow
{
    QString loopLabel;
    BytecodeGenerator::Label *breakLabel = nullptr;
    BytecodeGenerator::Label *continueLabel = nullptr;

    ControlFlowLoop(Codegen *cg, BytecodeGenerator::Label *breakLabel, BytecodeGenerator::Label *continueLabel = nullptr)
        : ControlFlow(cg, Loop), loopLabel(ControlFlow::loopLabel()), breakLabel(breakLabel), continueLabel(continueLabel)
    {
    }

    virtual QString label() const { return loopLabel; }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        switch (type) {
        case Break:
            if (breakLabel && (label.isEmpty() || label == loopLabel))
                return { type, loopLabel, *breakLabel, -1, 0 };
            break;
        case Continue:
            if (continueLabel && (label.isEmpty() || label == loopLabel))
                return { type, loopLabel, *continueLabel, -1, 0 };
            break;
        case Return:
        case Throw:
            break;
        case Invalid:
            Q_ASSERT(false);
            Q_UNREACHABLE();
        }
        return getParentHandler(type, label);
    }

};

struct ControlFlowUnwind : public ControlFlow
{
    BytecodeGenerator::ExceptionHandler unwindLabel;
    int controlFlowTemp;
    QVector<Handler> handlers;

    ControlFlowUnwind(Codegen *cg, Type type)
        : ControlFlow(cg, type), unwindLabel(generator()->newExceptionHandler())
    {
        Q_ASSERT(type != Loop);
        controlFlowTemp = static_cast<int>(generator()->newRegister());
        Reference::storeConstOnStack(cg, QV4::Encode::undefined(), controlFlowTemp);
        // we'll need at least a handler for throw
        getHandler(Throw);
    }

    void emitUnwindHandler()
    {
        Q_ASSERT(!isSimple());

        Reference temp = Reference::fromStackSlot(cg, controlFlowTemp);
        for (const auto &h : qAsConst(handlers)) {
            Handler parentHandler = getParentHandler(h.type, h.label);

            if (h.type == Throw || parentHandler.tempIndex >= 0) {
                BytecodeGenerator::Label skip = generator()->newLabel();
                generator()->jumpStrictNotEqualStackSlotInt(temp.stackSlot(), h.value).link(skip);
                if (h.type == Throw)
                    emitForThrowHandling();
                jumpToHandler(parentHandler);
                skip.link();
            } else {
                if (parentHandler.linkLabel.isReturn()) {
                    BytecodeGenerator::Label skip = generator()->newLabel();
                    generator()->jumpStrictNotEqualStackSlotInt(temp.stackSlot(), h.value).link(skip);
                    emitReturnStatement();
                    skip.link();
                } else {
                   generator()->jumpStrictEqualStackSlotInt(temp.stackSlot(), h.value).link(parentHandler.linkLabel);
                }
            }
        }
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        for (const auto &h : qAsConst(handlers)) {
            if (h.type == type && h.label == label)
                return h;
        }
        Handler h = {
            type,
            label,
            unwindLabel,
            controlFlowTemp,
            handlers.size()
        };
        handlers.append(h);
        return h;
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return &unwindLabel;
    }

    virtual void emitForThrowHandling() { }
};

struct ControlFlowWith : public ControlFlowUnwind
{
    ControlFlowWith(Codegen *cg)
        : ControlFlowUnwind(cg, With)
    {
        needsLookupByName = true;

        savedContextRegister = Moth::StackSlot::createRegister(generator()->newRegister());

        // assumes the with object is in the accumulator
        Instruction::PushWithContext pushScope;
        pushScope.reg = savedContextRegister;
        generator()->addInstruction(pushScope);
        generator()->setExceptionHandler(&unwindLabel);
    }

    virtual ~ControlFlowWith() {
        // emit code for unwinding
        unwindLabel.link();

        generator()->setExceptionHandler(parentExceptionHandler());
        Instruction::PopContext pop;
        pop.reg = savedContextRegister;
        generator()->addInstruction(pop);

        emitUnwindHandler();
    }
    Moth::StackSlot savedContextRegister;
};

struct ControlFlowCatch : public ControlFlowUnwind
{
    AST::Catch *catchExpression;
    bool insideCatch = false;
    BytecodeGenerator::ExceptionHandler exceptionLabel;
    BytecodeGenerator::ExceptionHandler catchUnwindLabel;

    ControlFlowCatch(Codegen *cg, AST::Catch *catchExpression)
        : ControlFlowUnwind(cg, Catch), catchExpression(catchExpression),
          exceptionLabel(generator()->newExceptionHandler()),
          catchUnwindLabel(generator()->newExceptionHandler())
    {
        generator()->setExceptionHandler(&exceptionLabel);
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        Handler h = getParentHandler(type, label);
        if (h.type == Invalid)
            return h;
        h = ControlFlowUnwind::getHandler(type, label);
        if (insideCatch)
            // if we're inside the catch block, we need to jump to the pop scope
            // instruction at the end of the catch block, not the unwind handler
            h.linkLabel = catchUnwindLabel;
        else if (type == Throw)
            // if we're inside the try block, we need to jump to the catch block,
            // not the unwind handler
            h.linkLabel = exceptionLabel;
        return h;
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return insideCatch ? &catchUnwindLabel : &exceptionLabel;
    }

    ~ControlFlowCatch() {
        // emit code for unwinding

        needsLookupByName = true;
        insideCatch = true;

        Codegen::RegisterScope scope(cg);

        // exceptions inside the try block go here
        exceptionLabel.link();
        Moth::StackSlot savedContextReg = Moth::StackSlot::createRegister(generator()->newRegister());
        Instruction::PushCatchContext pushCatch;
        pushCatch.name = cg->registerString(catchExpression->name.toString());
        pushCatch.reg = savedContextReg;
        generator()->addInstruction(pushCatch);
        // clear the unwind temp for exceptions, we want to resume normal code flow afterwards
        Reference::storeConstOnStack(cg, QV4::Encode::undefined(), controlFlowTemp);
        generator()->setExceptionHandler(&catchUnwindLabel);

        cg->statement(catchExpression->statement);

        insideCatch = false;
        needsLookupByName = false;

        // exceptions inside catch and break/return statements go here
        catchUnwindLabel.link();
        Instruction::PopContext pop;
        pop.reg = savedContextReg;
        generator()->addInstruction(pop);

        // break/continue/return statements in try go here
        unwindLabel.link();
        generator()->setExceptionHandler(parentExceptionHandler());

        emitUnwindHandler();
    }
};

struct ControlFlowFinally : public ControlFlowUnwind
{
    AST::Finally *finally;
    bool insideFinally = false;
    int exceptionTemp = -1;

    ControlFlowFinally(Codegen *cg, AST::Finally *finally)
        : ControlFlowUnwind(cg, Finally), finally(finally)
    {
        Q_ASSERT(finally != nullptr);
        generator()->setExceptionHandler(&unwindLabel);
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        // if we're inside the finally block, any exceptions etc. should
        // go directly to the parent handler
        if (insideFinally)
            return getParentHandler(type, label);
        return ControlFlowUnwind::getHandler(type, label);
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return insideFinally ? parentExceptionHandler() : ControlFlowUnwind::exceptionHandler();
    }

    ~ControlFlowFinally() {
        // emit code for unwinding
        unwindLabel.link();

        Codegen::RegisterScope scope(cg);

        Moth::StackSlot retVal = Moth::StackSlot::createRegister(generator()->newRegister());
        Instruction::StoreReg storeRetVal;
        storeRetVal.reg = retVal;
        generator()->addInstruction(storeRetVal);

        insideFinally = true;
        exceptionTemp = generator()->newRegister();
        Instruction::GetException instr;
        generator()->addInstruction(instr);
        Reference::fromStackSlot(cg, exceptionTemp).storeConsumeAccumulator();

        generator()->setExceptionHandler(parentExceptionHandler());
        cg->statement(finally->statement);
        insideFinally = false;

        Instruction::LoadReg loadRetVal;
        loadRetVal.reg = retVal;
        generator()->addInstruction(loadRetVal);

        emitUnwindHandler();
    }

    virtual void emitForThrowHandling() {
        // reset the exception flag, that got cleared before executing the statements in finally
        Reference::fromStackSlot(cg, exceptionTemp).loadInAccumulator();
        Instruction::SetException setException;
        Q_ASSERT(exceptionTemp != -1);
        generator()->addInstruction(setException);
    }
};

} } // QV4::Compiler namespace

QT_END_NAMESPACE

#endif
