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
#include <private/qv4jsir_p.h>
#include <private/qqmljsastfwd_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace Moth;

void BytecodeGenerator::setLocation(const QQmlJS::AST::SourceLocation &loc)
{
    if (static_cast<int>(loc.startLine) == currentLine)
        return;
    currentLine = static_cast<int>(loc.startLine);
    Instruction::Line line;
    line.lineNumber = currentLine;
//    addInstruction(line);
}

unsigned BytecodeGenerator::newTemp()
{
    int t = function->currentTemp++;
    if (function->tempCount < function->currentTemp)
        function->tempCount = function->currentTemp;
    return t;
}

QByteArray BytecodeGenerator::finalize()
{
    QByteArray code;

    Instruction::Push push;
    push.instructionType = Instr::Push;
    push.value = quint32(function->tempCount);
    code.append(reinterpret_cast<const char *>(&push), InstrMeta<Instr::Push>::Size);

    // content
    QVector<int> instructionOffsets;
    instructionOffsets.reserve(instructions.size());
    for (const auto &i : qAsConst(instructions)) {
        instructionOffsets.append(code.size());
        code.append(reinterpret_cast<const char *>(&i.instr), i.size);
    }

    // resolve jumps
//    qDebug() << "resolving jumps";
    for (const auto &j : jumps) {
        Q_ASSERT(j.linkedLabel != -1);
        int linkedInstruction = labels.at(j.linkedLabel);
        Q_ASSERT(linkedInstruction != -1);
        int offset = instructionOffsets.at(j.instructionIndex) + j.offset;
//        qDebug() << "offset data is at" << offset;
        char *c = code.data() + offset;
        ptrdiff_t linkedInstructionOffset = instructionOffsets.at(linkedInstruction) - offset;
//        qDebug() << "linked instruction" << linkedInstruction << "at " << instructionOffsets.at(linkedInstruction);
        memcpy(c, &linkedInstructionOffset, sizeof(ptrdiff_t));
    }

    return code;
}
