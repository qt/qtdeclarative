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

#ifndef FINDUNQUALIFIED_H
#define FINDUNQUALIFIED_H

#include "qmljstypedescriptionreader.h"
#include "qcoloroutput_p.h"

#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>

#include <QScopedPointer>

class ScopeTree;
enum class ScopeType;

class FindUnqualifiedIDVisitor : public QQmlJS::AST::Visitor {

public:
    explicit FindUnqualifiedIDVisitor(QStringList const &qmltypeDirs, const QString& code, const QString& fileName);
    ~FindUnqualifiedIDVisitor() override;
    bool check();

private:
    QScopedPointer<ScopeTree> m_rootScope;
    ScopeTree *m_currentScope;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> m_exportedName2MetaObject;
    QStringList m_qmltypeDirs;
    const QString& m_code;
    QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> m_qmlid2meta;
    QString m_rootId;
    QString m_filePath;
    QSet<QPair<QString, QString>> m_alreadySeenImports;
    QSet<QString> m_unknownImports;
    ColorOutput m_colorOut;
    bool m_visitFailed = false;

    struct OutstandingConnection {QString targetName; ScopeTree *scope; QQmlJS::AST::UiObjectDefinition *uiod;};

    QVarLengthArray<OutstandingConnection, 3> m_outstandingConnections; // Connections whose target we have not encountered

    void enterEnvironment(ScopeType type, QString name);
    void leaveEnvironment();
    void importHelper(QString id, QString prefix, int major, int minor);
    LanguageUtils::FakeMetaObject* localQmlFile2FakeMetaObject(QString filePath);


    void importExportedNames(QStringRef prefix, QString name);

    void throwRecursionDepthError() override;

    // start block/scope handling
    bool visit(QQmlJS::AST::UiProgram *ast) override;
    void endVisit(QQmlJS::AST::UiProgram *ast) override;

    bool visit(QQmlJS::AST::ClassExpression *ast) override;
    void endVisit(QQmlJS::AST::ClassExpression *ast) override;

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

    void visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr);
    bool visit(QQmlJS::AST::FunctionExpression *fexpr) override;
    void endVisit(QQmlJS::AST::FunctionExpression *fexpr) override;

    bool visit(QQmlJS::AST::FunctionDeclaration *fdecl) override;
    void endVisit(QQmlJS::AST::FunctionDeclaration *fdecl) override;
    /* --- end block handling --- */

    bool visit(QQmlJS::AST::VariableDeclarationList *vdl) override;
    bool visit(QQmlJS::AST::FormalParameterList *fpl) override;

    bool visit(QQmlJS::AST::UiImport *import) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *uied) override;
    bool visit(QQmlJS::AST::UiObjectBinding *uiob) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *uiob) override;
    bool visit(QQmlJS::AST::UiObjectDefinition *uiod) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiScriptBinding *uisb) override;
    bool visit(QQmlJS::AST::UiPublicMember *uipm) override;

    // expression handling
    bool visit(QQmlJS::AST::IdentifierExpression *idexp) override;
};


#endif // FINDUNQUALIFIED_H
