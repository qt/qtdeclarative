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

#include <private/qv4bytecodegenerator_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qqmljsastfwd_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace Moth;

void BytecodeGenerator::setLocation(const QQmlJS::AST::SourceLocation &loc)
{
    currentLine = static_cast<int>(loc.startLine);
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
    int instructionsAsInts[sizeof(Instr)/sizeof(int)];
    int nMembers = Moth::Instr::argumentCount[static_cast<int>(i.type)];
    char *data = i.packed;
    Q_ASSERT(*data == static_cast<char>(Instr::Type::XWide));
    data += 2;
    memcpy(instructionsAsInts, data, nMembers*sizeof(int));
    enum {
        Normal,
        Wide,
        XWide
    } width = Normal;
    for (int n = 0; n < nMembers; ++n) {
        if (width == Normal && (static_cast<char>(instructionsAsInts[n]) != instructionsAsInts[n]))
            width = Wide;
        if (width == Wide && (static_cast<short>(instructionsAsInts[n]) != instructionsAsInts[n]))
            width = XWide;
    }
    char *code = i.packed;
    switch (width) {
    case Normal:
        *code++ = static_cast<char>(i.type);
        for (int n = 0; n < nMembers; ++n) {
            char v = static_cast<char>(instructionsAsInts[n]);
            memcpy(code, &v, 1);
            code += 1;
        }
        i.size = code - i.packed;
        if (i.offsetForJump != -1)
            i.offsetForJump = i.size - 1;
        break;
    case Wide:
        *code++ = static_cast<char>(Instr::Type::Wide);
        *code++ = static_cast<char>(i.type);
        for (int n = 0; n < nMembers; ++n) {
            short v = static_cast<short>(instructionsAsInts[n]);
            memcpy(code, &v, 2);
            code += 2;
        }
        i.size = code - i.packed;
        if (i.offsetForJump != -1)
            i.offsetForJump = i.size - 2;
        break;
    case XWide:
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
        Q_ASSERT(i.linkedLabel != -1 && labels.at(i.linkedLabel) != -1);
        const auto &linkedInstruction = instructions.at(labels.at(i.linkedLabel));
        char *c = i.packed + i.offsetForJump;
        int jumpOffset = linkedInstruction.position - (i.position + i.size);
//        qDebug() << "adjusting jump offset for instruction" << index << i.position << i.size << "offsetForJump" << i.offsetForJump << "target"
//                 << labels.at(i.linkedLabel) << linkedInstruction.position << "jumpOffset" << jumpOffset;
        if (*i.packed == static_cast<char>(Instr::Type::XWide)) {
            Q_ASSERT(i.offsetForJump == i.size - 4);
            memcpy(c, &jumpOffset, sizeof(int));
        } else if (*i.packed == static_cast<char>(Instr::Type::Wide)) {
            Q_ASSERT(i.offsetForJump == i.size - 2);
            short o = jumpOffset;
            Q_ASSERT(o == jumpOffset);
            memcpy(c, &o, sizeof(short));
        } else {
            Q_ASSERT(i.offsetForJump == i.size - 1);
            char o = jumpOffset;
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
    compressInstructions();

    // collect content and line numbers
    QByteArray code;
    QVector<int> lineNumbers;
    currentLine = startLine;
    for (const auto &i : qAsConst(instructions)) {
        if (i.line != currentLine) {
            Q_ASSERT(i.line > currentLine);
            while (currentLine < i.line) {
                lineNumbers.append(code.size());
                ++currentLine;
            }
        }
        code.append(i.packed, i.size);
    }

    context->code = code;
    context->lineNumberMapping = lineNumbers;
}

int BytecodeGenerator::addInstructionHelper(Instr::Type type, const Instr &i, int offsetOfOffset) {
    int pos = instructions.size();
    int s = Moth::Instr::argumentCount[static_cast<int>(type)]*sizeof(int);
    if (offsetOfOffset != -1)
        offsetOfOffset += 2;
    I instr{type, static_cast<short>(s + 2), 0, currentLine, offsetOfOffset, -1, "\0\0" };
    char *code = instr.packed;
    *code++ = static_cast<char>(Instr::Type::XWide);
    *code++ = static_cast<char>(type);
    memcpy(code, &i, s);
    instructions.append(instr);

    return pos;
}
