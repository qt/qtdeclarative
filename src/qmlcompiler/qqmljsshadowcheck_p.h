// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSSHADOWCHECK_P_H
#define QQMLJSSHADOWCHECK_P_H

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

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSShadowCheck : public QQmlJSCompilePass
{
public:
    QQmlJSShadowCheck(const QV4::Compiler::JSUnitGenerator *jsUnitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : QQmlJSCompilePass(jsUnitGenerator, typeResolver, logger)
    {}

    ~QQmlJSShadowCheck() = default;

    void run(InstructionAnnotations *annotations, const Function *function,
             QQmlJS::DiagnosticMessage *error);

private:
    struct ResettableStore {
        QQmlJSRegisterContent accumulatorIn;
        int instructionOffset = -1;
    };

    void handleStore(int base, const QString &memberName);

    void generate_LoadProperty(int nameIndex) override;
    void generate_GetLookup(int index) override;
    void generate_GetOptionalLookup(int index, int offset) override;
    void generate_StoreProperty(int nameIndex, int base) override;
    void generate_SetLookup(int index, int base) override;
    void generate_CallProperty(int nameIndex, int base, int argc, int argv) override;
    void generate_CallPropertyLookup(int nameIndex, int base, int argc, int argv) override;

    QV4::Moth::ByteCodeHandler::Verdict startInstruction(QV4::Moth::Instr::Type) override;
    void endInstruction(QV4::Moth::Instr::Type) override;

    enum Shadowability { NotShadowable, Shadowable };
    Shadowability checkShadowing(
            const QQmlJSRegisterContent &baseType, const QString &propertyName, int baseRegister);

    void checkResettable(const QQmlJSRegisterContent &accumulatorIn, int instructionOffset);

    Shadowability checkBaseType(const QQmlJSRegisterContent &baseType);

    QList<ResettableStore> m_resettableStores;
    QList<QQmlJSRegisterContent> m_baseTypes;
    QSet<QQmlJSRegisterContent> m_adjustedTypes;

    InstructionAnnotations *m_annotations = nullptr;
    State m_state;
};

QT_END_NAMESPACE

#endif // QQMLJSSHADOWCHECK_P_H
