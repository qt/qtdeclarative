// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLINTERTYPEPROPAGATOR_P_H
#define QQMLJSLINTERTYPEPROPAGATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qduplicatetracker_p.h>
#include <private/qqmljstypepropagator_p.h>
#include <private/qqmljscontextproperties_p.h>
#include <private/qqmljsusercontextproperties_p.h>
#include <private/qqmljslinterrenamedcomponents_p.h>
#include <private/qqmljslintercontext_p.h>

QT_BEGIN_NAMESPACE

struct IdMemberShadow;

class QQmlJSLinterTypePropagator : public QQmlJSTypePropagator
{
public:
    QQmlJSLinterTypePropagator(const QV4::Compiler::JSUnitGenerator *unitGenerator,
                               const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger,
                               const QQmlJS::LinterContext &linterContext,
                               const BasicBlocks &basicBlocks = { },
                               const InstructionAnnotations &annotations = { },
                               QQmlSA::PassManager *passManager = nullptr);

    void setIdMemberShadows(QSet<IdMemberShadow> *idMemberShadows)
    {
        m_idMemberShadows = idMemberShadows;
    }

private:
    void generate_Ret() override;
    void generate_LoadQmlContextPropertyLookup(int index) override;
    void generate_GetOptionalLookup(int index, int offset) override;
    void generate_StoreProperty(int nameIndex, int base) override;
    void generate_CallProperty(int nameIndex, int base, int argc, int argv) override;
    void generate_CallPossiblyDirectEval(int argc, int argv) override;

    void handleUnqualifiedAccess(const QString &name, bool isMethod) const override;
    void handleUnqualifiedAccessAndContextProperties(const QString &name, bool isMethod) const override;
    void checkDeprecated(QQmlJSScope::ConstPtr scope, const QString &name, bool isMethod) const override;
    bool isCallingProperty(QQmlJSScope::ConstPtr scope, const QString &name) const override;
    bool handleImportNamespaceLookup(const QString &propertyName) override;
    void handleLookupError(const QString &propertyName) override;
    bool checkForEnumProblems(QQmlJSRegisterContent base, const QString &propertyName) override;
    void generate_StoreNameCommon(int nameIndex) override;
    void propagatePropertyLookup(const QString &name,
                                 int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex) override;
    void propagateCall(const QList<QQmlJSMetaMethod> &methods, int argc, int argv,
                       QQmlJSRegisterContent scope) override;
    void propagateTranslationMethod_SAcheck(const QString &methodName) override;
    void warnAboutTypeCoercion(int lhs) override;

    bool checkTypeResolved(const QQmlJSScope::ConstPtr &type);
    void checkWrite(const QQmlJSRegisterContent &callBase, const QString &propertyName);

    QQmlSA::PassManager *m_passManager = nullptr;
    const QQmlJS::LinterContext &m_context;
    QSet<IdMemberShadow> *m_idMemberShadows = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLJSLINTERTYPEPROPAGATOR_P_H
