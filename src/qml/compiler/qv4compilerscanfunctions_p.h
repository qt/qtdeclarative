/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
**
****************************************************************************/
#ifndef QV4COMPILERSCANFUNCTIONS_P_H
#define QV4COMPILERSCANFUNCTIONS_P_H

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

#include "private/qv4global_p.h"
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qv4util_p.h>
#include <QtCore/QStringList>
#include <QStack>

QT_BEGIN_NAMESPACE

using namespace QQmlJS;

namespace QV4 {

namespace Moth {
struct Instruction;
}

namespace CompiledData {
struct CompilationUnit;
}

namespace Compiler {

class Codegen;

class ScanFunctions: protected QQmlJS::AST::Visitor
{
    typedef QV4::TemporaryAssignment<bool> TemporaryBoolAssignment;
public:
    ScanFunctions(Codegen *cg, const QString &sourceCode, CompilationMode defaultProgramMode);
    void operator()(AST::Node *node);

    void enterGlobalEnvironment(CompilationMode compilationMode);
    void enterEnvironment(AST::Node *node, CompilationMode compilationMode);
    void leaveEnvironment();

    void enterQmlFunction(AST::FunctionDeclaration *ast)
    { enterFunction(ast, false); }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    void checkDirectivePrologue(AST::SourceElements *ast);

    void checkName(const QStringRef &name, const AST::SourceLocation &loc);
    bool formalsContainName(AST::FormalParameterList *parameters, const QString &name);

    bool visit(AST::Program *ast) override;
    void endVisit(AST::Program *) override;

    bool visit(AST::CallExpression *ast) override;
    bool visit(AST::NewMemberExpression *ast) override;
    bool visit(AST::ArrayLiteral *ast) override;
    bool visit(AST::VariableDeclaration *ast) override;
    bool visit(AST::IdentifierExpression *ast) override;
    bool visit(AST::ExpressionStatement *ast) override;
    bool visit(AST::FunctionExpression *ast) override;

    void enterFunction(AST::FunctionExpression *ast, bool enterName);

    void endVisit(AST::FunctionExpression *) override;

    bool visit(AST::ObjectLiteral *ast) override;

    bool visit(AST::PropertyGetterSetter *ast) override;
    void endVisit(AST::PropertyGetterSetter *) override;

    bool visit(AST::FunctionDeclaration *ast) override;
    void endVisit(AST::FunctionDeclaration *) override;

    bool visit(AST::TryStatement *ast) override;
    bool visit(AST::WithStatement *ast) override;

    bool visit(AST::DoWhileStatement *ast) override;
    bool visit(AST::ForStatement *ast) override;
    bool visit(AST::LocalForStatement *ast) override;
    bool visit(AST::ForEachStatement *ast) override;
    bool visit(AST::LocalForEachStatement *ast) override;
    bool visit(AST::ThisExpression *ast) override;

    bool visit(AST::Block *ast) override;

protected:
    void enterFunction(AST::Node *ast, const QString &name, AST::FormalParameterList *formals, AST::FunctionBody *body, AST::FunctionExpression *expr);

    void calcEscapingVariables();
// fields:
    Codegen *_cg;
    const QString _sourceCode;
    Context *_context;
    QStack<Context *> _contextStack;

    bool _allowFuncDecls;
    CompilationMode defaultProgramMode;

private:
    static constexpr AST::Node *astNodeForGlobalEnvironment = nullptr;
};

}

}

QT_END_NAMESPACE

#endif // QV4CODEGEN_P_H
