/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4IRBUILDER_P_H
#define QV4IRBUILDER_P_H

#include <QtCore/qglobal.h>

#include "qv4ir_p.h"

QT_BEGIN_NAMESPACE

class QV4IRBuilder : public QQmlJS::AST::Visitor
{
public:
    QV4IRBuilder(const QV4Compiler::Expression *, QQmlEnginePrivate *);

    bool operator()(QQmlJS::IR::Function *, QQmlJS::AST::Node *, bool *invalidatable);

protected:
    struct ExprResult {
        enum Format {
            ex, // expression
            cx  // condition
        };

        QQmlJS::IR::Expr *code;
        QQmlJS::IR::BasicBlock *iftrue;
        QQmlJS::IR::BasicBlock *iffalse;
        Format hint; // requested format
        Format format; // instruction format

        ExprResult(QQmlJS::IR::Expr *expr = 0)
            : code(expr), iftrue(0), iffalse(0), hint(ex), format(ex) {}

        ExprResult(QQmlJS::IR::BasicBlock *iftrue, QQmlJS::IR::BasicBlock *iffalse)
            : code(0), iftrue(iftrue), iffalse(iffalse), hint(cx), format(ex) {}

        inline QQmlJS::IR::Type type() const { return code ? code->type : QQmlJS::IR::InvalidType; }

        inline QQmlJS::IR::Expr *get() const { return code; }
        inline operator QQmlJS::IR::Expr *() const  { return get(); }
        inline QQmlJS::IR::Expr *operator->() const { return get(); }
        inline bool isValid() const { return code ? code->type != QQmlJS::IR::InvalidType : false; }
        inline bool is(QQmlJS::IR::Type t) const { return type() == t; }
        inline bool isNot(QQmlJS::IR::Type t) const { return type() != t; }

        bool isPrimitive() const {
            switch (type()) {
            case QQmlJS::IR::UndefinedType: // ### TODO
            case QQmlJS::IR::NullType: // ### TODO
            case QQmlJS::IR::UrlType: // ### TODO
                return false;

            case QQmlJS::IR::StringType:
            case QQmlJS::IR::BoolType:
            case QQmlJS::IR::IntType:
            case QQmlJS::IR::FloatType:
            case QQmlJS::IR::NumberType:
                return true;

            default:
                return false;
            } // switch
        }
    };

    inline void accept(QQmlJS::AST::Node *ast) { QQmlJS::AST::Node::accept(ast, this); }

    ExprResult expression(QQmlJS::AST::ExpressionNode *ast);
    ExprResult statement(QQmlJS::AST::Statement *ast);
    void sourceElement(QQmlJS::AST::SourceElement *ast);
    void condition(QQmlJS::AST::ExpressionNode *ast, QQmlJS::IR::BasicBlock *iftrue, QQmlJS::IR::BasicBlock *iffalse);
    void binop(QQmlJS::AST::BinaryExpression *ast, ExprResult left, ExprResult right);

    void implicitCvt(ExprResult &expr, QQmlJS::IR::Type type);

    virtual bool preVisit(QQmlJS::AST::Node *ast);

    // QML
    virtual bool visit(QQmlJS::AST::UiProgram *ast);
    virtual bool visit(QQmlJS::AST::UiImportList *ast);
    virtual bool visit(QQmlJS::AST::UiImport *ast);
    virtual bool visit(QQmlJS::AST::UiPublicMember *ast);
    virtual bool visit(QQmlJS::AST::UiSourceElement *ast);
    virtual bool visit(QQmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QQmlJS::AST::UiObjectInitializer *ast);
    virtual bool visit(QQmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QQmlJS::AST::UiScriptBinding *ast);
    virtual bool visit(QQmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QQmlJS::AST::UiObjectMemberList *ast);
    virtual bool visit(QQmlJS::AST::UiArrayMemberList *ast);
    virtual bool visit(QQmlJS::AST::UiQualifiedId *ast);

    // JS
    virtual bool visit(QQmlJS::AST::Program *ast);
    virtual bool visit(QQmlJS::AST::SourceElements *ast);
    virtual bool visit(QQmlJS::AST::FunctionSourceElement *ast);
    virtual bool visit(QQmlJS::AST::StatementSourceElement *ast);

    // object literals
    virtual bool visit(QQmlJS::AST::PropertyAssignmentList *ast);
    virtual bool visit(QQmlJS::AST::PropertyNameAndValue *ast);
    virtual bool visit(QQmlJS::AST::PropertyGetterSetter *ast);
    virtual bool visit(QQmlJS::AST::IdentifierPropertyName *ast);
    virtual bool visit(QQmlJS::AST::StringLiteralPropertyName *ast);
    virtual bool visit(QQmlJS::AST::NumericLiteralPropertyName *ast);

    // array literals
    virtual bool visit(QQmlJS::AST::ElementList *ast);
    virtual bool visit(QQmlJS::AST::Elision *ast);

    // function calls
    virtual bool visit(QQmlJS::AST::ArgumentList *ast);

    // expressions
    virtual bool visit(QQmlJS::AST::ObjectLiteral *ast);
    virtual bool visit(QQmlJS::AST::ArrayLiteral *ast);
    virtual bool visit(QQmlJS::AST::ThisExpression *ast);
    virtual bool visit(QQmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QQmlJS::AST::NullExpression *ast);
    virtual bool visit(QQmlJS::AST::TrueLiteral *ast);
    virtual bool visit(QQmlJS::AST::FalseLiteral *ast);
    virtual bool visit(QQmlJS::AST::StringLiteral *ast);
    virtual bool visit(QQmlJS::AST::NumericLiteral *ast);
    virtual bool visit(QQmlJS::AST::RegExpLiteral *ast);
    virtual bool visit(QQmlJS::AST::NestedExpression *ast);
    virtual bool visit(QQmlJS::AST::ArrayMemberExpression *ast);
    virtual bool visit(QQmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QQmlJS::AST::NewMemberExpression *ast);
    virtual bool visit(QQmlJS::AST::NewExpression *ast);
    virtual bool visit(QQmlJS::AST::CallExpression *ast);
    virtual bool visit(QQmlJS::AST::PostIncrementExpression *ast);
    virtual bool visit(QQmlJS::AST::PostDecrementExpression *ast);
    virtual bool visit(QQmlJS::AST::DeleteExpression *ast);
    virtual bool visit(QQmlJS::AST::VoidExpression *ast);
    virtual bool visit(QQmlJS::AST::TypeOfExpression *ast);
    virtual bool visit(QQmlJS::AST::PreIncrementExpression *ast);
    virtual bool visit(QQmlJS::AST::PreDecrementExpression *ast);
    virtual bool visit(QQmlJS::AST::UnaryPlusExpression *ast);
    virtual bool visit(QQmlJS::AST::UnaryMinusExpression *ast);
    virtual bool visit(QQmlJS::AST::TildeExpression *ast);
    virtual bool visit(QQmlJS::AST::NotExpression *ast);
    virtual bool visit(QQmlJS::AST::BinaryExpression *ast);
    virtual bool visit(QQmlJS::AST::ConditionalExpression *ast);
    virtual bool visit(QQmlJS::AST::Expression *ast);

    // statements
    virtual bool visit(QQmlJS::AST::Block *ast);
    virtual bool visit(QQmlJS::AST::StatementList *ast);
    virtual bool visit(QQmlJS::AST::VariableStatement *ast);
    virtual bool visit(QQmlJS::AST::VariableDeclarationList *ast);
    virtual bool visit(QQmlJS::AST::VariableDeclaration *ast);
    virtual bool visit(QQmlJS::AST::EmptyStatement *ast);
    virtual bool visit(QQmlJS::AST::ExpressionStatement *ast);
    virtual bool visit(QQmlJS::AST::IfStatement *ast);
    virtual bool visit(QQmlJS::AST::DoWhileStatement *ast);
    virtual bool visit(QQmlJS::AST::WhileStatement *ast);
    virtual bool visit(QQmlJS::AST::ForStatement *ast);
    virtual bool visit(QQmlJS::AST::LocalForStatement *ast);
    virtual bool visit(QQmlJS::AST::ForEachStatement *ast);
    virtual bool visit(QQmlJS::AST::LocalForEachStatement *ast);
    virtual bool visit(QQmlJS::AST::ContinueStatement *ast);
    virtual bool visit(QQmlJS::AST::BreakStatement *ast);
    virtual bool visit(QQmlJS::AST::ReturnStatement *ast);
    virtual bool visit(QQmlJS::AST::WithStatement *ast);
    virtual bool visit(QQmlJS::AST::SwitchStatement *ast);
    virtual bool visit(QQmlJS::AST::CaseBlock *ast);
    virtual bool visit(QQmlJS::AST::CaseClauses *ast);
    virtual bool visit(QQmlJS::AST::CaseClause *ast);
    virtual bool visit(QQmlJS::AST::DefaultClause *ast);
    virtual bool visit(QQmlJS::AST::LabelledStatement *ast);
    virtual bool visit(QQmlJS::AST::ThrowStatement *ast);
    virtual bool visit(QQmlJS::AST::TryStatement *ast);
    virtual bool visit(QQmlJS::AST::Catch *ast);
    virtual bool visit(QQmlJS::AST::Finally *ast);
    virtual bool visit(QQmlJS::AST::FunctionDeclaration *ast);
    virtual bool visit(QQmlJS::AST::FunctionExpression *ast);
    virtual bool visit(QQmlJS::AST::FormalParameterList *ast);
    virtual bool visit(QQmlJS::AST::FunctionBody *ast);
    virtual bool visit(QQmlJS::AST::DebuggerStatement *ast);

private:
    bool buildName(QList<QStringRef> &name, QQmlJS::AST::Node *node,
                   QList<QQmlJS::AST::ExpressionNode *> *nodes);
    void discard();

    const QV4Compiler::Expression *m_expression;
    QQmlEnginePrivate *m_engine;

    QQmlJS::IR::Function *_function;
    QQmlJS::IR::BasicBlock *_block;
    bool _discard;
    bool _invalidatable;

    ExprResult _expr;
};

QT_END_NAMESPACE

#endif // QV4IRBUILDER_P_H 
