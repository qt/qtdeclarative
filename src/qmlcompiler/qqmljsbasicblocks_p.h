/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLJSBASICBLOCKS_P_H
#define QQMLJSBASICBLOCKS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.


#include <private/qqmljscompilepass_p.h>
#include <private/qflatmap_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSBasicBlocks : public QQmlJSCompilePass
{
public:
    struct BasicBlock {
        QList<int> jumpOrigins;
        QList<QQmlJSScope::ConstPtr> readRegisters;
        int jumpTarget = -1;
        bool jumpIsUnconditional = false;
    };

    QQmlJSBasicBlocks(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : QQmlJSCompilePass(unitGenerator, typeResolver, logger)
    {
    }

    ~QQmlJSBasicBlocks() = default;

    InstructionAnnotations run(const Function *function, const InstructionAnnotations &annotations);

private:
    struct RegisterAccess
    {
        QList<QQmlJSScope::ConstPtr> trackedTypes;
        QHash<int, QQmlJSScope::ConstPtr> readers;
    };

    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type type) override;
    void endInstruction(QV4::Moth::Instr::Type type) override;

    void generate_Jump(int offset) override;
    void generate_JumpTrue(int offset) override;
    void generate_JumpFalse(int offset) override;
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;

    void generate_Ret() override;
    void generate_ThrowException() override;

    enum JumpMode { Unconditional, Conditional };
    void processJump(int offset, JumpMode mode);
    void populateBasicBlocks();
    void populateReaderLocations();
    void adjustTypes();

    InstructionAnnotations m_annotations;
    QFlatMap<int, BasicBlock> m_basicBlocks;
    QHash<int, RegisterAccess> m_readerLocations;
    bool m_skipUntilNextLabel = false;
    bool m_hadBackJumps = false;
};

QT_END_NAMESPACE

#endif // QQMLJSBASICBLOCKS_P_H
