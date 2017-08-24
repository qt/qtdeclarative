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

void BytecodeGenerator::compressInstructions()
{
    for (auto &i : instructions) {
        Instr instr = i.instr;
        i.packed[0] = static_cast<char>(Instr::Type::XWide);
        i.packed[1] = static_cast<char>(i.type);
        memcpy(i.packed + 2, reinterpret_cast<char *>(&instr), i.size);
        i.size += 2;
        if (i.offsetForJump != -1)
            i.offsetForJump += 2;
    }
}

void BytecodeGenerator::finalize(Compiler::Context *context)
{
    QByteArray code;

    compressInstructions();

    // content
    QVector<int> instructionOffsets;
    QVector<int> lineNumbers;
    currentLine = startLine;
    instructionOffsets.reserve(instructions.size());
    for (const auto &i : qAsConst(instructions)) {
        if (i.line != currentLine) {
            Q_ASSERT(i.line > currentLine);
            while (currentLine < i.line) {
                lineNumbers.append(code.size());
                ++currentLine;
            }
        }
        instructionOffsets.append(code.size());
        code.append(i.packed, i.size);
    }

    // resolve jumps
//    qDebug() << "resolving jumps";
    for (int index = 0; index < instructions.size(); ++index) {
        const auto &i = instructions.at(index);
        if (i.offsetForJump == -1) // no jump
            continue;
        Q_ASSERT(i.linkedLabel != -1);
        int linkedInstruction = labels.at(i.linkedLabel);
        Q_ASSERT(linkedInstruction != -1);
        int offset = instructionOffsets.at(index) + i.offsetForJump;
//        qDebug() << "offset data is at" << offset;
        char *c = code.data() + offset;
        int linkedInstructionOffset = instructionOffsets.at(linkedInstruction) - instructionOffsets.at(index + 1);
//        qDebug() << "linked instruction" << linkedInstruction << "at " << instructionOffsets.at(linkedInstruction);
        memcpy(c, &linkedInstructionOffset, sizeof(int));
    }

    context->code = code;
    context->lineNumberMapping = lineNumbers;
}
