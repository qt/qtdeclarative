// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4BYTECODEGENERATOR_P_H
#define QV4BYTECODEGENERATOR_P_H

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
#include <private/qv4instr_moth_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qqmljssourcelocation_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class SourceLocation;
}

namespace QV4 {

namespace Compiler {
struct Context;
}

namespace Moth {

class BytecodeGenerator {
public:
    BytecodeGenerator(int line, DebugInstructionPass debugInstructionPass,
                      bool storeSourceLocation = false)
        : debugInstructionPass(debugInstructionPass), startLine(line)
        , lastInstr(Instruction::Nop(), 0)
    {
        if (storeSourceLocation)
            m_sourceLocationTable.reset(new QV4::Compiler::Context::SourceLocationTable {});
    }

    struct Label {
        enum Special: int {
            Unlinked = -1,
            Dead = -2,
        };

        enum LinkMode {
            LinkNow,
            LinkLater
        };
        Label() = default;
        Label(BytecodeGenerator *generator, LinkMode mode = LinkNow)
            : generator(generator),
              index(generator->labels.size()) {
            generator->labels.append(Unlinked);
            generator->labelReachable.append(false);
            if (mode == LinkNow)
                link();
        }

        void link() const {
            Q_ASSERT(index >= 0);
            Q_ASSERT(generator->labels[index] == Unlinked);
            generator->locationSetAfterLabel = false;
            generator->clearLastInstruction();
            if (generator->labelReachable[index])
                generator->reachable = true;
            else
                generator->labelReachable[index] = generator->reachable;

            // A label that nothing can reach doesn't need a position. Code following it is only
            // emitted if another label makes it reachable, and that label gets its own position.
            generator->labels[index] = generator->labelReachable[index]
                    ? int(generator->instructions.size())
                    : Dead;
        }
        bool isValid() const { return generator != nullptr; }

        BytecodeGenerator *generator = nullptr;
        int index = -1;
    };

    struct Jump {
        // instruction is -1 if the jump was not emitted because it is unreachable.
        Jump(BytecodeGenerator *generator, int instruction)
            : generator(generator),
              index(instruction)
        { Q_ASSERT(generator); }

        ~Jump() {
            Q_ASSERT(index == -1 || generator->hasError || generator->instructions[index].linkedLabel != -1); // make sure link() got called
        }

        Jump(Jump &&j) {
            std::swap(generator, j.generator);
            std::swap(index, j.index);
        }

        BytecodeGenerator *generator = nullptr;
        int index = -1;

        void link() {
            // Link the jump first, so that the label knows it is reachable.
            Label l = generator->newLabel();
            link(l);
            l.link();
        }
        void link(Label l) {
            Q_ASSERT(l.index >= 0);
            if (index == -1)
                return;
            Q_ASSERT(generator->instructions[index].linkedLabel == -1);
            generator->instructions[index].linkedLabel = l.index;

            // We only drop code that cannot be reached when we see it. Therefore, the target of a
            // backward jump has to be reachable already. Code generation has to make sure of
            // this by entering loops by fall-through.
            Q_ASSERT(generator->labels[l.index] == Label::Unlinked
                     || generator->labelReachable[l.index]);
            generator->labelReachable[l.index] = true;
        }

    private:
        // make this type move-only:
        Q_DISABLE_COPY(Jump)
        // we never move-assign this type anywhere, so disable it:
        Jump &operator=(Jump &&) = delete;
    };

    struct ExceptionHandler : public Label {
        ExceptionHandler() = default;
        ExceptionHandler(BytecodeGenerator *generator)
            : Label(generator, LinkLater)
        {
        }
        ~ExceptionHandler()
        {
            Q_ASSERT(!generator || generator->currentExceptionHandler != this);
        }
        ExceptionHandler(const ExceptionHandler &) = default;
        ExceptionHandler(ExceptionHandler &&) = default;
        ExceptionHandler &operator=(const ExceptionHandler &) = default;
        ExceptionHandler &operator=(ExceptionHandler &&) = default;
        bool isValid() const { return generator != nullptr; }
    };

    Label label() {
        return Label(this, Label::LinkNow);
    }

    Label newLabel() {
        return Label(this, Label::LinkLater);
    }

    ExceptionHandler newExceptionHandler() {
        return ExceptionHandler(this);
    }

    template<typename Data>
    void addInstruction(const Data &data)
    {
        addInstructionHelper(Data::type, Moth::Instr::Factory<Data>::create(data));
    }

    Q_REQUIRED_RESULT Jump jump()
    {
        return addJumpInstruction(Instruction::Jump());
    }

    Q_REQUIRED_RESULT Jump jumpTrue()
    {
        return addJumpInstruction(Instruction::JumpTrue());
    }

    Q_REQUIRED_RESULT Jump jumpFalse()
    {
        return addJumpInstruction(Instruction::JumpFalse());
    }

    Q_REQUIRED_RESULT Jump jumpNotUndefined()
    {
        Instruction::JumpNotUndefined data{};
        return addJumpInstruction(data);
    }

    Q_REQUIRED_RESULT Jump jumpNoException()
    {
        Instruction::JumpNoException data{};
        return addJumpInstruction(data);
    }

    Q_REQUIRED_RESULT Jump jumpOptionalLookup(int index)
    {
        Instruction::GetOptionalLookup data{};
        data.index = index;
        return addJumpInstruction(data);
    }

    Q_REQUIRED_RESULT Jump jumpOptionalProperty(int name)
    {
        Instruction::LoadOptionalProperty data{};
        data.name = name;
        return addJumpInstruction(data);
    }

    void jumpStrictEqual(const StackSlot &lhs, const Label &target)
    {
        Instruction::CmpStrictEqual cmp;
        cmp.lhs = lhs;
        addInstruction(std::move(cmp));
        addJumpInstruction(Instruction::JumpTrue()).link(target);
    }

    void jumpStrictNotEqual(const StackSlot &lhs, const Label &target)
    {
        Instruction::CmpStrictNotEqual cmp;
        cmp.lhs = lhs;
        addInstruction(std::move(cmp));
        addJumpInstruction(Instruction::JumpTrue()).link(target);
    }

    void checkException()
    {
        Instruction::CheckException chk;
        addInstruction(chk);
    }

    void setUnwindHandler(ExceptionHandler *handler)
    {
        currentExceptionHandler = handler;
        Instruction::SetUnwindHandler data;
        data.offset = 0;
        if (!handler)
            addInstruction(data);
        else
            addJumpInstruction(data).link(*handler);
    }

    void unwindToLabel(int level, const Label &target)
    {
        if (level) {
            Instruction::UnwindToLabel unwind;
            unwind.level = level;
            addJumpInstruction(unwind).link(target);
        } else {
            jump().link(target);
        }
    }



    void setLocation(const QQmlJS::SourceLocation &loc);
    void incrementStatement();

    ExceptionHandler *exceptionHandler() const {
        return currentExceptionHandler;
    }

    int newRegister();
    int newRegisterArray(int n);
    int registerCount() const { return regCount; }
    int currentRegister() const { return currentReg; }

    void finalize(Compiler::Context *context);

    template<typename Data>
    Jump addJumpInstruction(const Data &data)
    {
        return Jump(this, addInstructionHelper(Data::type, Moth::Instr::Factory<Data>::create(data),
                                               offsetof(Data, offset)));
    }

    void addCJumpInstruction(bool jumpOnFalse, const Label *trueLabel, const Label *falseLabel)
    {
        if (jumpOnFalse)
            addJumpInstruction(Instruction::JumpFalse()).link(*falseLabel);
        else
            addJumpInstruction(Instruction::JumpTrue()).link(*trueLabel);
    }

    void clearLastInstruction()
    {
        lastInstrType = -1;
    }

    void addLoopStart(const Label &start)
    {
        _labelInfos.push_back({ start.index });
    }

    void setError(bool b) { hasError = b; }

private:
    friend struct Jump;
    friend struct Label;
    friend struct ExceptionHandler;
    friend class DebugInstructions;

    int addInstructionHelper(Moth::Instr::Type type, const Instr &i, int offsetOfOffset = -1);

    struct I {
        Moth::Instr::Type type;
        short size;
        uint position;
        int line;
        int statement;
        int offsetForJump;
        int linkedLabel;
        // Whether code generation set a location between the last label and this instruction.
        bool locationSetAfterLabel;
        unsigned char packed[sizeof(Instr) + 2]; // 2 for instruction type
    };

    static bool endsFlow(Moth::Instr::Type type)
    {
        switch (type) {
        case Instr::Type::Ret:
        case Instr::Type::Jump:
        case Instr::Type::UnwindToLabel:
        case Instr::Type::ThrowException:
            return true;
        default:
            return false;
        }
    }

    static I makeInstruction(Moth::Instr::Type type, const Instr &i, int offsetOfOffset, int line,
                             int statement)
    {
        const int argCount = Moth::InstrInfo::argumentCount[static_cast<int>(type)];
        int s = argCount * sizeof(int);
        if (offsetOfOffset != -1)
            offsetOfOffset += Instr::encodedLength(type);
        I instr{ type,      static_cast<short>(s + Instr::encodedLength(type)),
                 0,         line,
                 statement, offsetOfOffset,
                 -1,        false,
                 "\0\0" };
        uchar *code = instr.packed;
        code = Instr::pack(code, Instr::wideInstructionType(type));

        for (int j = 0; j < argCount; ++j) {
            qToLittleEndian<qint32>(i[j], code);
            code += sizeof(int);
        }
        return instr;
    }

    void compressInstructions();
    void packInstruction(I &i);
    void adjustJumpOffsets();
    bool isLabelValid(int position) const { return labels.at(position) >= 0; }

    QList<I> instructions;
    QList<int> labels;

    // Whether a label can be reached. Before it is linked, whether a reachable jump targets it.
    QList<bool> labelReachable;
    ExceptionHandler *currentExceptionHandler = nullptr;
    DebugInstructionPass debugInstructionPass = nullptr;
    int regCount = 0;
public:
    int currentReg = 0;
private:
    int startLine = 0;
    int currentLine = 0;
    QQmlJS::SourceLocation currentSourceLocation;
    bool locationSetAfterLabel = true;
    std::unique_ptr<QV4::Compiler::Context::SourceLocationTable> m_sourceLocationTable;
    int currentStatement = 0;
    bool hasError = false;

    // Whether control flow can reach the next instruction. Unreachable instructions are dropped.
    bool reachable = true;

    int lastInstrType = -1;
    Moth::Instr lastInstr;

    struct LabelInfo {
        int labelIndex;
    };
    std::vector<LabelInfo> _labelInfos;
};

}
}

QT_END_NAMESPACE

#endif
