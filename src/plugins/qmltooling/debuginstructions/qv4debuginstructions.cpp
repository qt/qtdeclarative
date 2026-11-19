// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4debuginstructions_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Moth {

enum : int { NoJumpLine = -1, MultipleJumpLines = -2 };

static int mergeJumpLines(int a, int b)
{
    if (a == NoJumpLine)
        return b;
    if (b == NoJumpLine || a == b)
        return a;
    return MultipleJumpLines;
}

static bool isFromOtherLine(int jumpLine, int line)
{
    return jumpLine == MultipleJumpLines || (jumpLine != NoJumpLine && jumpLine != line);
}

void DebugInstructions::insert(BytecodeGenerator *generator)
{
    DebugInstructions(generator).run();
}

void DebugInstructions::appendDebug(int line, const I &before)
{
    m_generator->instructions.append(makeInstruction(Instruction::Debug(), line, before.statement));
}

// The jump skips the Debug and the instruction after it, and lands on the instruction it belongs to.
void DebugInstructions::appendJumpOverDebug(const I &before)
{
    QList<int> &labels = m_generator->labels;
    I jump = makeJumpInstruction(Instruction::Jump(), before.line, before.statement);
    jump.linkedLabel = labels.size();
    labels.append(m_generator->instructions.size() + 2);
    m_generator->labelReachable.append(true);
    m_generator->instructions.append(jump);
}

void DebugInstructions::run()
{
    // Source locations are only stored for AOT compilation, which doesn't involve the debugger.
    Q_ASSERT(!m_generator->m_sourceLocationTable);

    const QList<I> instructions = std::exchange(m_generator->instructions, {});

    QList<int> &labels = m_generator->labels;
    const qsizetype labelCount = labels.size();

    // The debugger checks for breakpoints when it executes a Debug instruction. A jump from
    // another line has to land on one, or the target line is skipped. Record the lines of the
    // jumps to each label. Only forward jumps count. Loops are entered by fall-through and have to
    // take care of their backward jumps themselves. An exception handler is not entered by a
    // jump, but when the exception happens.
    std::vector<int> labelJumpLines(labelCount, NoJumpLine);
    for (qsizetype i = 0, end = instructions.size(); i != end; ++i) {
        const I &instruction = instructions[i];
        if (instruction.linkedLabel == -1 || instruction.type == Instr::Type::SetUnwindHandler)
            continue;
        if (labels[instruction.linkedLabel] <= i)
            continue;
        int &jumpLine = labelJumpLines[instruction.linkedLabel];
        jumpLine = mergeJumpLines(jumpLine, instruction.line);
    }

    // Several labels can be linked at the same position. Labels without a position are dead.
    std::vector<int> positionJumpLines(instructions.size(), NoJumpLine);
    for (qsizetype i = 0; i < labelCount; ++i) {
        if (labels[i] < 0)
            continue;
        int &jumpLine = positionJumpLines[labels[i]];
        jumpLine = mergeJumpLines(jumpLine, labelJumpLines[i]);
    }

    // Where each instruction ends up, and where the instructions inserted in front of it go.
    struct Position
    {
        int first = -1;
        int debug = -1;
        int instruction = -1;
        bool jumpOverDebug = false;
    };
    std::vector<Position> positions(instructions.size());

    for (qsizetype i = 0, end = instructions.size(); i != end; ++i) {
        Position &position = positions[i];
        position.first = m_generator->instructions.size();
        const I &instruction = instructions[i];

        // Without an explicit location, the instruction is just a leftover with the line of the
        // code before. Pausing on that line again would be wrong.
        const bool jumpFromOtherLine = instruction.locationSetAfterLabel
                && isFromOtherLine(positionJumpLines[i], instruction.line);

        if (i == 0 || instruction.line != instructions[i - 1].line) {
            appendDebug(instruction.line, instruction);
        } else if (jumpFromOtherLine) {
            // If the code before falls through to the instruction on the same line, it has passed
            // a Debug for the line already. Let it jump over the new Debug.
            if (!BytecodeGenerator::endsFlow(instructions[i - 1].type)) {
                appendJumpOverDebug(instruction);
                position.jumpOverDebug = true;
            }
            position.debug = m_generator->instructions.size();
            appendDebug(instruction.line, instruction);
        } else if (instruction.type == Instr::Type::Ret) {
            // The negated line never matches a breakpoint.
            appendDebug(-instruction.line, instruction);
        }

        position.instruction = m_generator->instructions.size();
        m_generator->instructions.append(instruction);
    }

    for (qsizetype i = 0; i < labelCount; ++i) {
        int &label = labels[i];
        Q_ASSERT(label != BytecodeGenerator::Label::Unlinked);
        // Unreachable labels don't have a position.
        if (label == BytecodeGenerator::Label::Dead)
            continue;
        Q_ASSERT(label < instructions.size());
        const Position &position = positions[label];
        if (!position.jumpOverDebug) {
            label = position.first;
            continue;
        }

        // Loop heads, exception handlers, and the targets of jumps within the line are not entered
        // by a jump from another line. They continue after the Debug.
        label = isFromOtherLine(labelJumpLines[i], instructions[label].line)
                ? position.debug
                : position.instruction;
    }
}

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE
