/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
**/
#ifndef ASTDUMPER_H
#define ASTDUMPER_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmljsglobal_p.h>
#include <private/qqmljsastvisitor_p.h>
#include <QtCore/QString>
#include <functional>
#include <ostream>

QT_BEGIN_NAMESPACE
class QDebug;

namespace QQmlJS {

enum class DumperOptions {
    None=0,
    NoLocations=0x1,
    NoAnnotations=0x2,
    DumpNode=0x4
};
bool operator & (DumperOptions lhs, DumperOptions rhs);
DumperOptions operator | (DumperOptions lhs, DumperOptions rhs);

// no export, currently just a supporting file...
class AstDumper: public AST::BaseVisitor
{
public:
    static QString printNode2(AST::Node *);

    static QString diff(AST::Node *n1, AST::Node *n2, int nContext=3, DumperOptions opt=DumperOptions::None, int indent=0);
    static QString printNode(AST::Node *n, DumperOptions opt=DumperOptions::None, int indent=1, int baseIndent=0);

    AstDumper(const std::function <void (const QString &)> &dumper, DumperOptions options=DumperOptions::None,
              int indent=1, int baseIndent=0);

    void start(const QString &str);
    void start(const char *str);
    void stop(const QString &str);
    void stop(const char *str);

    QString qs(const QString &s);
    QString qs(const char *s);
    QString qs(const QStringRef &s);

    QString loc(const SourceLocation &s);

    QString boolStr(bool v);

    bool preVisit(AST::Node *el) override;
    void postVisit(AST::Node *el) override;

    // Ui
    bool visit(AST::UiProgram *el) override;
    bool visit(AST::UiHeaderItemList *) override;
    bool visit(AST::UiPragma *el) override;
    bool visit(AST::UiImport *el) override;
    bool visit(AST::UiPublicMember *el) override;
    bool visit(AST::UiSourceElement *) override;
    bool visit(AST::UiObjectDefinition *) override;
    bool visit(AST::UiObjectInitializer *) override;
    bool visit(AST::UiObjectBinding *) override;
    bool visit(AST::UiScriptBinding *) override;
    bool visit(AST::UiArrayBinding *) override;
    bool visit(AST::UiParameterList *) override;
    bool visit(AST::UiObjectMemberList *) override;
    bool visit(AST::UiArrayMemberList *) override;
    bool visit(AST::UiQualifiedId *) override;
    bool visit(AST::UiEnumDeclaration *) override;
    bool visit(AST::UiEnumMemberList *) override;
    bool visit(AST::UiVersionSpecifier *) override;
    bool visit(AST::UiInlineComponent *) override;
    bool visit(AST::UiRequired *) override;
    bool visit(AST::UiAnnotation *) override;
    bool visit(AST::UiAnnotationList *) override;

    void endVisit(AST::UiProgram *) override;
    void endVisit(AST::UiImport *) override;
    void endVisit(AST::UiHeaderItemList *) override;
    void endVisit(AST::UiPragma *) override;
    void endVisit(AST::UiPublicMember *) override;
    void endVisit(AST::UiSourceElement *) override;
    void endVisit(AST::UiObjectDefinition *) override;
    void endVisit(AST::UiObjectInitializer *) override;
    void endVisit(AST::UiObjectBinding *) override;
    void endVisit(AST::UiScriptBinding *) override;
    void endVisit(AST::UiArrayBinding *) override;
    void endVisit(AST::UiParameterList *) override;
    void endVisit(AST::UiObjectMemberList *) override;
    void endVisit(AST::UiArrayMemberList *) override;
    void endVisit(AST::UiQualifiedId *) override;
    void endVisit(AST::UiEnumDeclaration *) override;
    void endVisit(AST::UiEnumMemberList *) override;
    void endVisit(AST::UiVersionSpecifier *) override;
    void endVisit(AST::UiInlineComponent *) override;
    void endVisit(AST::UiRequired *) override;
    void endVisit(AST::UiAnnotation *) override;
    void endVisit(AST::UiAnnotationList *) override;

    // QQmlJS
    bool visit(AST::ThisExpression *) override;
    void endVisit(AST::ThisExpression *) override;

    bool visit(AST::IdentifierExpression *) override;
    void endVisit(AST::IdentifierExpression *) override;

    bool visit(AST::NullExpression *) override;
    void endVisit(AST::NullExpression *) override;

    bool visit(AST::TrueLiteral *) override;
    void endVisit(AST::TrueLiteral *) override;

    bool visit(AST::FalseLiteral *) override;
    void endVisit(AST::FalseLiteral *) override;

    bool visit(AST::SuperLiteral *) override;
    void endVisit(AST::SuperLiteral *) override;

    bool visit(AST::StringLiteral *) override;
    void endVisit(AST::StringLiteral *) override;

    bool visit(AST::TemplateLiteral *) override;
    void endVisit(AST::TemplateLiteral *) override;

    bool visit(AST::NumericLiteral *) override;
    void endVisit(AST::NumericLiteral *) override;

    bool visit(AST::RegExpLiteral *) override;
    void endVisit(AST::RegExpLiteral *) override;

    bool visit(AST::ArrayPattern *) override;
    void endVisit(AST::ArrayPattern *) override;

    bool visit(AST::ObjectPattern *) override;
    void endVisit(AST::ObjectPattern *) override;

    bool visit(AST::PatternElementList *) override;
    void endVisit(AST::PatternElementList *) override;

    bool visit(AST::PatternPropertyList *) override;
    void endVisit(AST::PatternPropertyList *) override;

    bool visit(AST::PatternElement *) override;
    void endVisit(AST::PatternElement *) override;

    bool visit(AST::PatternProperty *) override;
    void endVisit(AST::PatternProperty *) override;

    bool visit(AST::Elision *) override;
    void endVisit(AST::Elision *) override;

    bool visit(AST::NestedExpression *) override;
    void endVisit(AST::NestedExpression *) override;

    bool visit(AST::IdentifierPropertyName *) override;
    void endVisit(AST::IdentifierPropertyName *) override;

    bool visit(AST::StringLiteralPropertyName *) override;
    void endVisit(AST::StringLiteralPropertyName *) override;

    bool visit(AST::NumericLiteralPropertyName *) override;
    void endVisit(AST::NumericLiteralPropertyName *) override;

    bool visit(AST::ComputedPropertyName *) override;
    void endVisit(AST::ComputedPropertyName *) override;

    bool visit(AST::ArrayMemberExpression *) override;
    void endVisit(AST::ArrayMemberExpression *) override;

    bool visit(AST::FieldMemberExpression *) override;
    void endVisit(AST::FieldMemberExpression *) override;

    bool visit(AST::TaggedTemplate *) override;
    void endVisit(AST::TaggedTemplate *) override;

    bool visit(AST::NewMemberExpression *) override;
    void endVisit(AST::NewMemberExpression *) override;

    bool visit(AST::NewExpression *) override;
    void endVisit(AST::NewExpression *) override;

    bool visit(AST::CallExpression *) override;
    void endVisit(AST::CallExpression *) override;

    bool visit(AST::ArgumentList *) override;
    void endVisit(AST::ArgumentList *) override;

    bool visit(AST::PostIncrementExpression *) override;
    void endVisit(AST::PostIncrementExpression *) override;

    bool visit(AST::PostDecrementExpression *) override;
    void endVisit(AST::PostDecrementExpression *) override;

    bool visit(AST::DeleteExpression *) override;
    void endVisit(AST::DeleteExpression *) override;

    bool visit(AST::VoidExpression *) override;
    void endVisit(AST::VoidExpression *) override;

    bool visit(AST::TypeOfExpression *) override;
    void endVisit(AST::TypeOfExpression *) override;

    bool visit(AST::PreIncrementExpression *) override;
    void endVisit(AST::PreIncrementExpression *) override;

    bool visit(AST::PreDecrementExpression *) override;
    void endVisit(AST::PreDecrementExpression *) override;

    bool visit(AST::UnaryPlusExpression *) override;
    void endVisit(AST::UnaryPlusExpression *) override;

    bool visit(AST::UnaryMinusExpression *) override;
    void endVisit(AST::UnaryMinusExpression *) override;

    bool visit(AST::TildeExpression *) override;
    void endVisit(AST::TildeExpression *) override;

    bool visit(AST::NotExpression *) override;
    void endVisit(AST::NotExpression *) override;

    bool visit(AST::BinaryExpression *) override;
    void endVisit(AST::BinaryExpression *) override;

    bool visit(AST::ConditionalExpression *) override;
    void endVisit(AST::ConditionalExpression *) override;

    bool visit(AST::Expression *) override;
    void endVisit(AST::Expression *) override;

    bool visit(AST::Block *) override;
    void endVisit(AST::Block *) override;

    bool visit(AST::StatementList *) override;
    void endVisit(AST::StatementList *) override;

    bool visit(AST::VariableStatement *) override;
    void endVisit(AST::VariableStatement *) override;

    bool visit(AST::VariableDeclarationList *) override;
    void endVisit(AST::VariableDeclarationList *) override;

    bool visit(AST::EmptyStatement *) override;
    void endVisit(AST::EmptyStatement *) override;

    bool visit(AST::ExpressionStatement *) override;
    void endVisit(AST::ExpressionStatement *) override;

    bool visit(AST::IfStatement *) override;
    void endVisit(AST::IfStatement *) override;

    bool visit(AST::DoWhileStatement *) override;
    void endVisit(AST::DoWhileStatement *) override;

    bool visit(AST::WhileStatement *) override;
    void endVisit(AST::WhileStatement *) override;

    bool visit(AST::ForStatement *) override;
    void endVisit(AST::ForStatement *) override;

    bool visit(AST::ForEachStatement *) override;
    void endVisit(AST::ForEachStatement *) override;

    bool visit(AST::ContinueStatement *) override;
    void endVisit(AST::ContinueStatement *) override;

    bool visit(AST::BreakStatement *) override;
    void endVisit(AST::BreakStatement *) override;

    bool visit(AST::ReturnStatement *) override;
    void endVisit(AST::ReturnStatement *) override;

    bool visit(AST::YieldExpression *) override;
    void endVisit(AST::YieldExpression *) override;

    bool visit(AST::WithStatement *) override;
    void endVisit(AST::WithStatement *) override;

    bool visit(AST::SwitchStatement *) override;
    void endVisit(AST::SwitchStatement *) override;

    bool visit(AST::CaseBlock *) override;
    void endVisit(AST::CaseBlock *) override;

    bool visit(AST::CaseClauses *) override;
    void endVisit(AST::CaseClauses *) override;

    bool visit(AST::CaseClause *) override;
    void endVisit(AST::CaseClause *) override;

    bool visit(AST::DefaultClause *) override;
    void endVisit(AST::DefaultClause *) override;

    bool visit(AST::LabelledStatement *) override;
    void endVisit(AST::LabelledStatement *) override;

    bool visit(AST::ThrowStatement *) override;
    void endVisit(AST::ThrowStatement *) override;

    bool visit(AST::TryStatement *) override;
    void endVisit(AST::TryStatement *) override;

    bool visit(AST::Catch *) override;
    void endVisit(AST::Catch *) override;

    bool visit(AST::Finally *) override;
    void endVisit(AST::Finally *) override;

    bool visit(AST::FunctionDeclaration *) override;
    void endVisit(AST::FunctionDeclaration *) override;

    bool visit(AST::FunctionExpression *) override;
    void endVisit(AST::FunctionExpression *) override;

    bool visit(AST::FormalParameterList *) override;
    void endVisit(AST::FormalParameterList *) override;

    bool visit(AST::ClassExpression *) override;
    void endVisit(AST::ClassExpression *) override;

    bool visit(AST::ClassDeclaration *) override;
    void endVisit(AST::ClassDeclaration *) override;

    bool visit(AST::ClassElementList *) override;
    void endVisit(AST::ClassElementList *) override;

    bool visit(AST::Program *) override;
    void endVisit(AST::Program *) override;

    bool visit(AST::NameSpaceImport *) override;
    void endVisit(AST::NameSpaceImport *) override;

    bool visit(AST::ImportSpecifier *) override;
    void endVisit(AST::ImportSpecifier *) override;

    bool visit(AST::ImportsList *) override;
    void endVisit(AST::ImportsList *) override;

    bool visit(AST::NamedImports *) override;
    void endVisit(AST::NamedImports *) override;

    bool visit(AST::FromClause *) override;
    void endVisit(AST::FromClause *) override;

    bool visit(AST::ImportClause *) override;
    void endVisit(AST::ImportClause *) override;

    bool visit(AST::ImportDeclaration *) override;
    void endVisit(AST::ImportDeclaration *) override;

    bool visit(AST::ExportSpecifier *) override;
    void endVisit(AST::ExportSpecifier *) override;

    bool visit(AST::ExportsList *) override;
    void endVisit(AST::ExportsList *) override;

    bool visit(AST::ExportClause *) override;
    void endVisit(AST::ExportClause *) override;

    bool visit(AST::ExportDeclaration *) override;
    void endVisit(AST::ExportDeclaration *) override;

    bool visit(AST::ESModule *) override;
    void endVisit(AST::ESModule *) override;

    bool visit(AST::DebuggerStatement *) override;
    void endVisit(AST::DebuggerStatement *) override;

    bool visit(AST::Type *) override;
    void endVisit(AST::Type *) override;

    bool visit(AST::TypeArgumentList *) override;
    void endVisit(AST::TypeArgumentList *) override;

    bool visit(AST::TypeAnnotation *) override;
    void endVisit(AST::TypeAnnotation *) override;

    void throwRecursionDepthError() override;

private:
    // attributes
    std::function <void (const QString &)> dumper;
    DumperOptions options = DumperOptions::None;
    int indent = 0;
    int baseIndent = 0;
    bool dumpNode();
    bool noLocations();
    bool noAnnotations();
};

QDebug operator<<(QDebug d, AST::Node *n);

std::ostream &operator<<(std::ostream &stream, AST::Node *n);

} // namespace AST

QT_END_NAMESPACE

#endif // ASTDUMPER_H
