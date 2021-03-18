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

#ifndef DUMPAST_H
#define DUMPAST_H

#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QHash>
#include <QStack>

#include "commentastvisitor.h"

using namespace QQmlJS::AST;
using namespace QQmlJS;

class DumpAstVisitor : protected Visitor
{
public:
    DumpAstVisitor(QQmlJS::Engine *engine, Node *rootNode, CommentAstVisitor *comment);

    QString toString() const { return m_result; }

    bool preVisit(Node *) override;

    bool visit(UiScriptBinding *node) override;

    bool visit(UiArrayBinding *node) override;
    void endVisit(UiArrayBinding *node) override;

    bool visit(UiObjectBinding *node) override;
    void endVisit(UiObjectBinding *node) override;

    bool visit(FunctionDeclaration *node) override;
    void endVisit(FunctionDeclaration *node) override;

    bool visit(UiInlineComponent *node) override;

    bool visit(UiObjectDefinition *node) override;
    void endVisit(UiObjectDefinition *node) override;

    bool visit(UiEnumDeclaration *node) override;
    void endVisit(UiEnumDeclaration *node) override;

    bool visit(UiEnumMemberList *node) override;
    bool visit(UiPublicMember *node) override;
    bool visit(UiImport *node) override;
    bool visit(UiPragma *node) override;

    bool visit(UiAnnotation *node) override;
    void endVisit(UiAnnotation *node) override;

    void throwRecursionDepthError() override {}

    bool error() const { return m_error; }
private:
    struct ScopeProperties {
        bool m_firstOfAll = true;
        bool m_firstSignal = true;
        bool m_firstProperty = true;
        bool m_firstBinding = true;
        bool m_firstObject = true;
        bool m_firstFunction = true;
        bool m_inArrayBinding = false;
        bool m_pendingBinding = false;

        UiObjectMember* m_lastInArrayBinding = nullptr;
        QHash<QString, UiObjectMember*> m_bindings;
    };

    QString generateIndent() const;
    QString formatLine(QString line, bool newline = true) const;

    QString formatComment(const Comment &comment) const;

    QString getComment(Node *node, Comment::Location location) const;
    QString getListItemComment(SourceLocation srcLocation, Comment::Location location) const;

    void addNewLine(bool always = false);
    void addLine(QString line);

    QString getOrphanedComments(Node *node) const;

    QString parseStatement(Statement *statement, bool blockHasNext = false,
                           bool blockAllowBraceless = false);
    QString parseStatementList(StatementList *list);

    QString parseExpression(ExpressionNode *expression);

    QString parsePatternElement(PatternElement *element, bool scope = true);
    QString parsePatternElementList(PatternElementList *element);

    QString parsePatternProperty(PatternProperty *property);
    QString parsePatternPropertyList(PatternPropertyList *list);

    QString parseArgumentList(ArgumentList *list);

    QString parseUiParameterList(UiParameterList *list);

    QString parseVariableDeclarationList(VariableDeclarationList *list);

    QString parseCaseBlock(CaseBlock *block);
    QString parseBlock(Block *block, bool hasNext, bool allowBraceless);

    QString parseExportsList(ExportsList *list);
    QString parseExportSpecifier(ExportSpecifier *specifier);

    QString parseFormalParameterList(FormalParameterList *list);

    QString parseType(Type *type);

    QString parseFunctionExpression(FunctionExpression *expression, bool omitFunction = false);

    ScopeProperties& scope() { return m_scope_properties.top(); }

    int m_indentLevel = 0;

    bool m_error = false;
    bool m_blockNeededBraces = false;

    QStack<ScopeProperties> m_scope_properties;

    QString m_result = "";
    QString m_component_name = "";
    QQmlJS::Engine *m_engine;
    CommentAstVisitor *m_comment;
};

#endif // DUMPAST_H
