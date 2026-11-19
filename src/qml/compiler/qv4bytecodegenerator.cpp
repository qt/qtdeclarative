// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qv4bytecodegenerator_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qqmljsastfwd_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace Moth;

void BytecodeGenerator::setLocation(const QQmlJS::SourceLocation &loc)
{
    currentLine = static_cast<int>(loc.startLine);
    currentSourceLocation = loc;
    locationSetAfterLabel = true;
}

void BytecodeGenerator::incrementStatement()
{
    ++currentStatement;
}

int BytecodeGenerator::newRegister()
{
    int t = currentReg++;
    if (regCount < currentReg)
        regCount = currentReg;
    return t;
}

int BytecodeGenerator::newRegisterArray(int n)
{
    int t = currentReg;
    currentReg += n;
    if (regCount < currentReg)
        regCount = currentReg;
    return t;
}

void BytecodeGenerator::packInstruction(I &i)
{
    Instr::Type type = Instr::unpack(i.packed);
    Q_ASSERT(int(type) < int(Instr::Type::Count));
    type = Instr::narrowInstructionType(type);
    int instructionsAsInts[Instr::MaxArgCount] = {};
    int nMembers = Moth::InstrInfo::argumentCount[static_cast<int>(i.type)];
    uchar *code = i.packed + Instr::encodedLength(type);
    for (int j = 0; j < nMembers; ++j) {
        instructionsAsInts[j] = qFromLittleEndian<qint32>(code + j * sizeof(int));
    }
    enum {
        Normal,
        Wide
    } width = Normal;
    for (int n = 0; n < nMembers; ++n) {
        if (width == Normal && (static_cast<qint8>(instructionsAsInts[n]) != instructionsAsInts[n])) {
            width = Wide;
            break;
        }
    }
    code = i.packed;
    switch (width) {
    case Normal:
        code = Instr::pack(code, type);
        for (int n = 0; n < nMembers; ++n) {
            qint8 v = static_cast<qint8>(instructionsAsInts[n]);
            memcpy(code, &v, 1);
            code += 1;
        }
        i.size = code - i.packed;
        if (i.offsetForJump != -1)
            i.offsetForJump = i.size - 1;
        break;
    case Wide:
        // nothing to do
        break;
    }
}

void BytecodeGenerator::adjustJumpOffsets()
{
    for (int index = 0; index < instructions.size(); ++index) {
        auto &i = instructions[index];
        if (i.offsetForJump == -1) // no jump
            continue;
        Q_ASSERT(i.linkedLabel != -1 && isLabelValid(i.linkedLabel));
        const auto &linkedInstruction = instructions.at(labels.at(i.linkedLabel));
        qint8 *c = reinterpret_cast<qint8*>(i.packed + i.offsetForJump);
        int jumpOffset = linkedInstruction.position - (i.position + i.size);
//        qDebug() << "adjusting jump offset for instruction" << index << i.position << i.size << "offsetForJump" << i.offsetForJump << "target"
//                 << labels.at(i.linkedLabel) << linkedInstruction.position << "jumpOffset" << jumpOffset;
        Instr::Type type = Instr::unpack(i.packed);
        if (Instr::isWide(type)) {
            Q_ASSERT(i.offsetForJump == i.size - 4);
            qToLittleEndian<qint32>(jumpOffset, c);
        } else {
            Q_ASSERT(i.offsetForJump == i.size - 1);
            qint8 o = jumpOffset;
            Q_ASSERT(o == jumpOffset);
            *c = o;
        }
    }
}

void BytecodeGenerator::compressInstructions()
{
    // first round: compress all non jump instructions
    int position = 0;
    for (auto &i : instructions) {
        i.position = position;
        if (i.offsetForJump == -1)
            packInstruction(i);
        position += i.size;
    }

    adjustJumpOffsets();

    // compress all jumps
    position = 0;
    for (auto &i : instructions) {
        i.position = position;
        if (i.offsetForJump != -1)
            packInstruction(i);
        position += i.size;
    }

    // adjust once again, as the packing above could have changed offsets
    adjustJumpOffsets();
}

void BytecodeGenerator::finalize(Compiler::Context *context)
{
    if (debugInstructionPass)
        debugInstructionPass(this);

    compressInstructions();

    // collect content and line numbers
    QByteArray code;
    QList<CompiledData::CodeOffsetToLineAndStatement> lineAndStatementNumbers;

    currentLine = -1;
    currentStatement = -1;

    Q_UNUSED(startLine);
    for (qsizetype i = 0; i < instructions.size(); i++) {
        if (instructions[i].line != currentLine || instructions[i].statement != currentStatement) {
            currentLine = instructions[i].line;
            currentStatement = instructions[i].statement;
            CompiledData::CodeOffsetToLineAndStatement entry;
            entry.codeOffset = code.size();
            entry.line = currentLine;
            entry.statement = currentStatement;
            lineAndStatementNumbers.append(entry);
        }

        if (m_sourceLocationTable)
            m_sourceLocationTable->entries[i].offset = static_cast<quint32>(code.size());

        code.append(reinterpret_cast<const char *>(instructions[i].packed), instructions[i].size);
    }

    context->code = code;
    context->lineAndStatementNumberMapping = lineAndStatementNumbers;
    context->sourceLocationTable = std::move(m_sourceLocationTable);

    context->labelInfo.reserve(context->labelInfo.size() + _labelInfos.size());
    for (const auto &li : _labelInfos) {
        if (labelReachable.at(li.labelIndex))
            context->labelInfo.push_back(instructions.at(labels.at(li.labelIndex)).position);
    }
}

int BytecodeGenerator::addInstructionHelper(Instr::Type type, const Instr &i, int offsetOfOffset) {
    if (!reachable)
        return -1;

    if (lastInstrType == int(Instr::Type::StoreReg)) {
        if (type == Instr::Type::LoadReg) {
            if (i.as<Instruction::LoadReg>().reg == lastInstr.as<Instruction::StoreReg>().reg) {
                // value is already in the accumulator
                return -1;
            }
        }
        if (type == Instr::Type::MoveReg) {
            const auto move = i.as<Instruction::MoveReg>();
            if (move.srcReg == lastInstr.as<Instruction::StoreReg>().reg) {
                Instruction::StoreReg store;
                store.reg = move.destReg;
                addInstruction(store);
                return -1;
            }
        }
    }
    lastInstrType = int(type);
    lastInstr = i;

    const int pos = instructions.size();
    I instr = makeInstruction(type, i, offsetOfOffset, currentLine, currentStatement);
    instr.locationSetAfterLabel = locationSetAfterLabel;
    instructions.append(instr);
    if (m_sourceLocationTable)
        m_sourceLocationTable->entries.append({ 0, currentSourceLocation });

    if (endsFlow(type))
        reachable = false;

    return pos;
}
