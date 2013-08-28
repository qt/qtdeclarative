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
#include <cassert>

#ifdef CONST
#undef CONST
#endif

#define QV4_NO_LIVENESS
#undef SHOW_SSA

using namespace QQmlJS;
using namespace AST;

Codegen::ScanFunctions::ScanFunctions(Codegen *cg, const QString &sourceCode)
    : _cg(cg)
    , _sourceCode(sourceCode)
    , _env(0)
    , _allowFuncDecls(true)
{
}

void Codegen::ScanFunctions::operator()(Node *node)
{
    if (node)
        node->accept(this);
}

void Codegen::ScanFunctions::enterEnvironment(Node *node)
{
    Environment *e = _cg->newEnvironment(node, _env);
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
            _cg->throwSyntaxError(loc, QCoreApplication::translate("qv4codegen", "Unexpected strict mode reserved word"));
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
    enterEnvironment(ast);
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
        _cg->throwSyntaxError(ast->identifierToken, QCoreApplication::translate("qv4codegen", "Variable name may not be eval or arguments in strict mode"));
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
            _cg->throwSyntaxError(expr->functionToken, QCoreApplication::translate("qv4codegen", "conditional function or closure declaration"));

        enterFunction(expr, /*enterName*/ true);
        Node::accept(expr->formals, this);
        Node::accept(expr->body, this);
        leaveEnvironment();
        return false;
    } else {
        SourceLocation firstToken = ast->firstSourceLocation();
        if (_sourceCode.midRef(firstToken.offset, firstToken.length) == QStringLiteral("function")) {
            _cg->throwSyntaxError(firstToken, QCoreApplication::translate("qv4codegen", "unexpected token"));
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
        _cg->throwSyntaxError(ast->identifierToken, QCoreApplication::translate("qv4codegen", "Function name may not be eval or arguments in strict mode"));
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
        _cg->throwSyntaxError(ast->withToken, QCoreApplication::translate("qv4codegen", "'with' statement is not allowed in strict mode"));
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

    enterEnvironment(ast);
    checkForArguments(formals);

    _env->isNamedFunctionExpression = isExpression && !name.isEmpty();
    _env->formals = formals;

    if (body)
        checkDirectivePrologue(body->elements);

    if (wasStrict || _env->isStrict) {
        QStringList args;
        for (FormalParameterList *it = formals; it; it = it->next) {
            QString arg = it->name.toString();
            if (args.contains(arg))
                _cg->throwSyntaxError(it->identifierToken, QCoreApplication::translate("qv4codegen", "Duplicate parameter name '%1' is not allowed in strict mode").arg(arg));
            if (arg == QLatin1String("eval") || arg == QLatin1String("arguments"))
                _cg->throwSyntaxError(it->identifierToken, QCoreApplication::translate("qv4codegen", "'%1' cannot be used as parameter name in strict mode").arg(arg));
            args += arg;
        }
    }
}


Codegen::Codegen(bool strict)
    : _module(0)
    , _function(0)
    , _block(0)
    , _exitBlock(0)
    , _throwBlock(0)
    , _returnAddress(0)
    , _mode(GlobalCode)
    , _env(0)
    , _loop(0)
    , _labelledStatement(0)
    , _scopeAndFinally(0)
    , _strictMode(strict)
{
}

V4IR::Function *Codegen::generateFromProgram(const QString &fileName,
                                  const QString &sourceCode,
                                  Program *node,
                                  V4IR::Module *module,
                                  Mode mode,
                                  const QStringList &inheritedLocals)
{
    assert(node);

    _module = module;
    _env = 0;

    _module->setFileName(fileName);

    ScanFunctions scan(this, sourceCode);
    scan(node);

    V4IR::Function *globalCode = defineFunction(QStringLiteral("%entry"), node, 0,
                                              node->elements, mode, inheritedLocals);
    qDeleteAll(_envMap);
    _envMap.clear();

    return globalCode;
}

V4IR::Function *Codegen::generateFromFunctionExpression(const QString &fileName,
                                  const QString &sourceCode,
                                  AST::FunctionExpression *ast,
                                  V4IR::Module *module)
{
    _module = module;
    _module->setFileName(fileName);
    _env = 0;

    ScanFunctions scan(this, sourceCode);
    // fake a global environment
    scan.enterEnvironment(0);
    scan(ast);
    scan.leaveEnvironment();

    V4IR::Function *function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);

    qDeleteAll(_envMap);
    _envMap.clear();

    return function;
}


void Codegen::enterEnvironment(Node *node)
{
    _env = _envMap.value(node);
    assert(_env);
}

void Codegen::leaveEnvironment()
{
    assert(_env);
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

    assert(base->asTemp() && index->asTemp());
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
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember() && !expr->asSubscript()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

V4IR::Expr *Codegen::unop(V4IR::AluOp op, V4IR::Expr *expr)
{
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
                return _block->CONST(V4IR::NumberType, ~QV4::Value::toInt32(c->value));
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
    assert(expr->asTemp());
    return _block->UNOP(op, expr->asTemp());
}

V4IR::Expr *Codegen::binop(V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right)
{
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
                case V4IR::OpLShift: return _block->CONST(V4IR::NumberType, QV4::Value::toInt32(c1->value) << (QV4::Value::toUInt32(c2->value) & 0x1f));
                case V4IR::OpMod: return _block->CONST(V4IR::NumberType, std::fmod(c1->value, c2->value));
                case V4IR::OpMul: return _block->CONST(V4IR::NumberType, c1->value * c2->value);
                case V4IR::OpOr: return _block->CONST(V4IR::NumberType, c1->value ? c1->value : c2->value);
                case V4IR::OpRShift: return _block->CONST(V4IR::NumberType, QV4::Value::toInt32(c1->value) >> (QV4::Value::toUInt32(c2->value) & 0x1f));
                case V4IR::OpSub: return _block->CONST(V4IR::NumberType, c1->value - c2->value);
                case V4IR::OpURShift: return _block->CONST(V4IR::NumberType,QV4::Value::toUInt32(c1->value) >> (QV4::Value::toUInt32(c2->value) & 0x1f));

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

    assert(left->asTemp());
    assert(right->asTemp());

    return _block->BINOP(op, left, right);
}

V4IR::Expr *Codegen::call(V4IR::Expr *base, V4IR::ExprList *args)
{
    base = reference(base);
    return _block->CALL(base, args);
}

void Codegen::move(V4IR::Expr *target, V4IR::Expr *source, V4IR::AluOp op)
{
    assert(target->isLValue());

    // TODO: verify the rest of the function for when op == OpInvalid
    if (op != V4IR::OpInvalid) {
        move(target, binop(op, target, source), V4IR::OpInvalid);
        return;
    }

    if (!source->asTemp() && !source->asConst() && (op != V4IR::OpInvalid || ! target->asTemp())) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }
    if (source->asConst() && (!target->asTemp() || op != V4IR::OpInvalid)) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }

    _block->MOVE(target, source, op);
}

void Codegen::cjump(V4IR::Expr *cond, V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (! (cond->asTemp() || cond->asBinop())) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), cond);
        cond = _block->TEMP(t);
    }
    _block->CJUMP(cond, iftrue, iffalse);
}

void Codegen::accept(Node *node)
{
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
    }
}

void Codegen::variableDeclaration(VariableDeclaration *ast)
{
    V4IR::Expr *initializer = 0;
    if (!ast->expression)
        return;
    Result expr = expression(ast->expression);
    assert(expr.code);
    initializer = *expr;

    if (! _env->parent || _function->insideWithOrCatch) {
        // it's global code.
        move(_block->NAME(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn), initializer);
    } else {
        const int index = _env->findMember(ast->name.toString());
        assert(index != -1);
        move(_block->LOCAL(index, 0), initializer);
    }
}

void Codegen::variableDeclarationList(VariableDeclarationList *ast)
{
    for (VariableDeclarationList *it = ast; it; it = it->next) {
        variableDeclaration(it->declaration);
    }
}


bool Codegen::visit(ArgumentList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseBlock *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClause *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClauses *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Catch *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(DefaultClause *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(ElementList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Elision *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Finally *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(FormalParameterList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(FunctionBody *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Program *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyAssignmentList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyNameAndValue *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyGetterSetter *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(SourceElements *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(StatementList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiArrayMemberList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiImport *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiImportList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectInitializer *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectMemberList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiParameterList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiProgram *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiQualifiedId *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclaration *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclarationList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Expression *ast)
{
    statement(ast->left);
    accept(ast->right);
    return false;
}

bool Codegen::visit(ArrayLiteral *ast)
{
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
    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock());
            condition(ast->left, iftrue, _expr.iffalse);
            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock());
            V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock());

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
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock());
            condition(ast->left, _expr.iftrue, iffalse);
            _block = iffalse;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock());
            V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock());

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
        throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation());
        V4IR::Expr* right = *expression(ast->right);
        if (! (left->asTemp() || left->asName() || left->asSubscript() || left->asMember()))
            throwReferenceError(ast->operatorToken, QCoreApplication::translate("qv4codegen", "left-hand side of assignment operator is not an lvalue"));

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
        throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation());
        V4IR::Expr* right = *expression(ast->right);
        if (!left->isLValue())
            throwSyntaxError(ast->operatorToken, QCoreApplication::translate("qv4codegen", "left-hand side of inplace operator is not an lvalue"));

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
    V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock());

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
    V4IR::Expr* expr = *expression(ast->expression);
    // Temporaries cannot be deleted
    V4IR::Temp *t = expr->asTemp();
    if (t && t->index < _env->members.size()) {
        // Trying to delete a function argument might throw.
        if (_function->isStrict)
            throwSyntaxError(ast->deleteToken, "Delete of an unqualified identifier in strict mode.");
        _expr.code = _block->CONST(V4IR::BoolType, 0);
        return false;
    }
    if (_function->isStrict && expr->asName())
        throwSyntaxError(ast->deleteToken, "Delete of an unqualified identifier in strict mode.");

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
    if (expr->asTemp() && expr->asTemp()->index >=  _env->members.size()) {
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
    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 0);
    }
    return false;
}

bool Codegen::visit(FieldMemberExpression *ast)
{
    Result base = expression(ast->base);
    _expr.code = member(*base, _function->newString(ast->name.toString()));
    return false;
}

bool Codegen::visit(FunctionExpression *ast)
{
    V4IR::Function *function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    _expr.code = _block->CLOSURE(function);
    return false;
}

V4IR::Expr *Codegen::identifier(const QString &name, int line, int col)
{
    uint scope = 0;
    Environment *e = _env;
    V4IR::Function *f = _function;

    while (f && e->parent) {
        if (f->insideWithOrCatch || (f->isNamedExpression && f->name == name))
            return _block->NAME(name, line, col);

        int index = e->findMember(name);
        assert (index < e->members.size());
        if (index != -1) {
            V4IR::Temp *t = _block->LOCAL(index, scope);
            if (name == "arguments" || name == "eval")
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

    if (!e->parent && (!f || !f->insideWithOrCatch) && _mode != EvalCode && _mode != QmlBinding)
        return _block->GLOBALNAME(name, line, col);

    // global context or with. Lookup by name
    return _block->NAME(name, line, col);

}

bool Codegen::visit(IdentifierExpression *ast)
{
    _expr.code = identifier(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn);
    return false;
}

bool Codegen::visit(NestedExpression *ast)
{
    accept(ast->expression);
    return false;
}

bool Codegen::visit(NewExpression *ast)
{
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
    Result expr = expression(ast->expression);
    const unsigned r = _block->newTemp();
    move(_block->TEMP(r), unop(V4IR::OpNot, *expr));
    _expr.code = _block->TEMP(r);
    return false;
}

bool Codegen::visit(NullExpression *)
{
    if (_expr.accept(cx)) _block->JUMP(_expr.iffalse);
    else _expr.code = _block->CONST(V4IR::NullType, 0);

    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    if (_expr.accept(cx)) {
        if (ast->value) _block->JUMP(_expr.iftrue);
        else _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::NumberType, ast->value);
    }
    return false;
}

struct ObjectPropertyValue {
    V4IR::Expr *value;
    V4IR::Function *getter;
    V4IR::Function *setter;
};

bool Codegen::visit(ObjectLiteral *ast)
{
    QMap<QString, ObjectPropertyValue> valueMap;

    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        if (PropertyNameAndValue *nv = AST::cast<AST::PropertyNameAndValue *>(it->assignment)) {
            QString name = propertyName(nv->name);
            Result value = expression(nv->value);
            ObjectPropertyValue &v = valueMap[name];
            if (v.getter || v.setter || (_function->isStrict && v.value))
                throwSyntaxError(nv->lastSourceLocation(),
                                 QCoreApplication::translate("qv4codegen", "Illegal duplicate key '%1' in object literal").arg(name));

            valueMap[name].value = *value;
        } else if (PropertyGetterSetter *gs = AST::cast<AST::PropertyGetterSetter *>(it->assignment)) {
            QString name = propertyName(gs->name);
            V4IR::Function *function = defineFunction(name, gs, gs->formals, gs->functionBody ? gs->functionBody->elements : 0);
            ObjectPropertyValue &v = valueMap[name];
            if (v.value ||
                (gs->type == PropertyGetterSetter::Getter && v.getter) ||
                (gs->type == PropertyGetterSetter::Setter && v.setter))
                throwSyntaxError(gs->lastSourceLocation(),
                                 QCoreApplication::translate("qv4codegen", "Illegal duplicate key '%1' in object literal").arg(name));
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
            if (QV4::String(0, it.key()).asArrayIndex() != UINT_MAX) {
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
                move(_block->TEMP(getter), it->getter ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->setter ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));

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
                move(_block->TEMP(getter), it->getter ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->setter ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));


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
    Result expr = expression(ast->base);
    if (!expr->isLValue())
        throwReferenceError(ast->base->lastSourceLocation(), "Invalid left-hand side expression in postfix operation");
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken);

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
    Result expr = expression(ast->base);
    if (!expr->isLValue())
        throwReferenceError(ast->base->lastSourceLocation(), "Invalid left-hand side expression in postfix operation");
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken);

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
    Result expr = expression(ast->expression);
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken);
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
    Result expr = expression(ast->expression);
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken);
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
    _expr.code = _block->REGEXP(_function->newString(ast->pattern.toString()), ast->flags);
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    _expr.code = _block->STRING(_function->newString(ast->value.toString()));
    return false;
}

bool Codegen::visit(ThisExpression *ast)
{
    _expr.code = _block->NAME(QStringLiteral("this"), ast->thisToken.startLine, ast->thisToken.startColumn);
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpCompl, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(TrueLiteral *)
{
    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iftrue);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
    }
    return false;
}

bool Codegen::visit(TypeOfExpression *ast)
{
    Result expr = expression(ast->expression);
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(reference(*expr));
    _expr.code = call(_block->NAME(V4IR::Name::builtin_typeof, ast->typeofToken.startLine, ast->typeofToken.startColumn), args);
    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUMinus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUPlus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    statement(ast->expression);
    _expr.code = _block->CONST(V4IR::UndefinedType, 0);
    return false;
}

bool Codegen::visit(FunctionDeclaration * ast)
{
    if (_mode == QmlBinding)
        move(_block->TEMP(_returnAddress), _block->NAME(ast->name.toString(), 0, 0));
    _expr.accept(nx);
    return false;
}

V4IR::Function *Codegen::defineFunction(const QString &name, AST::Node *ast,
                                      AST::FormalParameterList *formals,
                                      AST::SourceElements *body, Mode mode,
                                      const QStringList &inheritedLocals)
{
    qSwap(_mode, mode); // enter function code.
    Loop *loop = 0;
    qSwap(_loop, loop);

    ScopeAndFinally *scopeAndFinally = 0;

    enterEnvironment(ast);
    V4IR::Function *function = _module->newFunction(name, _function);

    V4IR::BasicBlock *entryBlock = function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *exitBlock = function->newBasicBlock(groupStartBlock(), V4IR::Function::DontInsertBlock);
    V4IR::BasicBlock *throwBlock = function->newBasicBlock(groupStartBlock());
    function->hasDirectEval = _env->hasDirectEval;
    function->usesArgumentsObject = _env->parent && (_env->usesArgumentsObject == Environment::ArgumentsObjectUsed);
    function->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, (int)QV4::Global::ReservedArgumentCount);
    function->isStrict = _env->isStrict;
    function->isNamedExpression = _env->isNamedFunctionExpression;

    if (function->usesArgumentsObject)
        _env->enter("arguments", Environment::VariableDeclaration);

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (_mode == FunctionCode) {
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
            next->expr = entryBlock->CONST(V4IR::BoolType, (mode == EvalCode || mode == QmlBinding));
            next->next = args;
            args = next;

            entryBlock->EXP(entryBlock->CALL(entryBlock->NAME(V4IR::Name::builtin_declare_vars, 0, 0), args));
        }
    }

    unsigned returnAddress = entryBlock->newTemp();

    entryBlock->MOVE(entryBlock->TEMP(returnAddress), entryBlock->CONST(V4IR::UndefinedType, 0));
    exitBlock->RET(exitBlock->TEMP(returnAddress));
    V4IR::ExprList *throwArgs = function->New<V4IR::ExprList>();
    throwArgs->expr = throwBlock->TEMP(returnAddress);
    throwBlock->EXP(throwBlock->CALL(throwBlock->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    throwBlock->JUMP(exitBlock);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_throwBlock, throwBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);

    for (FormalParameterList *it = formals; it; it = it->next) {
        _function->RECEIVE(it->name.toString());
    }

    foreach (const Environment::Member &member, _env->members) {
        if (member.function) {
            V4IR::Function *function = defineFunction(member.function->name.toString(), member.function, member.function->formals,
                                                    member.function->body ? member.function->body->elements : 0);
            if (! _env->parent) {
                move(_block->NAME(member.function->name.toString(), member.function->identifierToken.startLine, member.function->identifierToken.startColumn),
                     _block->CLOSURE(function));
            } else {
                assert(member.index >= 0);
                move(_block->LOCAL(member.index, 0), _block->CLOSURE(function));
            }
        }
    }
    if (_function->usesArgumentsObject) {
        move(identifier("arguments", ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn),
             _block->CALL(_block->NAME(V4IR::Name::builtin_setup_argument_object,
                     ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), 0));
    }

    sourceElements(body);

    _function->insertBasicBlock(_exitBlock);

    _block->JUMP(_exitBlock);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_throwBlock, throwBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);
    qSwap(_loop, loop);

    leaveEnvironment();

    qSwap(_mode, mode);

    return function;
}

bool Codegen::visit(IdentifierPropertyName *ast)
{
    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(NumericLiteralPropertyName *ast)
{
    _property = QString::number(ast->id, 'g', 16);
    return false;
}

bool Codegen::visit(StringLiteralPropertyName *ast)
{
    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(FunctionSourceElement *ast)
{
    statement(ast->declaration);
    return false;
}

bool Codegen::visit(StatementSourceElement *ast)
{
    statement(ast->statement);
    return false;
}

bool Codegen::visit(Block *ast)
{
    for (StatementList *it = ast->statements; it; it = it->next) {
        statement(it->statement);
    }
    return false;
}

bool Codegen::visit(BreakStatement *ast)
{
    if (!_loop)
        throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Break outside of loop"));
    Loop *loop = 0;
    if (ast->label.isEmpty())
        loop = _loop;
    else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label)
                break;
        }
        if (!loop)
            throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Undefined label '%1'").arg(ast->label.toString()));
    }
    unwindException(loop->scopeAndFinally);
    _block->JUMP(loop->breakBlock);
    return false;
}

bool Codegen::visit(ContinueStatement *ast)
{
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
        if (!loop)
            throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Undefined label '%1'").arg(ast->label.toString()));
    }
    if (!loop)
        throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "continue outside of loop"));
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
    V4IR::BasicBlock *loopbody = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *loopcond = _function->newBasicBlock(loopbody);
    V4IR::BasicBlock *loopend = _function->newBasicBlock(groupStartBlock());

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
    return false;
}

bool Codegen::visit(ExpressionStatement *ast)
{
    if (_mode == EvalCode || _mode == QmlBinding) {
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
    V4IR::BasicBlock *foreachin = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock(foreachin);
    V4IR::BasicBlock *foreachend = _function->newBasicBlock(groupStartBlock());

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
    V4IR::BasicBlock *forcond = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *forbody = _function->newBasicBlock(forcond);
    V4IR::BasicBlock *forstep = _function->newBasicBlock(forcond);
    V4IR::BasicBlock *forend = _function->newBasicBlock(groupStartBlock());

    enterLoop(ast, forcond, forend, forstep);

    statement(ast->initialiser);
    _block->JUMP(forcond);

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
    V4IR::BasicBlock *iftrue = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *iffalse = ast->ko ? _function->newBasicBlock(groupStartBlock()) : 0;
    V4IR::BasicBlock *endif = _function->newBasicBlock(groupStartBlock());
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
    // check that no outer loop contains the label
    Loop *l = _loop;
    while (l) {
        if (l->labelledStatement->label == ast->label) {
            QString error = QString(QStringLiteral("Label '%1' has already been declared")).arg(ast->label.toString());
            throwSyntaxError(ast->firstSourceLocation(), error);
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
        V4IR::BasicBlock *breakBlock = _function->newBasicBlock(groupStartBlock());
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
    V4IR::BasicBlock *foreachin = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock(foreachin);
    V4IR::BasicBlock *foreachend = _function->newBasicBlock(groupStartBlock());

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
    V4IR::BasicBlock *forcond = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *forbody = _function->newBasicBlock(forcond);
    V4IR::BasicBlock *forstep = _function->newBasicBlock(forcond);
    V4IR::BasicBlock *forend = _function->newBasicBlock(groupStartBlock());

    enterLoop(ast, forcond, forend, forstep);

    variableDeclarationList(ast->declarations);
    _block->JUMP(forcond);

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
    if (_mode != FunctionCode && _mode != QmlBinding)
        throwSyntaxError(ast->returnToken, QCoreApplication::translate("qv4codegen", "Return statement outside of function"));
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
    V4IR::BasicBlock *switchend = _function->newBasicBlock(groupStartBlock());

    if (ast->block) {
        Result lhs = expression(ast->expression);
        V4IR::BasicBlock *switchcond = _function->newBasicBlock(groupStartBlock());
        _block->JUMP(switchcond);
        V4IR::BasicBlock *previousBlock = 0;

        QHash<Node *, V4IR::BasicBlock *> blockMap;

        enterLoop(ast, 0, switchend, 0);

        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock(groupStartBlock());
            blockMap[clause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        if (ast->block->defaultClause) {
            _block = _function->newBasicBlock(groupStartBlock());
            blockMap[ast->block->defaultClause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = ast->block->defaultClause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock(groupStartBlock());
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
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock());
            cjump(binop(V4IR::OpStrictEqual, *lhs, *rhs), iftrue, iffalse);
            _block = iffalse;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Result rhs = expression(clause->expression);
            V4IR::BasicBlock *iftrue = blockMap[clause];
            V4IR::BasicBlock *iffalse = _function->newBasicBlock(groupStartBlock());
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
    Result expr = expression(ast->expression);
    move(_block->TEMP(_returnAddress), *expr);
    _block->JUMP(_throwBlock);
    return false;
}

bool Codegen::visit(TryStatement *ast)
{
    _function->hasTry = true;

    if (_function->isStrict && ast->catchExpression &&
            (ast->catchExpression->name == QLatin1String("eval") || ast->catchExpression->name == QLatin1String("arguments")))
        throwSyntaxError(ast->catchExpression->identifierToken, QCoreApplication::translate("qv4codegen", "Catch variable name may not be eval or arguments in strict mode"));

    V4IR::BasicBlock *tryBody = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *catchBody =  _function->newBasicBlock(groupStartBlock());
    // We always need a finally body to clean up the exception handler
    V4IR::BasicBlock *finallyBody = _function->newBasicBlock(groupStartBlock());

    V4IR::BasicBlock *throwBlock = _function->newBasicBlock(groupStartBlock());
    V4IR::ExprList *throwArgs = _function->New<V4IR::ExprList>();
    throwArgs->expr = throwBlock->TEMP(_returnAddress);
    throwBlock->EXP(throwBlock->CALL(throwBlock->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    throwBlock->JUMP(catchBody);
    qSwap(_throwBlock, throwBlock);

    int hasException = _block->newTemp();
    move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, false));

    // Pass the hidden "needRethrow" TEMP to the
    // builtin_delete_exception_handler, in order to have those TEMPs alive for
    // the duration of the exception handling block.
    V4IR::ExprList *finishTryArgs = _function->New<V4IR::ExprList>();
    finishTryArgs->init(_block->TEMP(hasException));

    ScopeAndFinally tcf(_scopeAndFinally, ast->finallyExpression, finishTryArgs);
    _scopeAndFinally = &tcf;

    int exception_to_rethrow  = _block->newTemp();

    _block->TRY(tryBody, catchBody,
                _function->newString(ast->catchExpression ? ast->catchExpression->name.toString() : QString()),
                _block->TEMP(exception_to_rethrow));

    _block = tryBody;
    statement(ast->statement);
    _block->JUMP(finallyBody);

    _block = catchBody;

    if (ast->catchExpression) {
        // check if an exception got thrown within catch. Go to finally
        // and then rethrow
        V4IR::BasicBlock *b = _function->newBasicBlock(groupStartBlock());
        _block->CJUMP(_block->TEMP(hasException), finallyBody, b);
        _block = b;
    }

    move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, true));

    if (ast->catchExpression) {
        ++_function->insideWithOrCatch;
        {
            ScopeAndFinally scope(_scopeAndFinally, ScopeAndFinally::CatchScope);
            _scopeAndFinally = &scope;
            statement(ast->catchExpression->statement);
            _scopeAndFinally = scope.parent;
        }
        --_function->insideWithOrCatch;
        move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, false));
    }
    _block->JUMP(finallyBody);

    _scopeAndFinally = tcf.parent;

    qSwap(_throwBlock, throwBlock);

    V4IR::BasicBlock *after = _function->newBasicBlock(groupStartBlock());
    _block = finallyBody;

    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_finish_try, 0, 0), finishTryArgs));

    if (ast->finallyExpression && ast->finallyExpression->statement)
        statement(ast->finallyExpression->statement);

    V4IR::BasicBlock *rethrowBlock = _function->newBasicBlock(groupStartBlock());
    _block->CJUMP(_block->TEMP(hasException), rethrowBlock, after);
    _block = rethrowBlock;
    move(_block->TEMP(_returnAddress), _block->TEMP(exception_to_rethrow));
    _block->JUMP(_throwBlock);

    _block = after;

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
            _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0)));
            // fall through
        case ScopeAndFinally::CatchScope:
            _scopeAndFinally = _scopeAndFinally->parent;
            --_function->insideWithOrCatch;
            break;
        case ScopeAndFinally::TryScope: {
            _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_finish_try, 0, 0), _scopeAndFinally->finishTryArgs));
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
    variableDeclarationList(ast->declarations);
    return false;
}

bool Codegen::visit(WhileStatement *ast)
{
    V4IR::BasicBlock *whilecond = _function->newBasicBlock(groupStartBlock());
    V4IR::BasicBlock *whilebody = _function->newBasicBlock(whilecond);
    V4IR::BasicBlock *whileend = _function->newBasicBlock(groupStartBlock());

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
    _function->hasWith = true;

    V4IR::BasicBlock *withBlock = _function->newBasicBlock(groupStartBlock());

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

    V4IR::BasicBlock *next = _function->newBasicBlock(groupStartBlock());
    _block->JUMP(next);
    _block = next;

    return false;
}

bool Codegen::visit(UiArrayBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectDefinition *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiPublicMember *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiScriptBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiSourceElement *)
{
    assert(!"not implemented");
    return false;
}

void Codegen::throwSyntaxErrorOnEvalOrArgumentsInStrictMode(V4IR::Expr *expr, const SourceLocation& loc)
{
    if (!_env->isStrict)
        return;
    if (V4IR::Name *n = expr->asName()) {
        if (*n->id != QLatin1String("eval") && *n->id != QLatin1String("arguments"))
            return;
    } else if (V4IR::Temp *t = expr->asTemp()) {
        if (!t->isArgumentsOrEval)
            return;
    } else {
        return;
    }
    throwSyntaxError(loc, QCoreApplication::translate("qv4codegen", "Variable name may not be eval or arguments in strict mode"));
}

void Codegen::throwSyntaxError(const SourceLocation &loc, const QString &detail)
{
    QQmlError error;
    error.setUrl(QUrl::fromLocalFile(_module->fileName));
    error.setDescription(detail);
    error.setLine(loc.startLine);
    error.setColumn(loc.startColumn);
    _errors << error;
}

void Codegen::throwReferenceError(const SourceLocation &loc, const QString &detail)
{
    QQmlError error;
    error.setUrl(QUrl::fromLocalFile(_module->fileName));
    error.setDescription(detail);
    error.setLine(loc.startLine);
    error.setColumn(loc.startColumn);
    _errors << error;
}

void RuntimeCodegen::throwSyntaxError(const AST::SourceLocation &loc, const QString &detail)
{
    context->throwSyntaxError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

void RuntimeCodegen::throwReferenceError(const AST::SourceLocation &loc, const QString &detail)
{
    context->throwReferenceError(detail, _module->fileName, loc.startLine, loc.startColumn);
}
