// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSOPTIMIZATIONS_P_H
#define QQMLJSOPTIMIZATIONS_P_H

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

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_EXPORT QQmlJSOptimizations : public QQmlJSCompilePass
{
public:
    using Conversions = QSet<int>;

    QQmlJSOptimizations(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                        const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
                        const BasicBlocks &basicBlocks, const InstructionAnnotations &annotations,
                        QList<ObjectOrArrayDefinition> objectAndArrayDefinitions)
        : QQmlJSCompilePass(unitGenerator, typeResolver, logger, basicBlocks, annotations),
          m_objectAndArrayDefinitions{ std::move(objectAndArrayDefinitions) }
    {
    }

    ~QQmlJSOptimizations() = default;

    BlocksAndAnnotations run(const Function *function);

private:
    struct RegisterAccess
    {
        // Content written by this instruction, or contents merged/converted into this register.
        QList<QQmlJSRegisterContent> trackedTypes;

        // Instructions that genuinely consume the *value* this write produced (never renames/copies
        // of it), keyed by instruction offset, with the exact content each one reads it as.
        // Tracked by content identity, so this follows the value across register renames rather
        // than being tied to one register slot.
        // Drives the write's required output type (see QQmlJSTypeResolver::adjustTrackedType) and
        // whether it can be moved into its single use site (see canMove).
        QHash<int, QQmlJSRegisterContent> typeReaders;

        // Reads of this write's *specific register slot* while it's still the reaching definition
        // (i.e. before any later write overwrites that register), keyed by instruction offset,
        // together with the type-conversion instructions crossed between the write and each read.
        // Drives which type conversions are actually needed in codegen (see liveConversions in
        // adjustTypes) and, combined with typeReaders, whether a store is conversion-free enough to
        // move.
        QHash<int, Conversions> registerReadersAndConversions;

        // Register slot written to by this instruction.
        int trackedRegister;
    };

    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type) override
    {
        return ProcessInstruction;
    }
    void endInstruction(QV4::Moth::Instr::Type) override { }

    void populateBasicBlocks();
    void populateReaderLocationsTrackedTypes();
    void populateReaderLocationsReadersAndConversions();
    void adjustTypes();
    bool canMove(int instructionOffset, const RegisterAccess &access) const;

    void removeReadsFromErasedInstructions(const QFlatMap<int, InstructionAnnotation>::const_iterator &it);
    void removeDeadStoresUntilStable();
    bool eraseDeadStore(const InstructionAnnotations::iterator &it, bool &erasedReaders);

    QHash<int, RegisterAccess> m_readerLocations;
    QList<ObjectOrArrayDefinition> m_objectAndArrayDefinitions;
};

QT_END_NAMESPACE

#endif // QQMLJSOPTIMIZATIONS_P_H
