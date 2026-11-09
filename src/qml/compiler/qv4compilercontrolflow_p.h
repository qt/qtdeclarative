// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

#include <private/qv4codegen_p.h>
#include <private/qqmljsast_p.h>
#include <private/qv4bytecodegenerator_p.h>

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
        Block,
        Finally,
        Catch
    };

    enum UnwindType {
        Break,
        Continue,
        Return
    };

    struct UnwindTarget {
        BytecodeGenerator::Label linkLabel;
        int unwindLevel;

        // If the unwind has to pass a finally block, the finally block continues
        // the unwind afterwards, depending on the value of completionRegister.
        int completionRegister = -1;
        int completion = 0;
    };

    Codegen *cg;
    ControlFlow *parent;
    Type type;

    ControlFlow(Codegen *cg, Type type)
        : cg(cg), parent(cg->controlFlow), type(type)
    {
        cg->controlFlow = this;
    }

    virtual ~ControlFlow() {
        cg->controlFlow = parent;
    }

    UnwindTarget unwindTarget(UnwindType type, const QString &label = QString())
    {
        Q_ASSERT(type == Break || type == Continue || type == Return);
        ControlFlow *flow = this;
        int level = 0;
        while (flow) {
            BytecodeGenerator::Label l = flow->getUnwindTarget(type, label);
            if (l.isValid())
                return UnwindTarget{l, level};
            UnwindTarget intercepted;
            if (flow->interceptUnwind(type, label, level, &intercepted))
                return intercepted;
            if (flow->requiresUnwind())
                ++level;
            flow = flow->parent;
        }
        if (type == Return)
            return UnwindTarget{ cg->returnLabel(), level };
        return UnwindTarget();
    }

    // Jumps to the target, passing the handlers in between.
    // Remembers pending completion in completionRegister.
    void unwind(const UnwindTarget &target)
    {
        if (target.completionRegister >= 0) {
            Reference::fromConst(cg, QV4::Encode(target.completion))
                    .storeOnStack(target.completionRegister);
        }
        generator()->unwindToLabel(target.unwindLevel, target.linkLabel);
    }

    virtual QString label() const { return QString(); }

    bool hasLoop() const {
        const ControlFlow *flow = this;
        while (flow) {
            if (flow->type == Loop)
                return true;
            flow = flow->parent;
        }
        return false;
    }

protected:
    virtual BytecodeGenerator::Label getUnwindTarget(UnwindType, const QString & = QString()) {
        return BytecodeGenerator::Label();
    }
    virtual bool requiresUnwind() {
        return false;
    }
    virtual bool interceptUnwind(UnwindType, const QString &, int, UnwindTarget *) { return false; }

public:
    BytecodeGenerator::ExceptionHandler *parentUnwindHandler() {
        return parent ? parent->unwindHandler() : nullptr;
    }

    virtual BytecodeGenerator::ExceptionHandler *unwindHandler() {
        return parentUnwindHandler();
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

struct ControlFlowUnwind : public ControlFlow
{
    BytecodeGenerator::ExceptionHandler unwindLabel;

    ControlFlowUnwind(Codegen *cg, Type type)
        : ControlFlow(cg, type)
    {
    }

    void setupUnwindHandler()
    {
        unwindLabel = generator()->newExceptionHandler();
    }

    void emitUnwindHandler()
    {
        Q_ASSERT(requiresUnwind());

        Instruction::UnwindDispatch dispatch;
        generator()->addInstruction(dispatch);
    }

    virtual BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return unwindLabel.isValid() ? &unwindLabel : parentUnwindHandler();
    }
};

struct ControlFlowUnwindCleanup : public ControlFlowUnwind
{
    std::function<void()> cleanup = nullptr;

    ControlFlowUnwindCleanup(Codegen *cg, std::function<void()> cleanup, Type type = Block)
        : ControlFlowUnwind(cg, type), cleanup(cleanup)
    {
        if (cleanup) {
            setupUnwindHandler();
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    ~ControlFlowUnwindCleanup() {
        if (cleanup) {
            unwindLabel.link();
            cleanup();
            generator()->setUnwindHandler(parentUnwindHandler());
            emitUnwindHandler();
        }
    }

    bool requiresUnwind() override {
        return cleanup != nullptr;
    }
};

struct ControlFlowLoop : public ControlFlowUnwindCleanup
{
    QString loopLabel;
    BytecodeGenerator::Label *breakLabel = nullptr;
    BytecodeGenerator::Label *continueLabel = nullptr;

    ControlFlowLoop(Codegen *cg, BytecodeGenerator::Label *breakLabel, BytecodeGenerator::Label *continueLabel = nullptr, std::function<void()> cleanup = nullptr)
        : ControlFlowUnwindCleanup(cg, cleanup, Loop), loopLabel(ControlFlow::loopLabel()), breakLabel(breakLabel), continueLabel(continueLabel)
    {
    }

    BytecodeGenerator::Label getUnwindTarget(UnwindType type, const QString &label) override {
        switch (type) {
        case Break:
            if (breakLabel && (label.isEmpty() || label == loopLabel))
                return *breakLabel;
            break;
        case Continue:
            if (continueLabel && (label.isEmpty() || label == loopLabel))
                return *continueLabel;
            break;
        default:
            break;
        }
        return BytecodeGenerator::Label();
    }

    QString label() const override { return loopLabel; }
};


struct ControlFlowWith : public ControlFlowUnwind
{
    ControlFlowWith(Codegen *cg)
        : ControlFlowUnwind(cg, With)
    {
        setupUnwindHandler();

        // assumes the with object is in the accumulator
        Instruction::PushWithContext pushScope;
        generator()->addInstruction(pushScope);
        generator()->setUnwindHandler(&unwindLabel);
    }

    ~ControlFlowWith() {
        // emit code for unwinding
        unwindLabel.link();

        generator()->setUnwindHandler(parentUnwindHandler());
        Instruction::PopContext pop;
        generator()->addInstruction(pop);

        emitUnwindHandler();
    }

    bool requiresUnwind() override {
        return true;
    }


};

struct ControlFlowBlock : public ControlFlowUnwind
{
    ControlFlowBlock(Codegen *cg, QQmlJS::AST::Node *ast)
        : ControlFlowUnwind(cg, Block)
    {
        block = cg->enterBlock(ast);
        block->emitBlockHeader(cg);

        if (block->requiresExecutionContext) {
            setupUnwindHandler();
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    virtual ~ControlFlowBlock() {
        // emit code for unwinding
        if (block->requiresExecutionContext) {
            unwindLabel.link();
            generator()->setUnwindHandler(parentUnwindHandler());
        }

        block->emitBlockFooter(cg);

        if (block->requiresExecutionContext )
            emitUnwindHandler();
        cg->leaveBlock();
    }

    virtual bool requiresUnwind() override {
        return block->requiresExecutionContext;
    }

    Context *block;
};

struct ControlFlowCatch : public ControlFlowUnwind
{
    QQmlJS::AST::Catch *catchExpression;
    bool insideCatch = false;
    BytecodeGenerator::ExceptionHandler exceptionLabel;

    ControlFlowCatch(Codegen *cg, QQmlJS::AST::Catch *catchExpression)
        : ControlFlowUnwind(cg, Catch), catchExpression(catchExpression),
          exceptionLabel(generator()->newExceptionHandler())
    {
        generator()->setUnwindHandler(&exceptionLabel);
    }

    virtual bool requiresUnwind() override {
        return true;
    }

    BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return insideCatch ? &unwindLabel : &exceptionLabel;
    }

    ~ControlFlowCatch() {
        // emit code for unwinding
        insideCatch = true;
        setupUnwindHandler();

        Codegen::RegisterScope scope(cg);

        // exceptions inside the try block go here
        exceptionLabel.link();
        BytecodeGenerator::Jump noException = generator()->jumpNoException();

        Context *block = cg->enterBlock(catchExpression);

        block->emitBlockHeader(cg);

        generator()->setUnwindHandler(&unwindLabel);

        if (catchExpression->patternElement->bindingIdentifier.isEmpty()) {
            // destructuring pattern
            cg->initializeAndDestructureBindingElement(
                    catchExpression->patternElement,
                    Reference::fromName(cg, QStringLiteral("@caught")), true);
        }
        // skip the additional block
        cg->statementList(catchExpression->statement->statements);

        // exceptions inside catch and break/return statements go here
        unwindLabel.link();
        block->emitBlockFooter(cg);

        cg->leaveBlock();

        noException.link();
        generator()->setUnwindHandler(parentUnwindHandler());

        emitUnwindHandler();
        insideCatch = false;
    }
};

struct ControlFlowFinally : public ControlFlowUnwind
{
    // A break, continue or return that passes the finally block does not leave anything in the
    // stack frame while the finally block runs. Instead, it stores a number identifying itself in
    // completionRegister and unwinds to the start of the finally block. After the finally block,
    // we find the original unwind by that number and continue it.
    struct Completion
    {
        QString label;
        UnwindType type;
    };

    QQmlJS::AST::Finally *finally;
    bool insideFinally = false;
    int completionRegister;
    QVarLengthArray<Completion, 4> completions;

    ControlFlowFinally(Codegen *cg, QQmlJS::AST::Finally *finally, bool hasCatchBlock)
        : ControlFlowUnwind(cg, Finally), finally(finally)
    {
        Q_ASSERT(finally != nullptr);
        setupUnwindHandler();

        // The register has to live as long as the finally block, not just the try block.
        completionRegister = generator()->newRegister();

        // No need to set the handler for the finally now if there is a catch block.
        // In that case, a handler for the latter will be set immediately after this.
        if (!hasCatchBlock) {
            generator()->setUnwindHandler(&unwindLabel);
        }
    }

    virtual bool requiresUnwind() override {
        return !insideFinally;
    }

    bool interceptUnwind(UnwindType type, const QString &label, int level,
                         UnwindTarget *target) override
    {
        if (insideFinally)
            return false;

        const auto it =
                std::find_if(completions.cbegin(), completions.cend(), [&](const Completion &c) {
                    return c.type == type && c.label == label;
                });
        const qsizetype index =
                it == completions.cend() ? completions.size() : it - completions.cbegin();
        if (it == completions.cend())
            completions.append({ label, type });

        target->linkLabel = unwindLabel;
        target->unwindLevel = level;
        target->completionRegister = completionRegister;
        target->completion = int(index) + 1;
        return true;
    }

    BytecodeGenerator::ExceptionHandler *unwindHandler() override {
        return insideFinally ? parentUnwindHandler() : ControlFlowUnwind::unwindHandler();
    }

    ~ControlFlowFinally() {
        if (!completions.isEmpty())
            Reference::fromConst(cg, QV4::Encode(0)).storeOnStack(completionRegister);

        // emit code for unwinding
        unwindLabel.link();

        Codegen::RegisterScope scope(cg);

        insideFinally = true;
        int returnValueTemp = -1;
        if (cg->requiresReturnValue) {
            returnValueTemp = generator()->newRegister();
            Instruction::MoveReg move;
            move.srcReg = cg->_returnAddress;
            move.destReg = returnValueTemp;
            generator()->addInstruction(move);
        }
        int exceptionTemp = generator()->newRegister();
        Instruction::GetException instr;
        generator()->addInstruction(instr);
        Reference::fromStackSlot(cg, exceptionTemp).storeConsumeAccumulator();

        generator()->setUnwindHandler(parentUnwindHandler());
        cg->statement(finally->statement);
        insideFinally = false;

        if (cg->requiresReturnValue) {
            Instruction::MoveReg move;
            move.srcReg = returnValueTemp;
            move.destReg = cg->_returnAddress;
            generator()->addInstruction(move);
        }
        Reference::fromStackSlot(cg, exceptionTemp).loadInAccumulator();
        Instruction::SetException setException;
        generator()->addInstruction(setException);

        emitUnwindHandler();

        // A pending exception is thrown by the UnwindDispatch above. Otherwise, continue with
        // the break, continue or return that brought us here, if any.
        insideFinally = true;
        for (qsizetype i = 0, end = completions.size(); i != end; ++i) {
            const Completion &completion = completions.at(i);
            BytecodeGenerator::Label next = generator()->newLabel();
            Reference::fromConst(cg, QV4::Encode(int(i) + 1)).loadInAccumulator();
            generator()->jumpStrictNotEqual(Moth::StackSlot::createRegister(completionRegister),
                                            next);
            const UnwindTarget target = unwindTarget(completion.type, completion.label);
            Q_ASSERT(target.linkLabel.isValid());
            unwind(target);
            next.link();
        }
        insideFinally = false;
    }
};

} } // QV4::Compiler namespace

QT_END_NAMESPACE

#endif
