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

class DumpAstVisitor : protected QQmlJS::AST::Visitor
{
public:
    enum Indentation { Tabs, Spaces };

    DumpAstVisitor(QQmlJS::Engine *engine, QQmlJS::AST::Node *rootNode, CommentAstVisitor *comment,
                   int indentWidth, Indentation indentation);

    QString toString() const { return m_result; }

    bool preVisit(QQmlJS::AST::Node *) override;

    bool visit(QQmlJS::AST::UiScriptBinding *node) override;

    bool visit(QQmlJS::AST::UiArrayBinding *node) override;
    void endVisit(QQmlJS::AST::UiArrayBinding *node) override;

    bool visit(QQmlJS::AST::UiObjectBinding *node) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *node) override;

    bool visit(QQmlJS::AST::FunctionDeclaration *node) override;
    void endVisit(QQmlJS::AST::FunctionDeclaration *node) override;

    bool visit(QQmlJS::AST::UiInlineComponent *node) override;

    bool visit(QQmlJS::AST::UiObjectDefinition *node) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *node) override;

    bool visit(QQmlJS::AST::UiEnumDeclaration *node) override;
    void endVisit(QQmlJS::AST::UiEnumDeclaration *node) override;

    bool visit(QQmlJS::AST::UiEnumMemberList *node) override;
    bool visit(QQmlJS::AST::UiPublicMember *node) override;
    bool visit(QQmlJS::AST::UiImport *node) override;
    bool visit(QQmlJS::AST::UiPragma *node) override;

    bool visit(QQmlJS::AST::UiAnnotation *node) override;
    void endVisit(QQmlJS::AST::UiAnnotation *node) override;

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

        QQmlJS::AST::UiObjectMember* m_lastInArrayBinding = nullptr;
        QHash<QString, QQmlJS::AST::UiObjectMember*> m_bindings;
    };

    QString generateIndent(int indentLevel) const;
    QString formatLine(QString line, bool newline = true) const;
    QString formatPartlyFormatedLines(QString line, bool newline = true) const;

    QString formatComment(const Comment &comment) const;

    QString getComment(QQmlJS::AST::Node *node, Comment::Location location) const;
    QString getListItemComment(QQmlJS::SourceLocation srcLocation, Comment::Location location) const;

    void addNewLine(bool always = false);
    void addLine(QString line);

    QString getOrphanedComments(QQmlJS::AST::Node *node) const;

    QString parseStatement(QQmlJS::AST::Statement *statement, bool blockHasNext = false,
                           bool blockAllowBraceless = false);
    QString parseStatementList(QQmlJS::AST::StatementList *list);

    QString parseExpression(QQmlJS::AST::ExpressionNode *expression);

    QString parsePatternElement(QQmlJS::AST::PatternElement *element, bool scope = true);
    QString parsePatternElementList(QQmlJS::AST::PatternElementList *element);

    QString parsePatternProperty(QQmlJS::AST::PatternProperty *property);
    QString parsePatternPropertyList(QQmlJS::AST::PatternPropertyList *list);

    QString parseArgumentList(QQmlJS::AST::ArgumentList *list);

    QString parseUiParameterList(QQmlJS::AST::UiParameterList *list);

    QString parseVariableDeclarationList(QQmlJS::AST::VariableDeclarationList *list);

    QString parseCaseBlock(QQmlJS::AST::CaseBlock *block);
    QString parseBlock(QQmlJS::AST::Block *block, bool hasNext, bool allowBraceless);

    QString parseExportsList(QQmlJS::AST::ExportsList *list);
    QString parseExportSpecifier(QQmlJS::AST::ExportSpecifier *specifier);

    QString parseFormalParameterList(QQmlJS::AST::FormalParameterList *list);

    QString parseType(QQmlJS::AST::Type *type);

    QString parseFunctionExpression(QQmlJS::AST::FunctionExpression *expression, bool omitFunction = false);

    ScopeProperties& scope() { return m_scope_properties.top(); }

    int m_indentLevel = 0;

    bool m_error = false;
    bool m_blockNeededBraces = false;

    QStack<ScopeProperties> m_scope_properties;

    QString m_result = "";
    QString m_component_name = "";
    QQmlJS::Engine *m_engine;
    CommentAstVisitor *m_comment;
    int m_indentWidth;
    Indentation m_indentation;
};

#endif // DUMPAST_H
