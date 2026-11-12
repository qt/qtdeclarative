// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4debuginstructions_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Moth {

void DebugInstructions::insert(BytecodeGenerator *generator)
{
    DebugInstructions(generator).run();
}

void DebugInstructions::appendDebug(int line, const I &before)
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized") // broken gcc warns about Instruction::Debug()
    m_generator->instructions.append(
            BytecodeGenerator::makeInstruction(Instruction::Debug(), line, before.statement));
QT_WARNING_POP
}

void DebugInstructions::run()
{
    // Source locations are only stored for AOT compilation, which doesn't involve the debugger.
    Q_ASSERT(!m_generator->m_sourceLocationTable);

    QList<I> instructions = std::exchange(m_generator->instructions, {});

    // Remember where each instruction ends up, including the instructions inserted in front of
    // it. The labels refer to the old indices. They are not sorted, as they are numbered in order
    // of creation but bound in order of appearance, so we cannot update them on the fly. The
    // position isn't needed before the instructions are packed. Use it as scratch space.
    for (qsizetype i = 0, end = instructions.size(); i != end; ++i) {
        I &instruction = instructions[i];
        instruction.position = m_generator->instructions.size();
        if (i == 0 || instruction.line != instructions[i - 1].line) {
            appendDebug(instruction.line, instruction);
        } else if (instruction.type == Instr::Type::Ret) {
            // The negated line never matches a breakpoint.
            appendDebug(-instruction.line, instruction);
        }
        m_generator->instructions.append(instruction);
    }

    for (int &label : m_generator->labels) {
        Q_ASSERT(label != BytecodeGenerator::Label::Unlinked);
        // Unreachable labels don't have a position.
        if (label == BytecodeGenerator::Label::Dead)
            continue;
        Q_ASSERT(label < instructions.size());
        label = int(instructions[label].position);
    }
}

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE
