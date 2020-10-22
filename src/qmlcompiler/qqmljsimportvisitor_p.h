/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QQMLJSIMPORTEDMEMBERSVISITOR_P_H
#define QQMLJSIMPORTEDMEMBERSVISITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"

#include <private/qqmljsast_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljsimporter_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSImportVisitor : public QQmlJS::AST::Visitor
{
public:
    QQmlJSImportVisitor(QQmlJSImporter *importer, const QString &implicitImportDirectory,
                        const QStringList &qmltypesFiles = QStringList());

    QQmlJSScope::Ptr result() const;
    QList<QQmlJS::DiagnosticMessage> errors() const { return m_errors; }
    QHash<QString, QQmlJSScope::ConstPtr> imports() const { return m_rootScopeImports; }
    QHash<QString, QQmlJSScope::ConstPtr> addressableScopes() const { return m_scopesById; }

protected:
    bool visit(QQmlJS::AST::UiProgram *) override;
    void endVisit(QQmlJS::AST::UiProgram *) override;
    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiPublicMember *) override;
    bool visit(QQmlJS::AST::UiScriptBinding *) override;
    void endVisit(QQmlJS::AST::UiScriptBinding *) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *uied) override;
    bool visit(QQmlJS::AST::FunctionExpression *fexpr) override;
    void endVisit(QQmlJS::AST::FunctionExpression *) override;
    bool visit(QQmlJS::AST::FunctionDeclaration *fdecl) override;
    void endVisit(QQmlJS::AST::FunctionDeclaration *) override;
    bool visit(QQmlJS::AST::ClassExpression *ast) override;
    void endVisit(QQmlJS::AST::ClassExpression *) override;
    bool visit(QQmlJS::AST::UiImport *import) override;
    bool visit(QQmlJS::AST::ClassDeclaration *ast) override;
    void endVisit(QQmlJS::AST::ClassDeclaration *ast) override;
    bool visit(QQmlJS::AST::ForStatement *ast) override;
    void endVisit(QQmlJS::AST::ForStatement *ast) override;
    bool visit(QQmlJS::AST::ForEachStatement *ast) override;
    void endVisit(QQmlJS::AST::ForEachStatement *ast) override;
    bool visit(QQmlJS::AST::Block *ast) override;
    void endVisit(QQmlJS::AST::Block *ast) override;
    bool visit(QQmlJS::AST::CaseBlock *ast) override;
    void endVisit(QQmlJS::AST::CaseBlock *ast) override;
    bool visit(QQmlJS::AST::Catch *ast) override;
    void endVisit(QQmlJS::AST::Catch *ast) override;
    bool visit(QQmlJS::AST::WithStatement *withStatement) override;
    void endVisit(QQmlJS::AST::WithStatement *ast) override;

    bool visit(QQmlJS::AST::VariableDeclarationList *vdl) override;
    bool visit(QQmlJS::AST::FormalParameterList *fpl) override;

    bool visit(QQmlJS::AST::UiObjectBinding *uiob) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *uiob) override;

    void throwRecursionDepthError() override;

    QString m_implicitImportDirectory;
    QStringList m_qmltypesFiles;
    QQmlJSScope::Ptr m_currentScope;
    QQmlJSScope::Ptr m_qmlRootScope;
    QQmlJSScope::ConstPtr m_globalScope;
    QHash<QString, QQmlJSScope::ConstPtr> m_scopesById;
    QHash<QString, QQmlJSScope::ConstPtr> m_rootScopeImports;
    QQmlJSImporter *m_importer;

    QList<QQmlJS::DiagnosticMessage> m_errors;

    void enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                          const QQmlJS::SourceLocation &location);
    void leaveEnvironment();

private:
    void resolveAliases();
    void visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr);
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTEDMEMBERSVISITOR_P_H
