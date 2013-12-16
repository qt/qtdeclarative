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

#include "qv4codegen_p.h"
#include "qv4util_p.h"
#include "qv4debugging_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include <private/qqmljsast_p.h>
#include <qv4runtime_p.h>
#include <qv4context_p.h>
#include <cmath>
#include <iostream>

#ifdef CONST
#undef CONST
#endif

using namespace QQmlJS;
using namespace AST;

Codegen::ScanFunctions::ScanFunctions(Codegen *cg, const QString &sourceCode, CompilationMode defaultProgramMode)
    : _cg(cg)
    , _sourceCode(sourceCode)
    , _env(0)
    , _allowFuncDecls(true)
    , defaultProgramMode(defaultProgramMode)
{
}

void Codegen::ScanFunctions::operator()(Node *node)
{
    if (node)
        node->accept(this);
}

void Codegen::ScanFunctions::enterEnvironment(Node *node, CompilationMode compilationMode)
{
    Environment *e = _cg->newEnvironment(node, _env, compilationMode);
    if (!e->isStrict)
        e->isStrict = _cg->_strictMode;
    _envStack.append(e);
    _env = e;
}

void Codegen::ScanFunctions::leaveEnvironment()
{
    _envStack.pop();
    _env = _envStack.isEmpty() ? 0 : _envStack.top();
}

void Codegen::ScanFunctions::checkDirectivePrologue(SourceElements *ast)
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
                    if (str == QStringLiteral("use strict")) {
                        _env->isStrict = true;
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

void Codegen::ScanFunctions::checkName(const QStringRef &name, const SourceLocation &loc)
{
    if (_env->isStrict) {
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
void Codegen::ScanFunctions::checkForArguments(AST::FormalParameterList *parameters)
{
    while (parameters) {
        if (parameters->name == QStringLiteral("arguments"))
            _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
        parameters = parameters->next;
    }
}

bool Codegen::ScanFunctions::visit(Program *ast)
{
    enterEnvironment(ast, defaultProgramMode);
    checkDirectivePrologue(ast->elements);
    return true;
}

void Codegen::ScanFunctions::endVisit(Program *)
{
    leaveEnvironment();
}

bool Codegen::ScanFunctions::visit(CallExpression *ast)
{
    if (! _env->hasDirectEval) {
        if (IdentifierExpression *id = cast<IdentifierExpression *>(ast->base)) {
            if (id->name == QStringLiteral("eval")) {
                if (_env->usesArgumentsObject == Environment::ArgumentsObjectUnknown)
                    _env->usesArgumentsObject = Environment::ArgumentsObjectUsed;
                _env->hasDirectEval = true;
            }
        }
    }
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);
    return true;
}

bool Codegen::ScanFunctions::visit(NewMemberExpression *ast)
{
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);
    return true;
}

bool Codegen::ScanFunctions::visit(ArrayLiteral *ast)
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
    _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, index);
    return true;
}

bool Codegen::ScanFunctions::visit(VariableDeclaration *ast)
{
    if (_env->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
    checkName(ast->name, ast->identifierToken);
    if (ast->name == QLatin1String("arguments"))
        _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
    _env->enter(ast->name.toString(), ast->expression ? Environment::VariableDefinition : Environment::VariableDeclaration);
    return true;
}

bool Codegen::ScanFunctions::visit(IdentifierExpression *ast)
{
    checkName(ast->name, ast->identifierToken);
    if (_env->usesArgumentsObject == Environment::ArgumentsObjectUnknown && ast->name == QLatin1String("arguments"))
        _env->usesArgumentsObject = Environment::ArgumentsObjectUsed;
    return true;
}

bool Codegen::ScanFunctions::visit(ExpressionStatement *ast)
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
        if (_sourceCode.midRef(firstToken.offset, firstToken.length) == QStringLiteral("function")) {
            _cg->throwSyntaxError(firstToken, QStringLiteral("unexpected token"));
        }
    }
    return true;
}

bool Codegen::ScanFunctions::visit(FunctionExpression *ast)
{
    enterFunction(ast, /*enterName*/ false);
    return true;
}

void Codegen::ScanFunctions::enterFunction(FunctionExpression *ast, bool enterName, bool isExpression)
{
    if (_env->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Function name may not be eval or arguments in strict mode"));
    enterFunction(ast, ast->name.toString(), ast->formals, ast->body, enterName ? ast : 0, isExpression);
}

void Codegen::ScanFunctions::endVisit(FunctionExpression *)
{
    leaveEnvironment();
}

bool Codegen::ScanFunctions::visit(ObjectLiteral *ast)
{
    int argc = 0;
    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        ++argc;
        if (AST::cast<AST::PropertyGetterSetter *>(it->assignment))
            ++argc;
    }
    _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
    Node::accept(ast->properties, this);
    return false;
}

bool Codegen::ScanFunctions::visit(PropertyGetterSetter *ast)
{
    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
    enterFunction(ast, QString(), ast->formals, ast->functionBody, /*FunctionExpression*/0, /*isExpression*/false);
    return true;
}

void Codegen::ScanFunctions::endVisit(PropertyGetterSetter *)
{
    leaveEnvironment();
}

bool Codegen::ScanFunctions::visit(FunctionDeclaration *ast)
{
    enterFunction(ast, /*enterName*/ true, /*isExpression */false);
    return true;
}

void Codegen::ScanFunctions::endVisit(FunctionDeclaration *)
{
    leaveEnvironment();
}

bool Codegen::ScanFunctions::visit(WithStatement *ast)
{
    if (_env->isStrict) {
        _cg->throwSyntaxError(ast->withToken, QStringLiteral("'with' statement is not allowed in strict mode"));
        return false;
    }

    return true;
}

bool Codegen::ScanFunctions::visit(DoWhileStatement *ast) {
    {
        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
        Node::accept(ast->statement, this);
    }
    Node::accept(ast->expression, this);
    return false;
}

bool Codegen::ScanFunctions::visit(ForStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(LocalForStatement *ast) {
    Node::accept(ast->declarations, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(ForEachStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(LocalForEachStatement *ast) {
    Node::accept(ast->declaration, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(ThisExpression *)
{
    _env->usesThis = true;
    return false;
}

bool Codegen::ScanFunctions::visit(Block *ast) {
    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, _env->isStrict ? false : _allowFuncDecls);
    Node::accept(ast->statements, this);
    return false;
}

void Codegen::ScanFunctions::enterFunction(Node *ast, const QString &name, FormalParameterList *formals, FunctionBody *body, FunctionExpression *expr, bool isExpression)
{
    bool wasStrict = false;
    if (_env) {
        _env->hasNestedFunctions = true;
        // The identifier of a function expression cannot be referenced from the enclosing environment.
        if (expr)
            _env->enter(name, Environment::FunctionDefinition, expr);
        if (name == QLatin1String("arguments"))
            _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
        wasStrict = _env->isStrict;
    }

    enterEnvironment(ast, FunctionCode);
    checkForArguments(formals);

    _env->isNamedFunctionExpression = isExpression && !name.isEmpty();
    _env->formals = formals;

    if (body)
        checkDirectivePrologue(body->elements);

    if (wasStrict || _env->isStrict) {
        QStringList args;
        for (FormalParameterList *it = formals; it; it = it->next) {
            QString arg = it->name.toString();
            if (args.contains(arg)) {
                _cg->throwSyntaxError(it->identifierToken, QStringLiteral("Duplicate parameter name '%1' is not allowed in strict mode").arg(arg));
                return;
            }
            if (arg == QLatin1String("eval") || arg == QLatin1String("arguments")) {
                _cg->throwSyntaxError(it->identifierToken, QStringLiteral("'%1' cannot be used as parameter name in strict mode").arg(arg));
                return;
            }
            args += arg;
        }
    }
}


Codegen::Codegen(bool strict)
    : _module(0)
    , _function(0)
    , _block(0)
    , _exitBlock(0)
    , _returnAddress(0)
    , _env(0)
    , _loop(0)
    , _labelledStatement(0)
    , _scopeAndFinally(0)
    , _strictMode(strict)
    , _fileNameIsUrl(false)
    , hasError(false)
{
}

void Codegen::generateFromProgram(const QString &fileName,
                                  const QString &sourceCode,
                                  Program *node,
                                  V4IR::Module *module,
                                  CompilationMode mode,
                                  const QStringList &inheritedLocals)
{
    Q_ASSERT(node);

    _module = module;
    _env = 0;

    _module->setFileName(fileName);

    ScanFunctions scan(this, sourceCode, mode);
    scan(node);

    defineFunction(QStringLiteral("%entry"), node, 0, node->elements, inheritedLocals);
    qDeleteAll(_envMap);
    _envMap.clear();
}

void Codegen::generateFromFunctionExpression(const QString &fileName,
                                             const QString &sourceCode,
                                             AST::FunctionExpression *ast,
                                             V4IR::Module *module)
{
    _module = module;
    _module->setFileName(fileName);
    _env = 0;

    ScanFunctions scan(this, sourceCode, GlobalCode);
    // fake a global environment
    scan.enterEnvironment(0, FunctionCode);
    scan(ast);
    scan.leaveEnvironment();

    defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);

    qDeleteAll(_envMap);
    _envMap.clear();
}


void Codegen::enterEnvironment(Node *node)
{
    _env = _envMap.value(node);
    Q_ASSERT(_env);
}

void Codegen::leaveEnvironment()
{
    Q_ASSERT(_env);
    _env = _env->parent;
}

void Codegen::enterLoop(Statement *node, V4IR::BasicBlock *startBlock, V4IR::BasicBlock *breakBlock, V4IR::BasicBlock *continueBlock)
{
    if (startBlock)
        startBlock->markAsGroupStart();
    _loop = new Loop(node, startBlock, breakBlock, continueBlock, _loop);
    _loop->labelledStatement = _labelledStatement; // consume the enclosing labelled statement
    _loop->scopeAndFinally = _scopeAndFinally;
    _labelledStatement = 0;
}

void Codegen::leaveLoop()
{
    Loop *current = _loop;
    _loop = _loop->parent;
    delete current;
}

V4IR::Expr *Codegen::member(V4IR::Expr *base, const QString *name)
{
    if (hasError)
        return 0;

    if (base->asTemp() /*|| base->asName()*/)
        return _block->MEMBER(base->asTemp(), name);
    else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        return _block->MEMBER(_block->TEMP(t), name);
    }
}

V4IR::Expr *Codegen::subscript(V4IR::Expr *base, V4IR::Expr *index)
{
    if (hasError)
        return 0;

    if (! base->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        base = _block->TEMP(t);
    }

    if (! index->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), index);
        index = _block->TEMP(t);
    }

    Q_ASSERT(base->asTemp() && index->asTemp());
    return _block->SUBSCRIPT(base->asTemp(), index->asTemp());
}

V4IR::Expr *Codegen::argument(V4IR::Expr *expr)
{
    if (expr && ! expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

// keeps references alive, converts other expressions to temps
V4IR::Expr *Codegen::reference(V4IR::Expr *expr)
{
    if (hasError)
        return 0;

    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember() && !expr->asSubscript()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

V4IR::Expr *Codegen::unop(V4IR::AluOp op, V4IR::Expr *expr)
{
    if (hasError)
        return 0;

    Q_ASSERT(op != V4IR::OpIncrement);
    Q_ASSERT(op != V4IR::OpDecrement);

    if (V4IR::Const *c = expr->asConst()) {
        if (c->type == V4IR::NumberType) {
            switch (op) {
            case V4IR::OpNot:
                return _block->CONST(V4IR::BoolType, !c->value);
            case V4IR::OpUMinus:
                return _block->CONST(V4IR::NumberType, -c->value);
            case V4IR::OpUPlus:
                return expr;
            case V4IR::OpCompl:
                return _block->CONST(V4IR::NumberType, ~QV4::Primitive::toInt32(c->value));
            case V4IR::OpIncrement:
                return _block->CONST(V4IR::NumberType, c->value + 1);
            case V4IR::OpDecrement:
                return _block->CONST(V4IR::NumberType, c->value - 1);
            default:
                break;
            }
        }
    }
    if (! expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    Q_ASSERT(expr->asTemp());
    return _block->UNOP(op, expr->asTemp());
}

V4IR::Expr *Codegen::binop(V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right)
{
    if (hasError)
        return 0;

    if (V4IR::Const *c1 = left->asConst()) {
        if (V4IR::Const *c2 = right->asConst()) {
            if (c1->type == V4IR::NumberType && c2->type == V4IR::NumberType) {
                switch (op) {
                case V4IR::OpAdd: return _block->CONST(V4IR::NumberType, c1->value + c2->value);
                case V4IR::OpAnd: return _block->CONST(V4IR::BoolType, c1->value ? c2->value : 0);
                case V4IR::OpBitAnd: return _block->CONST(V4IR::NumberType, int(c1->value) & int(c2->value));
                case V4IR::OpBitOr: return _block->CONST(V4IR::NumberType, int(c1->value) | int(c2->value));
                case V4IR::OpBitXor: return _block->CONST(V4IR::NumberType, int(c1->value) ^ int(c2->value));
                case V4IR::OpDiv: return _block->CONST(V4IR::NumberType, c1->value / c2->value);
                case V4IR::OpEqual: return _block->CONST(V4IR::BoolType, c1->value == c2->value);
                case V4IR::OpNotEqual: return _block->CONST(V4IR::BoolType, c1->value != c2->value);
                case V4IR::OpStrictEqual: return _block->CONST(V4IR::BoolType, c1->value == c2->value);
                case V4IR::OpStrictNotEqual: return _block->CONST(V4IR::BoolType, c1->value != c2->value);
                case V4IR::OpGe: return _block->CONST(V4IR::BoolType, c1->value >= c2->value);
                case V4IR::OpGt: return _block->CONST(V4IR::BoolType, c1->value > c2->value);
                case V4IR::OpLe: return _block->CONST(V4IR::BoolType, c1->value <= c2->value);
                case V4IR::OpLt: return _block->CONST(V4IR::BoolType, c1->value < c2->value);
                case V4IR::OpLShift: return _block->CONST(V4IR::NumberType, QV4::Primitive::toInt32(c1->value) << (QV4::Primitive::toUInt32(c2->value) & 0x1f));
                case V4IR::OpMod: return _block->CONST(V4IR::NumberType, std::fmod(c1->value, c2->value));
                case V4IR::OpMul: return _block->CONST(V4IR::NumberType, c1->value * c2->value);
                case V4IR::OpOr: return _block->CONST(V4IR::NumberType, c1->value ? c1->value : c2->value);
                case V4IR::OpRShift: return _block->CONST(V4IR::NumberType, QV4::Primitive::toInt32(c1->value) >> (QV4::Primitive::toUInt32(c2->value) & 0x1f));
                case V4IR::OpSub: return _block->CONST(V4IR::NumberType, c1->value - c2->value);
                case V4IR::OpURShift: return _block->CONST(V4IR::NumberType,QV4::Primitive::toUInt32(c1->value) >> (QV4::Primitive::toUInt32(c2->value) & 0x1f));

                case V4IR::OpInstanceof:
                case V4IR::OpIn:
                    break;

                case V4IR::OpIfTrue: // unary ops
                case V4IR::OpNot:
                case V4IR::OpUMinus:
                case V4IR::OpUPlus:
                case V4IR::OpCompl:
                case V4IR::OpIncrement:
                case V4IR::OpDecrement:
                case V4IR::OpInvalid:
                    break;
                }
            }
        }
    } else if (op == V4IR::OpAdd) {
        if (V4IR::String *s1 = left->asString()) {
            if (V4IR::String *s2 = right->asString()) {
                return _block->STRING(_function->newString(*s1->value + *s2->value));
            }
        }
    }

    if (!left->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), left);
        left = _block->TEMP(t);
    }

    if (!right->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), right);
        right = _block->TEMP(t);
    }

    Q_ASSERT(left->asTemp());
    Q_ASSERT(right->asTemp());

    return _block->BINOP(op, left, right);
}

V4IR::Expr *Codegen::call(V4IR::Expr *base, V4IR::ExprList *args)
{
    if (hasError)
        return 0;
    base = reference(base);
    return _block->CALL(base, args);
}

void Codegen::move(V4IR::Expr *target, V4IR::Expr *source, V4IR::AluOp op)
{
    if (hasError)
        return;

    Q_ASSERT(target->isLValue());

    if (op != V4IR::OpInvalid) {
        move(target, binop(op, target, source));
        return;
    }

    if (!source->asTemp() && !source->asConst() && !target->asTemp()) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }
    if (source->asConst() && !target->asTemp()) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }

    _block->MOVE(target, source);
}

void Codegen::cjump(V4IR::Expr *cond, V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (hasError)
        return;

    if (! (cond->asTemp() || cond->asBinop())) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), cond);
        cond = _block->TEMP(t);
    }
    _block->CJUMP(cond, iftrue, iffalse);
}

void Codegen::accept(Node *node)
{
    if (hasError)
        return;

    if (node)
        node->accept(this);
}

void Codegen::statement(Statement *ast)
{
    _block->nextLocation = ast->firstSourceLocation();
    accept(ast);
}

void Codegen::statement(ExpressionNode *ast)
{
    if (! ast) {
        return;
    } else {
        Result r(nx);
        qSwap(_expr, r);
        accept(ast);
        if (hasError)
            return;
        qSwap(_expr, r);
        if (r.format == ex) {
            if (r->asCall()) {
                _block->EXP(*r); // the nest nx representation for calls is EXP(CALL(c..))
            } else if (r->asTemp()) {
                // there is nothing to do
            } else {
                unsigned t = _block->newTemp();
                move(_block->TEMP(t), *r);
            }
        }
    }
}

void Codegen::condition(ExpressionNode *ast, V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (ast) {
        Result r(iftrue, iffalse);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            cjump(*r, r.iftrue, r.iffalse);
        }
    }
}

Codegen::Result Codegen::expression(ExpressionNode *ast)
{
    Result r;
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
    }
    return r;
}

QString Codegen::propertyName(PropertyName *ast)
{
    QString p;
    if (ast) {
        qSwap(_property, p);
        accept(ast);
        qSwap(_property, p);
    }
    return p;
}

Codegen::Result Codegen::sourceElement(SourceElement *ast)
{
    Result r(nx);
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
    }
    return r;
}

Codegen::UiMember Codegen::uiObjectMember(UiObjectMember *ast)
{
    UiMember m;
    if (ast) {
        qSwap(_uiMember, m);
        accept(ast);
        qSwap(_uiMember, m);
    }
    return m;
}

void Codegen::functionBody(FunctionBody *ast)
{
    if (ast)
        sourceElements(ast->elements);
}

void Codegen::program(Program *ast)
{
    if (ast) {
        sourceElements(ast->elements);
    }
}

void Codegen::sourceElements(SourceElements *ast)
{
    for (SourceElements *it = ast; it; it = it->next) {
        sourceElement(it->element);
        if (hasError)
            return;
    }
}

void Codegen::variableDeclaration(VariableDeclaration *ast)
{
    V4IR::Expr *initializer = 0;
    if (!ast->expression)
        return;
    Result expr = expression(ast->expression);
    if (hasError)
        return;

    Q_ASSERT(expr.code);
    initializer = *expr;

    int initialized = _block->newTemp();
    move(_block->TEMP(initialized), initializer);
    move(identifier(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn), _block->TEMP(initialized));
}

void Codegen::variableDeclarationList(VariableDeclarationList *ast)
{
    for (VariableDeclarationList *it = ast; it; it = it->next) {
        variableDeclaration(it->declaration);
    }
}


bool Codegen::visit(ArgumentList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseBlock *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClause *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClauses *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Catch *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(DefaultClause *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(ElementList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Elision *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Finally *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(FormalParameterList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(FunctionBody *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Program *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyAssignmentList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyNameAndValue *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyGetterSetter *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(SourceElements *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(StatementList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiArrayMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiImport *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiHeaderItemList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiPragma *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectInitializer *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiParameterList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiProgram *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiQualifiedId *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiQualifiedPragmaId *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclaration *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclarationList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Expression *ast)
{
    if (hasError)
        return false;

    statement(ast->left);
    accept(ast->right);
    return false;
}

bool Codegen::visit(ArrayLiteral *ast)
{
    if (hasError)
        return false;

    V4IR::ExprList *args = 0;
    V4IR::ExprList *current = 0;
    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next) {
            V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
            if (!current) {
                args = arg;
            } else {
                current->next = arg;
            }
            current = arg;
            current->expr = _block->CONST(V4IR::MissingType, 0);
        }
        Result expr = expression(it->expression);

        V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;

        V4IR::Expr *exp = *expr;
        if (exp->asTemp() || exp->asConst()) {
            current->expr = exp;
        } else {
            unsigned value = _block->newTemp();
            move(_block->TEMP(value), exp);
            current->expr = _block->TEMP(value);
        }
    }
    for (Elision *elision = ast->elision; elision; elision = elision->next) {
        V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;
        current->expr = _block->CONST(V4IR::MissingType, 0);
    }

    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->CALL(_block->NAME(V4IR::Name::builtin_define_array, 0, 0), args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(ArrayMemberExpression *ast)
{
    if (hasError)
        return false;

    Result base = expression(ast->base);
    Result index = expression(ast->expression);
    _expr.code = subscript(*base, *index);
    return false;
}

static V4IR::AluOp baseOp(int op)
{
    switch ((QSOperator::Op) op) {
    case QSOperator::InplaceAnd: return V4IR::OpBitAnd;
    case QSOperator::InplaceSub: return V4IR::OpSub;
    case QSOperator::InplaceDiv: return V4IR::OpDiv;
    case QSOperator::InplaceAdd: return V4IR::OpAdd;
    case QSOperator::InplaceLeftShift: return V4IR::OpLShift;
    case QSOperator::InplaceMod: return V4IR::OpMod;
    case QSOperator::InplaceMul: return V4IR::OpMul;
    case QSOperator::InplaceOr: return V4IR::OpBitOr;
    case QSOperator::InplaceRightShift: return V4IR::OpRShift;
    case QSOperator::InplaceURightShift: return V4IR::OpURShift;
    case QSOperator::InplaceXor: return V4IR::OpBitXor;
    default: return V4IR::OpInvalid;
    }
}

bool Codegen::visit(BinaryExpression *ast)
{
    if (hasError)
        return false;

    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            condition(ast->left, iftrue, _expr.iffalse);
            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

            const unsigned r = _block->newTemp();

            move(_block->TEMP(r), *expression(ast->left));
            cjump(_block->TEMP(r), iftrue, endif);
            _block = iftrue;
            move(_block->TEMP(r), *expression(ast->right));
            _block->JUMP(endif);

            _expr.code = _block->TEMP(r);
            _block = endif;
        }
        return false;
    } else if (ast->op == QSOperator::Or) {
        if (_expr.accept(cx)) {
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            condition(ast->left, _expr.iftrue, iffalse);
            _block = iffalse;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

            const unsigned r = _block->newTemp();
            move(_block->TEMP(r), *expression(ast->left));
            cjump(_block->TEMP(r), endif, iffalse);
            _block = iffalse;
            move(_block->TEMP(r), *expression(ast->right));
            _block->JUMP(endif);

            _block = endif;
            _expr.code = _block->TEMP(r);
        }
        return false;
    }

    V4IR::Expr* left = *expression(ast->left);

    switch (ast->op) {
    case QSOperator::Or:
    case QSOperator::And:
        break;

    case QSOperator::Assign: {
        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
            return false;
        V4IR::Expr* right = *expression(ast->right);
        if (!left->isLValue()) {
            throwReferenceError(ast->operatorToken, QStringLiteral("left-hand side of assignment operator is not an lvalue"));
            return false;
        }

        if (_expr.accept(nx)) {
            move(left, right);
        } else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), right);
            move(left, _block->TEMP(t));
            _expr.code = _block->TEMP(t);
        }
        break;
    }

    case QSOperator::InplaceAnd:
    case QSOperator::InplaceSub:
    case QSOperator::InplaceDiv:
    case QSOperator::InplaceAdd:
    case QSOperator::InplaceLeftShift:
    case QSOperator::InplaceMod:
    case QSOperator::InplaceMul:
    case QSOperator::InplaceOr:
    case QSOperator::InplaceRightShift:
    case QSOperator::InplaceURightShift:
    case QSOperator::InplaceXor: {
        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
            return false;
        V4IR::Expr* right = *expression(ast->right);
        if (!left->isLValue()) {
            throwSyntaxError(ast->operatorToken, QStringLiteral("left-hand side of inplace operator is not an lvalue"));
            return false;
        }

        if (_expr.accept(nx)) {
            move(left, right, baseOp(ast->op));
        } else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), right);
            move(left, _block->TEMP(t), baseOp(ast->op));
            _expr.code = left;
        }
        break;
    }

    case QSOperator::In:
    case QSOperator::InstanceOf:
    case QSOperator::Equal:
    case QSOperator::NotEqual:
    case QSOperator::Ge:
    case QSOperator::Gt:
    case QSOperator::Le:
    case QSOperator::Lt:
    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual: {
        if (!left->asTemp() && !left->asConst()) {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), left);
            left = _block->TEMP(t);
        }

        V4IR::Expr* right = *expression(ast->right);

        if (_expr.accept(cx)) {
            cjump(binop(V4IR::binaryOperator(ast->op), left, right), _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::Expr *e = binop(V4IR::binaryOperator(ast->op), left, right);
            if (e->asConst() || e->asString())
                _expr.code = e;
            else {
                const unsigned t = _block->newTemp();
                move(_block->TEMP(t), e);
                _expr.code = _block->TEMP(t);
            }
        }
        break;
    }

    case QSOperator::Add:
    case QSOperator::BitAnd:
    case QSOperator::BitOr:
    case QSOperator::BitXor:
    case QSOperator::Div:
    case QSOperator::LShift:
    case QSOperator::Mod:
    case QSOperator::Mul:
    case QSOperator::RShift:
    case QSOperator::Sub:
    case QSOperator::URShift: {
        if (!left->asTemp() && !left->asConst()) {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), left);
            left = _block->TEMP(t);
        }

        V4IR::Expr* right = *expression(ast->right);

        V4IR::Expr *e = binop(V4IR::binaryOperator(ast->op), left, right);
        if (e->asConst() || e->asString())
            _expr.code = e;
        else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), e);
            _expr.code = _block->TEMP(t);
        }
        break;
    }

    } // switch

    return false;
}

bool Codegen::visit(CallExpression *ast)
{
    if (hasError)
        return false;

    Result base = expression(ast->base);
    V4IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        V4IR::Expr *actual = argument(*arg);
        *args_it = _function->New<V4IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    _expr.code = call(*base, args);
    return false;
}

bool Codegen::visit(ConditionalExpression *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    const unsigned t = _block->newTemp();

    condition(ast->expression, iftrue, iffalse);

    _block = iftrue;
    move(_block->TEMP(t), *expression(ast->ok));
    _block->JUMP(endif);

    _block = iffalse;
    move(_block->TEMP(t), *expression(ast->ko));
    _block->JUMP(endif);

    _block = endif;

    _expr.code = _block->TEMP(t);

    return false;
}

bool Codegen::visit(DeleteExpression *ast)
{
    if (hasError)
        return false;

    V4IR::Expr* expr = *expression(ast->expression);
    // Temporaries cannot be deleted
    V4IR::Temp *t = expr->asTemp();
    if (t && t->index < static_cast<unsigned>(_env->members.size())) {
        // Trying to delete a function argument might throw.
        if (_function->isStrict) {
            throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
            return false;
        }
        _expr.code = _block->CONST(V4IR::BoolType, 0);
        return false;
    }
    if (_function->isStrict && expr->asName()) {
        throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
        return false;
    }

    // [[11.4.1]] Return true if it's not a reference
    if (expr->asConst() || expr->asString()) {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }

    // Return values from calls are also not a reference, but we have to
    // perform the call to allow for side effects.
    if (expr->asCall()) {
        _block->EXP(expr);
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }
    if (expr->asTemp() && expr->asTemp()->index >=  static_cast<unsigned>(_env->members.size())) {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }

    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(reference(expr));
    _expr.code = call(_block->NAME(V4IR::Name::builtin_delete, ast->deleteToken.startLine, ast->deleteToken.startColumn), args);
    return false;
}

bool Codegen::visit(FalseLiteral *)
{
    if (hasError)
        return false;

    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 0);
    }
    return false;
}

bool Codegen::visit(FieldMemberExpression *ast)
{
    if (hasError)
        return false;

    Result base = expression(ast->base);
    _expr.code = member(*base, _function->newString(ast->name.toString()));
    return false;
}

bool Codegen::visit(FunctionExpression *ast)
{
    if (hasError)
        return false;

    int function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    _expr.code = _block->CLOSURE(function);
    return false;
}

V4IR::Expr *Codegen::identifier(const QString &name, int line, int col)
{
    if (hasError)
        return 0;

    uint scope = 0;
    Environment *e = _env;
    V4IR::Function *f = _function;

    while (f && e->parent) {
        if (f->insideWithOrCatch || (f->isNamedExpression && f->name == name))
            return _block->NAME(name, line, col);

        int index = e->findMember(name);
        Q_ASSERT (index < e->members.size());
        if (index != -1) {
            V4IR::Temp *t = _block->LOCAL(index, scope);
            if (name == QStringLiteral("arguments") || name == QStringLiteral("eval"))
                t->isArgumentsOrEval = true;
            return t;
        }
        const int argIdx = f->indexOfArgument(&name);
        if (argIdx != -1)
            return _block->ARG(argIdx, scope);

        if (!f->isStrict && f->hasDirectEval)
            return _block->NAME(name, line, col);

        ++scope;
        e = e->parent;
        f = f->outer;
    }

    // This hook allows implementing QML lookup semantics
    if (V4IR::Expr *fallback = fallbackNameLookup(name, line, col))
        return fallback;

    if (!e->parent && (!f || !f->insideWithOrCatch) && _env->compilationMode != EvalCode && e->compilationMode != QmlBinding)
        return _block->GLOBALNAME(name, line, col);

    // global context or with. Lookup by name
    return _block->NAME(name, line, col);

}

V4IR::Expr *Codegen::fallbackNameLookup(const QString &name, int line, int col)
{
    Q_UNUSED(name)
    Q_UNUSED(line)
    Q_UNUSED(col)
    return 0;
}

bool Codegen::visit(IdentifierExpression *ast)
{
    if (hasError)
        return false;

    _expr.code = identifier(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn);
    return false;
}

bool Codegen::visit(NestedExpression *ast)
{
    if (hasError)
        return false;

    accept(ast->expression);
    return false;
}

bool Codegen::visit(NewExpression *ast)
{
    if (hasError)
        return false;

    Result base = expression(ast->expression);
    V4IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    _expr.code = _block->NEW(expr, 0);
    return false;
}

bool Codegen::visit(NewMemberExpression *ast)
{
    if (hasError)
        return false;

    Result base = expression(ast->base);
    V4IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }

    V4IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        V4IR::Expr *actual = argument(*arg);
        *args_it = _function->New<V4IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->NEW(expr, args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(NotExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    const unsigned r = _block->newTemp();
    move(_block->TEMP(r), unop(V4IR::OpNot, *expr));
    _expr.code = _block->TEMP(r);
    return false;
}

bool Codegen::visit(NullExpression *)
{
    if (hasError)
        return false;

    if (_expr.accept(cx)) _block->JUMP(_expr.iffalse);
    else _expr.code = _block->CONST(V4IR::NullType, 0);

    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    if (hasError)
        return false;

    if (_expr.accept(cx)) {
        if (ast->value) _block->JUMP(_expr.iftrue);
        else _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::NumberType, ast->value);
    }
    return false;
}

struct ObjectPropertyValue {
    ObjectPropertyValue()
        : value(0)
        , getter(-1)
        , setter(-1)
    {}

    V4IR::Expr *value;
    int getter; // index in _module->functions or -1 if not set
    int setter;

    bool hasGetter() const { return getter >= 0; }
    bool hasSetter() const { return setter >= 0; }
};

bool Codegen::visit(ObjectLiteral *ast)
{
    if (hasError)
        return false;

    QMap<QString, ObjectPropertyValue> valueMap;

    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        if (PropertyNameAndValue *nv = AST::cast<AST::PropertyNameAndValue *>(it->assignment)) {
            QString name = propertyName(nv->name);
            Result value = expression(nv->value);
            ObjectPropertyValue &v = valueMap[name];
            if (v.hasGetter() || v.hasSetter() || (_function->isStrict && v.value)) {
                throwSyntaxError(nv->lastSourceLocation(),
                                 QStringLiteral("Illegal duplicate key '%1' in object literal").arg(name));
                return false;
            }

            valueMap[name].value = *value;
        } else if (PropertyGetterSetter *gs = AST::cast<AST::PropertyGetterSetter *>(it->assignment)) {
            QString name = propertyName(gs->name);
            const int function = defineFunction(name, gs, gs->formals, gs->functionBody ? gs->functionBody->elements : 0);
            ObjectPropertyValue &v = valueMap[name];
            if (v.value ||
                (gs->type == PropertyGetterSetter::Getter && v.hasGetter()) ||
                (gs->type == PropertyGetterSetter::Setter && v.hasSetter())) {
                throwSyntaxError(gs->lastSourceLocation(),
                                 QStringLiteral("Illegal duplicate key '%1' in object literal").arg(name));
                return false;
            }
            if (gs->type == PropertyGetterSetter::Getter)
                v.getter = function;
            else
                v.setter = function;
        } else {
            Q_UNREACHABLE();
        }
    }

    V4IR::ExprList *args = 0;

    if (!valueMap.isEmpty()) {
        V4IR::ExprList *current;
        for (QMap<QString, ObjectPropertyValue>::iterator it = valueMap.begin(); it != valueMap.end(); ) {
            if (QV4::String::toArrayIndex(it.key()) != UINT_MAX) {
                ++it;
                continue;
            }

            if (!args) {
                args = _function->New<V4IR::ExprList>();
                current = args;
            } else {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
            }

            current->expr = _block->NAME(it.key(), 0, 0);

            if (it->value) {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->CONST(V4IR::BoolType, true);

                unsigned value = _block->newTemp();
                move(_block->TEMP(value), it->value);

                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(value);
            } else {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->CONST(V4IR::BoolType, false);

                unsigned getter = _block->newTemp();
                unsigned setter = _block->newTemp();
                move(_block->TEMP(getter), it->hasGetter() ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->hasSetter() ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));

                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(getter);
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(setter);
            }

            it = valueMap.erase(it);
        }
    }

    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->CALL(_block->NAME(V4IR::Name::builtin_define_object_literal,
         ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), args));

    // What's left are array entries
    if (!valueMap.isEmpty()) {
        unsigned value = 0;
        unsigned getter = 0;
        unsigned setter = 0;
        for (QMap<QString, ObjectPropertyValue>::const_iterator it = valueMap.constBegin(); it != valueMap.constEnd(); ++it) {
            V4IR::ExprList *args = _function->New<V4IR::ExprList>();
            V4IR::ExprList *current = args;
            current->expr = _block->TEMP(t);
            current->next = _function->New<V4IR::ExprList>();
            current = current->next;
            current->expr = _block->NAME(it.key(), 0, 0);
            current->next = _function->New<V4IR::ExprList>();
            current = current->next;

            if (it->value) {
                if (!value)
                    value = _block->newTemp();
                move(_block->TEMP(value), it->value);
                // __qmljs_builtin_define_property(Value object, String *name, Value val, ExecutionContext *ctx)
                current->expr = _block->TEMP(value);
                _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_define_property, 0, 0), args));
            } else {
                if (!getter) {
                    getter = _block->newTemp();
                    setter = _block->newTemp();
                }
                move(_block->TEMP(getter), it->hasGetter() ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->hasSetter() ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));


                // __qmljs_builtin_define_getter_setter(Value object, String *name, Value getter, Value setter, ExecutionContext *ctx);
                current->expr = _block->TEMP(getter);
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(setter);
                _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_define_getter_setter, 0, 0), args));
            }
        }
    }

    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(PostDecrementExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->base);
    if (!expr->isLValue()) {
        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
        return false;
    }
    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken))
        return false;

    const unsigned oldValue = _block->newTemp();
    move(_block->TEMP(oldValue), unop(V4IR::OpUPlus, *expr));

    const unsigned  newValue = _block->newTemp();
    move(_block->TEMP(newValue), binop(V4IR::OpSub, _block->TEMP(oldValue), _block->CONST(V4IR::NumberType, 1)));
    move(*expr, _block->TEMP(newValue));

    if (!_expr.accept(nx))
        _expr.code = _block->TEMP(oldValue);

    return false;
}

bool Codegen::visit(PostIncrementExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->base);
    if (!expr->isLValue()) {
        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
        return false;
    }
    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken))
        return false;

    const unsigned oldValue = _block->newTemp();
    move(_block->TEMP(oldValue), unop(V4IR::OpUPlus, *expr));

    const unsigned  newValue = _block->newTemp();
    move(_block->TEMP(newValue), binop(V4IR::OpAdd, _block->TEMP(oldValue), _block->CONST(V4IR::NumberType, 1)));
    move(*expr, _block->TEMP(newValue));

    if (!_expr.accept(nx))
        _expr.code = _block->TEMP(oldValue);

    return false;
}

bool Codegen::visit(PreDecrementExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    if (!expr->isLValue()) {
        throwReferenceError(ast->expression->lastSourceLocation(), QStringLiteral("Prefix ++ operator applied to value that is not a reference."));
        return false;
    }

    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken))
        return false;
    V4IR::Expr *op = binop(V4IR::OpSub, *expr, _block->CONST(V4IR::NumberType, 1));
    if (_expr.accept(nx)) {
        move(*expr, op);
    } else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), op);
        move(*expr, _block->TEMP(t));
        _expr.code = _block->TEMP(t);
    }
    return false;
}

bool Codegen::visit(PreIncrementExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    if (!expr->isLValue()) {
        throwReferenceError(ast->expression->lastSourceLocation(), QStringLiteral("Prefix ++ operator applied to value that is not a reference."));
        return false;
    }

    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken))
        return false;
    V4IR::Expr *op = binop(V4IR::OpAdd, unop(V4IR::OpUPlus, *expr), _block->CONST(V4IR::NumberType, 1));
    if (_expr.accept(nx)) {
        move(*expr, op);
    } else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), op);
        move(*expr, _block->TEMP(t));
        _expr.code = _block->TEMP(t);
    }
    return false;
}

bool Codegen::visit(RegExpLiteral *ast)
{
    if (hasError)
        return false;

    _expr.code = _block->REGEXP(_function->newString(ast->pattern.toString()), ast->flags);
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    if (hasError)
        return false;

    _expr.code = _block->STRING(_function->newString(ast->value.toString()));
    return false;
}

bool Codegen::visit(ThisExpression *ast)
{
    if (hasError)
        return false;

    _expr.code = _block->NAME(QStringLiteral("this"), ast->thisToken.startLine, ast->thisToken.startColumn);
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpCompl, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(TrueLiteral *)
{
    if (hasError)
        return false;

    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iftrue);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
    }
    return false;
}

bool Codegen::visit(TypeOfExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(reference(*expr));
    _expr.code = call(_block->NAME(V4IR::Name::builtin_typeof, ast->typeofToken.startLine, ast->typeofToken.startColumn), args);
    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUMinus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    if (hasError)
        return false;

    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUPlus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    if (hasError)
        return false;

    statement(ast->expression);
    _expr.code = _block->CONST(V4IR::UndefinedType, 0);
    return false;
}

bool Codegen::visit(FunctionDeclaration * ast)
{
    if (hasError)
        return false;

    if (_env->compilationMode == QmlBinding)
        move(_block->TEMP(_returnAddress), _block->NAME(ast->name.toString(), 0, 0));
    _expr.accept(nx);
    return false;
}

int Codegen::defineFunction(const QString &name, AST::Node *ast,
                            AST::FormalParameterList *formals,
                            AST::SourceElements *body,
                            const QStringList &inheritedLocals)
{
    Loop *loop = 0;
    qSwap(_loop, loop);
    QStack<V4IR::BasicBlock *> exceptionHandlers;
    qSwap(_exceptionHandlers, exceptionHandlers);

    ScopeAndFinally *scopeAndFinally = 0;

    enterEnvironment(ast);
    V4IR::Function *function = _module->newFunction(name, _function);
    int functionIndex = _module->functions.count() - 1;

    V4IR::BasicBlock *entryBlock = function->newBasicBlock(groupStartBlock(), 0);
    V4IR::BasicBlock *exitBlock = function->newBasicBlock(groupStartBlock(), 0, V4IR::Function::DontInsertBlock);
    function->hasDirectEval = _env->hasDirectEval || _env->compilationMode == EvalCode;
    function->usesArgumentsObject = _env->parent && (_env->usesArgumentsObject == Environment::ArgumentsObjectUsed);
    function->usesThis = _env->usesThis;
    function->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, (int)QV4::Global::ReservedArgumentCount);
    function->isStrict = _env->isStrict;
    function->isNamedExpression = _env->isNamedFunctionExpression;

    AST::SourceLocation loc = ast->firstSourceLocation();
    function->line = loc.startLine;
    function->column = loc.startColumn;

    if (function->usesArgumentsObject)
        _env->enter(QStringLiteral("arguments"), Environment::VariableDeclaration);

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (_env->compilationMode == FunctionCode || _env->compilationMode == QmlBinding) {
        unsigned t = 0;
        for (Environment::MemberMap::iterator it = _env->members.begin(); it != _env->members.end(); ++it) {
            const QString &local = it.key();
            function->LOCAL(local);
            (*it).index = t;
            entryBlock->MOVE(entryBlock->LOCAL(t, 0), entryBlock->CONST(V4IR::UndefinedType, 0));
            ++t;
        }
    } else {
        if (!_env->isStrict) {
            foreach (const QString &inheritedLocal, inheritedLocals) {
                function->LOCAL(inheritedLocal);
                unsigned tempIndex = entryBlock->newTemp();
                Environment::Member member = { Environment::UndefinedMember,
                                               static_cast<int>(tempIndex), 0 };
                _env->members.insert(inheritedLocal, member);
            }
        }

        V4IR::ExprList *args = 0;
        for (Environment::MemberMap::const_iterator it = _env->members.constBegin(); it != _env->members.constEnd(); ++it) {
            const QString &local = it.key();
            V4IR::ExprList *next = function->New<V4IR::ExprList>();
            next->expr = entryBlock->NAME(local, 0, 0);
            next->next = args;
            args = next;
        }
        if (args) {
            V4IR::ExprList *next = function->New<V4IR::ExprList>();
            next->expr = entryBlock->CONST(V4IR::BoolType, false); // ### Investigate removal of bool deletable
            next->next = args;
            args = next;

            entryBlock->EXP(entryBlock->CALL(entryBlock->NAME(V4IR::Name::builtin_declare_vars, 0, 0), args));
        }
    }

    unsigned returnAddress = entryBlock->newTemp();

    entryBlock->MOVE(entryBlock->TEMP(returnAddress), entryBlock->CONST(V4IR::UndefinedType, 0));
    exitBlock->RET(exitBlock->TEMP(returnAddress));

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);

    for (FormalParameterList *it = formals; it; it = it->next) {
        _function->RECEIVE(it->name.toString());
    }

    foreach (const Environment::Member &member, _env->members) {
        if (member.function) {
            const int function = defineFunction(member.function->name.toString(), member.function, member.function->formals,
                                                member.function->body ? member.function->body->elements : 0);
            if (! _env->parent) {
                move(_block->NAME(member.function->name.toString(), member.function->identifierToken.startLine, member.function->identifierToken.startColumn),
                     _block->CLOSURE(function));
            } else {
                Q_ASSERT(member.index >= 0);
                move(_block->LOCAL(member.index, 0), _block->CLOSURE(function));
            }
        }
    }
    if (_function->usesArgumentsObject) {
        move(identifier(QStringLiteral("arguments"), ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn),
             _block->CALL(_block->NAME(V4IR::Name::builtin_setup_argument_object,
                     ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), 0));
    }
    if (_function->usesThis && !_function->isStrict) {
        // make sure we convert this to an object
        _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_convert_this_to_object,
                ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), 0));
    }

    beginFunctionBodyHook();

    sourceElements(body);

    _function->insertBasicBlock(_exitBlock);

    _block->JUMP(_exitBlock);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);
    qSwap(_exceptionHandlers, exceptionHandlers);
    qSwap(_loop, loop);

    leaveEnvironment();

    return functionIndex;
}

bool Codegen::visit(IdentifierPropertyName *ast)
{
    if (hasError)
        return false;

    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(NumericLiteralPropertyName *ast)
{
    if (hasError)
        return false;

    _property = QString::number(ast->id, 'g', 16);
    return false;
}

bool Codegen::visit(StringLiteralPropertyName *ast)
{
    if (hasError)
        return false;

    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(FunctionSourceElement *ast)
{
    if (hasError)
        return false;

    statement(ast->declaration);
    return false;
}

bool Codegen::visit(StatementSourceElement *ast)
{
    if (hasError)
        return false;

    statement(ast->statement);
    return false;
}

bool Codegen::visit(Block *ast)
{
    if (hasError)
        return false;

    for (StatementList *it = ast->statements; it; it = it->next) {
        statement(it->statement);
    }
    return false;
}

bool Codegen::visit(BreakStatement *ast)
{
    if (hasError)
        return false;

    if (!_loop) {
        throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Break outside of loop"));
        return false;
    }
    Loop *loop = 0;
    if (ast->label.isEmpty())
        loop = _loop;
    else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label)
                break;
        }
        if (!loop) {
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Undefined label '%1'").arg(ast->label.toString()));
            return false;
        }
    }
    unwindException(loop->scopeAndFinally);
    _block->JUMP(loop->breakBlock);
    return false;
}

bool Codegen::visit(ContinueStatement *ast)
{
    if (hasError)
        return false;

    Loop *loop = 0;
    if (ast->label.isEmpty()) {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->continueBlock)
                break;
        }
    } else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label) {
                if (!loop->continueBlock)
                    loop = 0;
                break;
            }
        }
        if (!loop) {
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Undefined label '%1'").arg(ast->label.toString()));
            return false;
        }
    }
    if (!loop) {
        throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("continue outside of loop"));
        return false;
    }
    unwindException(loop->scopeAndFinally);
    _block->JUMP(loop->continueBlock);
    return false;
}

bool Codegen::visit(DebuggerStatement *)
{
    Q_UNIMPLEMENTED();
    return false;
}

bool Codegen::visit(DoWhileStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *loopbody = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *loopcond = _function->newBasicBlock(loopbody, exceptionHandler());
    V4IR::BasicBlock *loopend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    enterLoop(ast, loopbody, loopend, loopcond);

    _block->JUMP(loopbody);

    _block = loopbody;
    statement(ast->statement);
    _block->JUMP(loopcond);

    _block = loopcond;
    condition(ast->expression, loopbody, loopend);

    _block = loopend;

    leaveLoop();

    return false;
}

bool Codegen::visit(EmptyStatement *)
{
    if (hasError)
        return true;

    return false;
}

bool Codegen::visit(ExpressionStatement *ast)
{
    if (hasError)
        return true;

    if (_env->compilationMode == EvalCode || _env->compilationMode == QmlBinding) {
        Result e = expression(ast->expression);
        if (*e)
            move(_block->TEMP(_returnAddress), *e);
    } else {
        statement(ast->expression);
    }
    return false;
}

bool Codegen::visit(ForEachStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *foreachin = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock(foreachin, exceptionHandler());
    V4IR::BasicBlock *foreachend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    enterLoop(ast, foreachin, foreachend, foreachin);

    int objectToIterateOn = _block->newTemp();
    move(_block->TEMP(objectToIterateOn), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(objectToIterateOn));

    int iterator = _block->newTemp();
    move(_block->TEMP(iterator), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_iterator_object, 0, 0), args));

    _block->JUMP(foreachin);

    _block = foreachbody;
    int temp = _block->newTemp();
    move(*expression(ast->initialiser), _block->TEMP(temp));
    statement(ast->statement);
    _block->JUMP(foreachin);

    _block = foreachin;

    args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(temp), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_next_property_name, 0, 0), args));
    int null = _block->newTemp();
    move(_block->TEMP(null), _block->CONST(V4IR::NullType, 0));
    cjump(_block->BINOP(V4IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend);
    _block = foreachend;

    leaveLoop();
    return false;
}

bool Codegen::visit(ForStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *forcond = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *forbody = _function->newBasicBlock(forcond, exceptionHandler());
    V4IR::BasicBlock *forstep = _function->newBasicBlock(forcond, exceptionHandler());
    V4IR::BasicBlock *forend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    statement(ast->initialiser);
    _block->JUMP(forcond);

    enterLoop(ast, forcond, forend, forstep);

    _block = forcond;
    if (ast->condition)
        condition(ast->condition, forbody, forend);
    else
        _block->JUMP(forbody);

    _block = forbody;
    statement(ast->statement);
    _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    _block->JUMP(forcond);

    _block = forend;

    leaveLoop();

    return false;
}

bool Codegen::visit(IfStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *iffalse = ast->ko ? _function->newBasicBlock(groupStartBlock(), exceptionHandler()) : 0;
    V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    condition(ast->expression, iftrue, ast->ko ? iffalse : endif);

    _block = iftrue;
    statement(ast->ok);
    _block->JUMP(endif);

    if (ast->ko) {
        _block = iffalse;
        statement(ast->ko);
        _block->JUMP(endif);
    }

    _block = endif;

    return false;
}

bool Codegen::visit(LabelledStatement *ast)
{
    if (hasError)
        return true;

    // check that no outer loop contains the label
    Loop *l = _loop;
    while (l) {
        if (l->labelledStatement && l->labelledStatement->label == ast->label) {
            QString error = QString(QStringLiteral("Label '%1' has already been declared")).arg(ast->label.toString());
            throwSyntaxError(ast->firstSourceLocation(), error);
            return false;
        }
        l = l->parent;
    }
    _labelledStatement = ast;

    if (AST::cast<AST::SwitchStatement *>(ast->statement) ||
            AST::cast<AST::WhileStatement *>(ast->statement) ||
            AST::cast<AST::DoWhileStatement *>(ast->statement) ||
            AST::cast<AST::ForStatement *>(ast->statement) ||
            AST::cast<AST::ForEachStatement *>(ast->statement) ||
            AST::cast<AST::LocalForStatement *>(ast->statement) ||
            AST::cast<AST::LocalForEachStatement *>(ast->statement)) {
        statement(ast->statement); // labelledStatement will be associated with the ast->statement's loop.
    } else {
        V4IR::BasicBlock *breakBlock = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
        enterLoop(ast->statement, 0, breakBlock, /*continueBlock*/ 0);
        statement(ast->statement);
        _block->JUMP(breakBlock);
        _block = breakBlock;
        leaveLoop();
    }

    return false;
}

bool Codegen::visit(LocalForEachStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *foreachin = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock(foreachin, exceptionHandler());
    V4IR::BasicBlock *foreachend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    enterLoop(ast, foreachin, foreachend, foreachin);

    variableDeclaration(ast->declaration);

    int iterator = _block->newTemp();
    move(_block->TEMP(iterator), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(iterator), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_iterator_object, 0, 0), args));

    _block->JUMP(foreachin);

    _block = foreachbody;
    int temp = _block->newTemp();
    move(identifier(ast->declaration->name.toString()), _block->TEMP(temp));
    statement(ast->statement);
    _block->JUMP(foreachin);

    _block = foreachin;

    args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(temp), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_next_property_name, 0, 0), args));
    int null = _block->newTemp();
    move(_block->TEMP(null), _block->CONST(V4IR::NullType, 0));
    cjump(_block->BINOP(V4IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend);
    _block = foreachend;

    leaveLoop();
    return false;
}

bool Codegen::visit(LocalForStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *forcond = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *forbody = _function->newBasicBlock(forcond, exceptionHandler());
    V4IR::BasicBlock *forstep = _function->newBasicBlock(forcond, exceptionHandler());
    V4IR::BasicBlock *forend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    variableDeclarationList(ast->declarations);
    _block->JUMP(forcond);

    enterLoop(ast, forcond, forend, forstep);

    _block = forcond;
    if (ast->condition)
        condition(ast->condition, forbody, forend);
    else
        _block->JUMP(forbody);

    _block = forbody;
    statement(ast->statement);
    _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    _block->JUMP(forcond);

    _block = forend;

    leaveLoop();

    return false;
}

bool Codegen::visit(ReturnStatement *ast)
{
    if (hasError)
        return true;

    if (_env->compilationMode != FunctionCode && _env->compilationMode != QmlBinding) {
        throwSyntaxError(ast->returnToken, QStringLiteral("Return statement outside of function"));
        return false;
    }
    if (ast->expression) {
        Result expr = expression(ast->expression);
        move(_block->TEMP(_returnAddress), *expr);
    }
    unwindException(0);

    _block->JUMP(_exitBlock);
    return false;
}

bool Codegen::visit(SwitchStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *switchend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    if (ast->block) {
        Result lhs = expression(ast->expression);
        V4IR::BasicBlock *switchcond = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
        _block->JUMP(switchcond);
        V4IR::BasicBlock *previousBlock = 0;

        QHash<Node *, V4IR::BasicBlock *> blockMap;

        enterLoop(ast, 0, switchend, 0);

        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            blockMap[clause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        if (ast->block->defaultClause) {
            _block = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            blockMap[ast->block->defaultClause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = ast->block->defaultClause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            blockMap[clause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        leaveLoop();

        _block->JUMP(switchend);

        _block = switchcond;
        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Result rhs = expression(clause->expression);
            V4IR::BasicBlock *iftrue = blockMap[clause];
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            cjump(binop(V4IR::OpStrictEqual, *lhs, *rhs), iftrue, iffalse);
            _block = iffalse;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Result rhs = expression(clause->expression);
            V4IR::BasicBlock *iftrue = blockMap[clause];
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
            cjump(binop(V4IR::OpStrictEqual, *lhs, *rhs), iftrue, iffalse);
            _block = iffalse;
        }

        if (ast->block->defaultClause) {
            _block->JUMP(blockMap[ast->block->defaultClause]);
        }
    }

    _block->JUMP(switchend);

    _block = switchend;
    return false;
}

bool Codegen::visit(ThrowStatement *ast)
{
    if (hasError)
        return true;

    Result expr = expression(ast->expression);
    move(_block->TEMP(_returnAddress), *expr);
    V4IR::ExprList *throwArgs = _function->New<V4IR::ExprList>();
    throwArgs->expr = _block->TEMP(_returnAddress);
    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    return false;
}

bool Codegen::visit(TryStatement *ast)
{
    if (hasError)
        return true;

    _function->hasTry = true;

    if (_function->isStrict && ast->catchExpression &&
            (ast->catchExpression->name == QLatin1String("eval") || ast->catchExpression->name == QLatin1String("arguments"))) {
        throwSyntaxError(ast->catchExpression->identifierToken, QStringLiteral("Catch variable name may not be eval or arguments in strict mode"));
        return false;
    }

    V4IR::BasicBlock *surroundingExceptionHandler = exceptionHandler();

    // We always need a finally body to clean up the exception handler
    // exceptions thrown in finally get catched by the surrounding catch block
    V4IR::BasicBlock *finallyBody = 0;
    V4IR::BasicBlock *catchBody = 0;
    V4IR::BasicBlock *catchExceptionHandler = 0;
    V4IR::BasicBlock *end = _function->newBasicBlock(groupStartBlock(), surroundingExceptionHandler, V4IR::Function::DontInsertBlock);

    if (ast->finallyExpression)
        finallyBody = _function->newBasicBlock(groupStartBlock(), surroundingExceptionHandler, V4IR::Function::DontInsertBlock);

    if (ast->catchExpression) {
        // exception handler for the catch body
        catchExceptionHandler = _function->newBasicBlock(groupStartBlock(), 0, V4IR::Function::DontInsertBlock);
        pushExceptionHandler(catchExceptionHandler);
        catchBody =  _function->newBasicBlock(groupStartBlock(), catchExceptionHandler, V4IR::Function::DontInsertBlock);
        popExceptionHandler();
        pushExceptionHandler(catchBody);
    } else {
        Q_ASSERT(finallyBody);
        pushExceptionHandler(finallyBody);
    }

    V4IR::BasicBlock *tryBody = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    _block->JUMP(tryBody);

    ScopeAndFinally tcf(_scopeAndFinally, ast->finallyExpression);
    _scopeAndFinally = &tcf;

    _block = tryBody;
    statement(ast->statement);
    _block->JUMP(finallyBody ? finallyBody : end);

    popExceptionHandler();

    if (ast->catchExpression) {
        pushExceptionHandler(catchExceptionHandler);
        _function->insertBasicBlock(catchBody);
        _block = catchBody;

        ++_function->insideWithOrCatch;
        V4IR::ExprList *catchArgs = _function->New<V4IR::ExprList>();
        catchArgs->init(_block->STRING(_function->newString(ast->catchExpression->name.toString())));
        _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_push_catch_scope, 0, 0), catchArgs));
        {
            ScopeAndFinally scope(_scopeAndFinally, ScopeAndFinally::CatchScope);
            _scopeAndFinally = &scope;
            statement(ast->catchExpression->statement);
            _scopeAndFinally = scope.parent;
        }
        _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0), 0));
        --_function->insideWithOrCatch;
        _block->JUMP(finallyBody ? finallyBody : end);
        popExceptionHandler();

        _function->insertBasicBlock(catchExceptionHandler);
        catchExceptionHandler->EXP(catchExceptionHandler->CALL(catchExceptionHandler->NAME(V4IR::Name::builtin_pop_scope, 0, 0), 0));
        if (finallyBody || surroundingExceptionHandler)
            catchExceptionHandler->JUMP(finallyBody ? finallyBody : surroundingExceptionHandler);
        else
            catchExceptionHandler->EXP(catchExceptionHandler->CALL(catchExceptionHandler->NAME(V4IR::Name::builtin_rethrow, 0, 0), 0));
    }

    _scopeAndFinally = tcf.parent;

    if (finallyBody) {
        _function->insertBasicBlock(finallyBody);
        _block = finallyBody;

        int hasException = _block->newTemp();
        move(_block->TEMP(hasException), _block->CALL(_block->NAME(V4IR::Name::builtin_unwind_exception, /*line*/0, /*column*/0), 0));

        if (ast->finallyExpression && ast->finallyExpression->statement)
            statement(ast->finallyExpression->statement);

        V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
        arg->expr = _block->TEMP(hasException);
        _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), arg));
        _block->JUMP(end);
    }

    _function->insertBasicBlock(end);
    _block = end;

    return false;
}

void Codegen::unwindException(Codegen::ScopeAndFinally *outest)
{
    int savedDepthForWidthOrCatch = _function->insideWithOrCatch;
    ScopeAndFinally *scopeAndFinally = _scopeAndFinally;
    qSwap(_scopeAndFinally, scopeAndFinally);
    while (_scopeAndFinally != outest) {
        switch (_scopeAndFinally->type) {
        case ScopeAndFinally::WithScope:
            // fall through
        case ScopeAndFinally::CatchScope:
            _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0)));
            _scopeAndFinally = _scopeAndFinally->parent;
            --_function->insideWithOrCatch;
            break;
        case ScopeAndFinally::TryScope: {
            ScopeAndFinally *tc = _scopeAndFinally;
            _scopeAndFinally = tc->parent;
            if (tc->finally && tc->finally->statement)
                statement(tc->finally->statement);
            break;
        }
        }
    }
    qSwap(_scopeAndFinally, scopeAndFinally);
    _function->insideWithOrCatch = savedDepthForWidthOrCatch;
}

bool Codegen::visit(VariableStatement *ast)
{
    if (hasError)
        return true;

    variableDeclarationList(ast->declarations);
    return false;
}

bool Codegen::visit(WhileStatement *ast)
{
    if (hasError)
        return true;

    V4IR::BasicBlock *whilecond = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    V4IR::BasicBlock *whilebody = _function->newBasicBlock(whilecond, exceptionHandler());
    V4IR::BasicBlock *whileend = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    enterLoop(ast, whilecond, whileend, whilecond);

    _block->JUMP(whilecond);
    _block = whilecond;
    condition(ast->expression, whilebody, whileend);

    _block = whilebody;
    statement(ast->statement);
    _block->JUMP(whilecond);

    _block = whileend;
    leaveLoop();

    return false;
}

bool Codegen::visit(WithStatement *ast)
{
    if (hasError)
        return true;

    _function->hasWith = true;

    // need an exception handler for with to cleanup the with scope
    V4IR::BasicBlock *withExceptionHandler = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    withExceptionHandler->EXP(withExceptionHandler->CALL(withExceptionHandler->NAME(V4IR::Name::builtin_pop_scope, 0, 0), 0));
    if (!exceptionHandler())
        withExceptionHandler->EXP(withExceptionHandler->CALL(withExceptionHandler->NAME(V4IR::Name::builtin_rethrow, 0, 0), 0));
    else
        withExceptionHandler->JUMP(exceptionHandler());

    pushExceptionHandler(withExceptionHandler);

    V4IR::BasicBlock *withBlock = _function->newBasicBlock(groupStartBlock(), exceptionHandler());

    _block->JUMP(withBlock);
    _block = withBlock;
    int withObject = _block->newTemp();
    _block->MOVE(_block->TEMP(withObject), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(withObject));
    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_push_with_scope, 0, 0), args));

    ++_function->insideWithOrCatch;
    {
        ScopeAndFinally scope(_scopeAndFinally);
        _scopeAndFinally = &scope;
        statement(ast->statement);
        _scopeAndFinally = scope.parent;
    }
    --_function->insideWithOrCatch;
    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0), 0));
    popExceptionHandler();

    V4IR::BasicBlock *next = _function->newBasicBlock(groupStartBlock(), exceptionHandler());
    _block->JUMP(next);
    _block = next;

    return false;
}

bool Codegen::visit(UiArrayBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectDefinition *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiPublicMember *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiScriptBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiSourceElement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::throwSyntaxErrorOnEvalOrArgumentsInStrictMode(V4IR::Expr *expr, const SourceLocation& loc)
{
    if (!_env->isStrict)
        return false;
    if (V4IR::Name *n = expr->asName()) {
        if (*n->id != QLatin1String("eval") && *n->id != QLatin1String("arguments"))
            return false;
    } else if (V4IR::Temp *t = expr->asTemp()) {
        if (!t->isArgumentsOrEval)
            return false;
    } else {
        return false;
    }
    throwSyntaxError(loc, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
    return true;
}

void Codegen::throwSyntaxError(const SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;

    hasError = true;
    QQmlError error;
    error.setUrl(_fileNameIsUrl ? QUrl(_module->fileName) : QUrl::fromLocalFile(_module->fileName));
    error.setDescription(detail);
    error.setLine(loc.startLine);
    error.setColumn(loc.startColumn);
    _errors << error;
}

void Codegen::throwReferenceError(const SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;

    hasError = true;
    QQmlError error;
    error.setUrl(_fileNameIsUrl ? QUrl(_module->fileName) : QUrl::fromLocalFile(_module->fileName));
    error.setDescription(detail);
    error.setLine(loc.startLine);
    error.setColumn(loc.startColumn);
    _errors << error;
}

QList<QQmlError> Codegen::errors() const
{
    return _errors;
}

void RuntimeCodegen::throwSyntaxError(const AST::SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;
    hasError = true;
    context->throwSyntaxError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

void RuntimeCodegen::throwReferenceError(const AST::SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;
    hasError = true;
    context->throwReferenceError(detail, _module->fileName, loc.startLine, loc.startColumn);
}
