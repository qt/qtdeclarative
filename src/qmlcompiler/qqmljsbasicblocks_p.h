// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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


#include <private/qflatmap_p.h>
#include <private/qqmljscompilepass_p.h>
#include <private/qqmljscompiler_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSBasicBlocks : public QQmlJSCompilePass
{
public:
    using Conversions = QSet<int>;

    struct BasicBlock {
        QList<int> jumpOrigins;
        QList<int> readRegisters;
        QList<QQmlJSScope::ConstPtr> readTypes;
        int jumpTarget = -1;
        bool jumpIsUnconditional = false;
        bool isReturnBlock = false;
        bool isThrowBlock = false;
    };

    QQmlJSBasicBlocks(const QV4::Compiler::Context *context,
                      const QV4::Compiler::JSUnitGenerator *unitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : QQmlJSCompilePass(unitGenerator, typeResolver, logger), m_context{ context }
    {
    }

    ~QQmlJSBasicBlocks() = default;

    InstructionAnnotations run(const Function *function, const InstructionAnnotations &annotations,
                               QQmlJS::DiagnosticMessage *error, QQmlJSAotCompiler::Flags,
                               bool &basicBlocksValidationFailed);

    struct BasicBlocksValidationResult { bool success = true; QString errorMessage; };
    BasicBlocksValidationResult basicBlocksValidation();

private:
    struct RegisterAccess
    {
        QList<QQmlJSScope::ConstPtr> trackedTypes;
        QHash<int, QQmlJSScope::ConstPtr> typeReaders;
        QHash<int, Conversions> registerReadersAndConversions;
        int trackedRegister;
    };

    struct ObjectOrArrayDefinition
    {
        enum {
            ArrayClassId = -1,
            ArrayConstruct1ArgId = -2,
        };

        int instructionOffset = -1;
        int internalClassId = ArrayClassId;
        int argc = 0;
        int argv = -1;
    };

    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type type) override;
    void endInstruction(QV4::Moth::Instr::Type type) override;

    void generate_Jump(int offset) override;
    void generate_JumpTrue(int offset) override;
    void generate_JumpFalse(int offset) override;
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;
    void generate_IteratorNext(int value, int offset) override;
    void generate_GetOptionalLookup(int index, int offset) override;

    void generate_Ret() override;
    void generate_ThrowException() override;

    void generate_DefineArray(int argc, int argv) override;
    void generate_DefineObjectLiteral(int internalClassId, int argc, int args) override;
    void generate_Construct(int func, int argc, int argv) override;

    enum JumpMode { Unconditional, Conditional };
    void processJump(int offset, JumpMode mode);
    void populateBasicBlocks();
    void populateReaderLocations();
    void adjustTypes();
    bool canMove(int instructionOffset, const RegisterAccess &access) const;

    QFlatMap<int, BasicBlock>::iterator
    basicBlockForInstruction(QFlatMap<int, BasicBlock> &container, int instructionOffset);
    QFlatMap<int, BasicBlock>::const_iterator
    basicBlockForInstruction(const QFlatMap<int, BasicBlock> &container, int instructionOffset) const;

    void dumpBasicBlocks();
    void dumpDOTGraph();

    const QV4::Compiler::Context *m_context;
    InstructionAnnotations m_annotations;
    QFlatMap<int, BasicBlock> m_basicBlocks;
    QHash<int, RegisterAccess> m_readerLocations;
    QList<ObjectOrArrayDefinition> m_objectAndArrayDefinitions;
    bool m_skipUntilNextLabel = false;
    bool m_hadBackJumps = false;
};

QT_END_NAMESPACE

#endif // QQMLJSBASICBLOCKS_P_H
