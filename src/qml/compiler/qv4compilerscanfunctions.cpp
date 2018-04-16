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

#include "qv4compilerscanfunctions_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include <private/qqmljsast_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qv4codegen_p.h>
#include <private/qv4string_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace QV4::Compiler;
using namespace QQmlJS::AST;

ScanFunctions::ScanFunctions(Codegen *cg, const QString &sourceCode, CompilationMode defaultProgramMode)
    : _cg(cg)
    , _sourceCode(sourceCode)
    , _context(nullptr)
    , _allowFuncDecls(true)
    , defaultProgramMode(defaultProgramMode)
{
}

void ScanFunctions::operator()(Node *node)
{
    if (node)
        node->accept(this);

    calcEscapingVariables();
}

void ScanFunctions::enterGlobalEnvironment(CompilationMode compilationMode)
{
    enterEnvironment(astNodeForGlobalEnvironment, compilationMode);
}

void ScanFunctions::enterEnvironment(Node *node, CompilationMode compilationMode)
{
    Context *c = _cg->_module->contextMap.value(node);
    if (!c)
        c = _cg->_module->newContext(node, _context, compilationMode);
    if (!c->isStrict)
        c->isStrict = _cg->_strictMode;
    _contextStack.append(c);
    _context = c;
}

void ScanFunctions::leaveEnvironment()
{
    _contextStack.pop();
    _context = _contextStack.isEmpty() ? 0 : _contextStack.top();
}

void ScanFunctions::checkDirectivePrologue(SourceElements *ast)
{
    for (SourceElements *it = ast; it; it = it->next) {
        if (StatementSourceElement *stmt = cast<StatementSourceElement *>(it->element)) {
            if (ExpressionStatement *expr = cast<ExpressionStatement *>(stmt->statement)) {
                if (StringLiteral *strLit = cast<StringLiteral *>(expr->expression)) {
                    // Use the source code, because the StringLiteral's
                    // value might have escape sequences in it, which is not
                    // allowed.
                    if (strLit->literalToken.length < 2)
                        continue;
                    QStringRef str = _sourceCode.midRef(strLit->literalToken.offset + 1, strLit->literalToken.length - 2);
                    if (str == QLatin1String("use strict")) {
                        _context->isStrict = true;
                    } else {
                        // TODO: give a warning.
                    }
                    continue;
                }
            }
        }

        break;
    }
}

void ScanFunctions::checkName(const QStringRef &name, const SourceLocation &loc)
{
    if (_context->isStrict) {
        if (name == QLatin1String("implements")
                || name == QLatin1String("interface")
                || name == QLatin1String("let")
                || name == QLatin1String("package")
                || name == QLatin1String("private")
                || name == QLatin1String("protected")
                || name == QLatin1String("public")
                || name == QLatin1String("static")
                || name == QLatin1String("yield")) {
            _cg->throwSyntaxError(loc, QStringLiteral("Unexpected strict mode reserved word"));
        }
    }
}

bool ScanFunctions::formalsContainName(AST::FormalParameterList *parameters, const QString &name)
{
    while (parameters) {
        if (parameters->name == name)
            return true;
        parameters = parameters->next;
    }
    return false;
}

bool ScanFunctions::visit(Program *ast)
{
    enterEnvironment(ast, defaultProgramMode);
    checkDirectivePrologue(ast->elements);
    return true;
}

void ScanFunctions::endVisit(Program *)
{
    leaveEnvironment();
}

bool ScanFunctions::visit(CallExpression *ast)
{
    if (! _context->hasDirectEval) {
        if (IdentifierExpression *id = cast<IdentifierExpression *>(ast->base)) {
            if (id->name == QLatin1String("eval")) {
                if (_context->usesArgumentsObject == Context::ArgumentsObjectUnknown)
                    _context->usesArgumentsObject = Context::ArgumentsObjectUsed;
                _context->hasDirectEval = true;
            }
        }
    }
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _context->maxNumberOfArguments = qMax(_context->maxNumberOfArguments, argc);
    return true;
}

bool ScanFunctions::visit(NewMemberExpression *ast)
{
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _context->maxNumberOfArguments = qMax(_context->maxNumberOfArguments, argc);
    return true;
}

bool ScanFunctions::visit(ArrayLiteral *ast)
{
    int index = 0;
    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next)
            ++index;
        ++index;
    }
    if (ast->elision) {
        for (Elision *elision = ast->elision->next; elision; elision = elision->next)
            ++index;
    }
    _context->maxNumberOfArguments = qMax(_context->maxNumberOfArguments, index);
    return true;
}

bool ScanFunctions::visit(VariableDeclaration *ast)
{
    if (_context->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
    checkName(ast->name, ast->identifierToken);
    if (ast->name == QLatin1String("arguments"))
        _context->usesArgumentsObject = Context::ArgumentsObjectNotUsed;
    if (ast->scope == AST::VariableDeclaration::VariableScope::ReadOnlyBlockScope && !ast->expression) {
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Missing initializer in const declaration"));
        return false;
    }
    QString name = ast->name.toString();
    if (!_context->addLocalVar(ast->name.toString(), ast->expression ? Context::VariableDefinition : Context::VariableDeclaration, ast->scope)) {
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Identifier %1 has already been declared").arg(name));
        return false;
    }
    return true;
}

bool ScanFunctions::visit(IdentifierExpression *ast)
{
    checkName(ast->name, ast->identifierToken);
    if (_context->usesArgumentsObject == Context::ArgumentsObjectUnknown && ast->name == QLatin1String("arguments"))
        _context->usesArgumentsObject = Context::ArgumentsObjectUsed;
    _context->addUsedVariable(ast->name.toString());
    return true;
}

bool ScanFunctions::visit(ExpressionStatement *ast)
{
    if (FunctionExpression* expr = AST::cast<AST::FunctionExpression*>(ast->expression)) {
        if (!_allowFuncDecls)
            _cg->throwSyntaxError(expr->functionToken, QStringLiteral("conditional function or closure declaration"));

        enterFunction(expr, /*enterName*/ true);
        Node::accept(expr->formals, this);
        Node::accept(expr->body, this);
        leaveEnvironment();
        return false;
    } else {
        SourceLocation firstToken = ast->firstSourceLocation();
        if (_sourceCode.midRef(firstToken.offset, firstToken.length) == QLatin1String("function")) {
            _cg->throwSyntaxError(firstToken, QStringLiteral("unexpected token"));
        }
    }
    return true;
}

bool ScanFunctions::visit(FunctionExpression *ast)
{
    enterFunction(ast, /*enterName*/ false);
    return true;
}

void ScanFunctions::enterFunction(FunctionExpression *ast, bool enterName)
{
    if (_context->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Function name may not be eval or arguments in strict mode"));
    enterFunction(ast, ast->name.toString(), ast->formals, ast->body, enterName ? ast : nullptr);
}

void ScanFunctions::endVisit(FunctionExpression *)
{
    leaveEnvironment();
}

bool ScanFunctions::visit(ObjectLiteral *ast)
{
    int argc = 0;
    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        QString key = it->assignment->name->asString();
        if (QV4::String::toArrayIndex(key) != UINT_MAX)
            ++argc;
        ++argc;
        if (AST::cast<AST::PropertyGetterSetter *>(it->assignment))
            ++argc;
    }
    _context->maxNumberOfArguments = qMax(_context->maxNumberOfArguments, argc);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
    Node::accept(ast->properties, this);
    return false;
}

bool ScanFunctions::visit(PropertyGetterSetter *ast)
{
    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
    enterFunction(ast, QString(), ast->formals, ast->functionBody, /*FunctionExpression*/nullptr);
    return true;
}

void ScanFunctions::endVisit(PropertyGetterSetter *)
{
    leaveEnvironment();
}

bool ScanFunctions::visit(FunctionDeclaration *ast)
{
    enterFunction(ast, /*enterName*/ true);
    return true;
}

void ScanFunctions::endVisit(FunctionDeclaration *)
{
    leaveEnvironment();
}

bool ScanFunctions::visit(TryStatement *)
{
    // ### should limit to catch(), as try{} finally{} should be ok without
    _context->hasTry = true;
    return true;
}

bool ScanFunctions::visit(WithStatement *ast)
{
    if (_context->isStrict) {
        _cg->throwSyntaxError(ast->withToken, QStringLiteral("'with' statement is not allowed in strict mode"));
        return false;
    }

    _context->hasWith = true;
    return true;
}

bool ScanFunctions::visit(DoWhileStatement *ast) {
    {
        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_context->isStrict);
        Node::accept(ast->statement, this);
    }
    Node::accept(ast->expression, this);
    return false;
}

bool ScanFunctions::visit(ForStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_context->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool ScanFunctions::visit(LocalForStatement *ast) {
    Node::accept(ast->declarations, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_context->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool ScanFunctions::visit(ForEachStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_context->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool ScanFunctions::visit(LocalForEachStatement *ast) {
    Node::accept(ast->declaration, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_context->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool ScanFunctions::visit(ThisExpression *)
{
    _context->usesThis = true;
    return false;
}

bool ScanFunctions::visit(Block *ast) {
    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, _context->isStrict ? false : _allowFuncDecls);
    Node::accept(ast->statements, this);
    return false;
}

void ScanFunctions::enterFunction(Node *ast, const QString &name, FormalParameterList *formals, FunctionBody *body, FunctionExpression *expr)
{
    Context *outerContext = _context;
    enterEnvironment(ast, FunctionCode);

    if (outerContext) {
        outerContext->hasNestedFunctions = true;
        // The identifier of a function expression cannot be referenced from the enclosing environment.
        if (expr) {
            if (!outerContext->addLocalVar(name, Context::FunctionDefinition, AST::VariableDeclaration::FunctionScope, expr)) {
                _cg->throwSyntaxError(ast->firstSourceLocation(), QStringLiteral("Identifier %1 has already been declared").arg(name));
                return;
            }
        }
        if (name == QLatin1String("arguments"))
            outerContext->usesArgumentsObject = Context::ArgumentsObjectNotUsed;
    }

    if (formalsContainName(formals, QStringLiteral("arguments")))
        _context->usesArgumentsObject = Context::ArgumentsObjectNotUsed;


    if (!name.isEmpty() && !formalsContainName(formals, name))
        _context->addLocalVar(name, Context::ThisFunctionName, QQmlJS::AST::VariableDeclaration::FunctionScope);
    _context->formals = formals;

    if (body && !_context->isStrict)
        checkDirectivePrologue(body->elements);

    for (FormalParameterList *it = formals; it; it = it->next) {
        QString arg = it->name.toString();
        int duplicateIndex = _context->arguments.indexOf(arg);
        if (duplicateIndex != -1) {
            if (_context->isStrict) {
                _cg->throwSyntaxError(it->identifierToken, QStringLiteral("Duplicate parameter name '%1' is not allowed in strict mode").arg(arg));
                return;
            } else {
                // change the name of the earlier argument to enforce the specified lookup semantics
                QString modified = arg;
                while (_context->arguments.contains(modified))
                    modified += QString(0xfffe);
                _context->arguments[duplicateIndex] = modified;
            }
        }
        if (_context->isStrict) {
            if (arg == QLatin1String("eval") || arg == QLatin1String("arguments")) {
                _cg->throwSyntaxError(it->identifierToken, QStringLiteral("'%1' cannot be used as parameter name in strict mode").arg(arg));
                return;
            }
        }
        _context->arguments += arg;
    }
}

void ScanFunctions::calcEscapingVariables()
{
    Module *m = _cg->_module;

    for (Context *inner : qAsConst(m->contextMap)) {
        for (const QString &var : qAsConst(inner->usedVariables)) {
            Context *c = inner;
            while (c) {
                Context::MemberMap::const_iterator it = c->members.find(var);
                if (it != c->members.end()) {
                    if (c != inner)
                        it->canEscape = true;
                    break;
                }
                if (c->findArgument(var) != -1) {
                    if (c != inner)
                        c->argumentsCanEscape = true;
                    break;
                }
                c = c->parent;
            }
        }
        Context *c = inner->parent;
        while (c) {
            c->hasDirectEval |= inner->hasDirectEval;
            c = c->parent;
        }
    }

    static const bool showEscapingVars = qEnvironmentVariableIsSet("QV4_SHOW_ESCAPING_VARS");
    if (showEscapingVars) {
        qDebug() << "==== escaping variables ====";
        for (Context *c : qAsConst(m->contextMap)) {
            qDebug() << "Context" << c->name << ":";
            qDebug() << "    Arguments escape" << c->argumentsCanEscape;
            for (auto it = c->members.constBegin(); it != c->members.constEnd(); ++it) {
                qDebug() << "    " << it.key() << it.value().canEscape;
            }
        }
    }
}
