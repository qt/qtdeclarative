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

#include "qv4codegen_p.h"
#include "qv4util_p.h"
#include "qv4engine_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <QtCore/QLinkedList>
#include <QtCore/QStack>
#include <private/qqmljsast_p.h>
#include <private/qv4string_p.h>
#include <private/qv4value_p.h>
#include <private/qv4bytecodegenerator_p.h>

#ifndef V4_BOOTSTRAP
#include <qv4context_p.h>
#endif

#include <cmath>
#include <iostream>

#ifdef CONST
#undef CONST
#endif

QT_USE_NAMESPACE
using namespace QV4;
using namespace QQmlJS;
using namespace AST;

QT_BEGIN_NAMESPACE
namespace QV4 {

struct ScopeAndFinally {
    enum ScopeType {
        WithScope,
        TryScope,
        CatchScope
    };

    ScopeAndFinally *parent;
    AST::Finally *finally;
    ScopeType type;

    ScopeAndFinally(ScopeAndFinally *parent, ScopeType t = WithScope) : parent(parent), finally(0), type(t) {}
    ScopeAndFinally(ScopeAndFinally *parent, AST::Finally *finally)
    : parent(parent), finally(finally), type(TryScope)
    {}
};

struct Loop {
    AST::LabelledStatement *labelledStatement;
    AST::Statement *node;
    Moth::BytecodeGenerator::Label *breakLabel;
    Moth::BytecodeGenerator::Label *continueLabel;
    Loop *parent;
    ScopeAndFinally *scopeAndFinally;

    Loop(AST::Statement *node, Moth::BytecodeGenerator::Label *breakLabel, Moth::BytecodeGenerator::Label *continueLabel, Loop *parent)
        : labelledStatement(0), node(node), breakLabel(breakLabel), continueLabel(continueLabel), parent(parent) {}
};
} // QV4 namespace
QT_END_NAMESPACE

static inline void setLocation(IR::Stmt *s, const SourceLocation &loc)
{
    if (s && loc.isValid())
        s->location = loc;
}

static bool cjumpCanHandle(IR::AluOp op)
{
    switch (op) {
    case IR::OpIn:
    case IR::OpInstanceof:
    case IR::OpEqual:
    case IR::OpNotEqual:
    case IR::OpGe:
    case IR::OpGt:
    case IR::OpLe:
    case IR::OpLt:
    case IR::OpStrictEqual:
    case IR::OpStrictNotEqual:
        return true;
    default:
        return false;
    }
}

static inline void setJumpOutLocation(IR::Stmt *s, const Statement *body,
                                      const SourceLocation &fallback)
{
    switch (body->kind) {
    // Statements where we might never execute the last line.
    // Use the fallback.
    case Statement::Kind_ConditionalExpression:
    case Statement::Kind_ForEachStatement:
    case Statement::Kind_ForStatement:
    case Statement::Kind_IfStatement:
    case Statement::Kind_LocalForEachStatement:
    case Statement::Kind_LocalForStatement:
    case Statement::Kind_WhileStatement:
        setLocation(s, fallback);
        break;
    default:
        setLocation(s, body->lastSourceLocation());
        break;
    }
}

static inline bool isSimpleExpr(IR::Expr *e)
{
    switch (e->exprKind) {
    case IR::Expr::TempExpr:
    case IR::Expr::ArgLocalExpr:
    case IR::Expr::ConstExpr:
        return true;
    default:
        return false;
    }
}

static inline QV4::Runtime::RuntimeMethods aluOpFunction(IR::AluOp op)
{
    switch (op) {
    case IR::OpInvalid:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpIfTrue:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpNot:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpUMinus:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpUPlus:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpCompl:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpBitAnd:
        return QV4::Runtime::bitAnd;
    case IR::OpBitOr:
        return QV4::Runtime::bitOr;
    case IR::OpBitXor:
        return QV4::Runtime::bitXor;
    case IR::OpAdd:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpSub:
        return QV4::Runtime::sub;
    case IR::OpMul:
        return QV4::Runtime::mul;
    case IR::OpDiv:
        return QV4::Runtime::div;
    case IR::OpMod:
        return QV4::Runtime::mod;
    case IR::OpLShift:
        return QV4::Runtime::shl;
    case IR::OpRShift:
        return QV4::Runtime::shr;
    case IR::OpURShift:
        return QV4::Runtime::ushr;
    case IR::OpGt:
        return QV4::Runtime::greaterThan;
    case IR::OpLt:
        return QV4::Runtime::lessThan;
    case IR::OpGe:
        return QV4::Runtime::greaterEqual;
    case IR::OpLe:
        return QV4::Runtime::lessEqual;
    case IR::OpEqual:
        return QV4::Runtime::equal;
    case IR::OpNotEqual:
        return QV4::Runtime::notEqual;
    case IR::OpStrictEqual:
        return QV4::Runtime::strictEqual;
    case IR::OpStrictNotEqual:
        return QV4::Runtime::strictNotEqual;
    case IR::OpInstanceof:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpIn:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpAnd:
        return QV4::Runtime::InvalidRuntimeMethod;
    case IR::OpOr:
        return QV4::Runtime::InvalidRuntimeMethod;
    default:
        Q_ASSERT(!"Unknown AluOp");
        return QV4::Runtime::InvalidRuntimeMethod;
    }
};


Codegen::ScanFunctions::ScanFunctions(Codegen *cg, const QString &sourceCode, CompilationMode defaultProgramMode)
    : _cg(cg)
    , _sourceCode(sourceCode)
    , _variableEnvironment(0)
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
    Environment *e = _cg->newEnvironment(node, _variableEnvironment, compilationMode);
    if (!e->isStrict)
        e->isStrict = _cg->_strictMode;
    _envStack.append(e);
    _variableEnvironment = e;
}

void Codegen::ScanFunctions::leaveEnvironment()
{
    _envStack.pop();
    _variableEnvironment = _envStack.isEmpty() ? 0 : _envStack.top();
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
                    if (str == QLatin1String("use strict")) {
                        _variableEnvironment->isStrict = true;
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
    if (_variableEnvironment->isStrict) {
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
        if (parameters->name == QLatin1String("arguments"))
            _variableEnvironment->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
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
    if (! _variableEnvironment->hasDirectEval) {
        if (IdentifierExpression *id = cast<IdentifierExpression *>(ast->base)) {
            if (id->name == QLatin1String("eval")) {
                if (_variableEnvironment->usesArgumentsObject == Environment::ArgumentsObjectUnknown)
                    _variableEnvironment->usesArgumentsObject = Environment::ArgumentsObjectUsed;
                _variableEnvironment->hasDirectEval = true;
            }
        }
    }
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _variableEnvironment->maxNumberOfArguments = qMax(_variableEnvironment->maxNumberOfArguments, argc);
    return true;
}

bool Codegen::ScanFunctions::visit(NewMemberExpression *ast)
{
    int argc = 0;
    for (ArgumentList *it = ast->arguments; it; it = it->next)
        ++argc;
    _variableEnvironment->maxNumberOfArguments = qMax(_variableEnvironment->maxNumberOfArguments, argc);
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
    _variableEnvironment->maxNumberOfArguments = qMax(_variableEnvironment->maxNumberOfArguments, index);
    return true;
}

bool Codegen::ScanFunctions::visit(VariableDeclaration *ast)
{
    if (_variableEnvironment->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
    checkName(ast->name, ast->identifierToken);
    if (ast->name == QLatin1String("arguments"))
        _variableEnvironment->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
    if (ast->scope == AST::VariableDeclaration::VariableScope::ReadOnlyBlockScope && !ast->expression) {
        _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Missing initializer in const declaration"));
        return false;
    }
    QString name = ast->name.toString();
    const Environment::Member *m = 0;
    if (_variableEnvironment->memberInfo(name, &m)) {
        if (m->isLexicallyScoped() || ast->isLexicallyScoped()) {
            _cg->throwSyntaxError(ast->identifierToken, QStringLiteral("Identifier %1 has already been declared").arg(name));
            return false;
        }
    }
    _variableEnvironment->enter(ast->name.toString(), ast->expression ? Environment::VariableDefinition : Environment::VariableDeclaration, ast->scope);
    return true;
}

bool Codegen::ScanFunctions::visit(IdentifierExpression *ast)
{
    checkName(ast->name, ast->identifierToken);
    if (_variableEnvironment->usesArgumentsObject == Environment::ArgumentsObjectUnknown && ast->name == QLatin1String("arguments"))
        _variableEnvironment->usesArgumentsObject = Environment::ArgumentsObjectUsed;
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
        if (_sourceCode.midRef(firstToken.offset, firstToken.length) == QLatin1String("function")) {
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
    if (_variableEnvironment->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
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
        QString key = it->assignment->name->asString();
        if (QV4::String::toArrayIndex(key) != UINT_MAX)
            ++argc;
        ++argc;
        if (AST::cast<AST::PropertyGetterSetter *>(it->assignment))
            ++argc;
    }
    _variableEnvironment->maxNumberOfArguments = qMax(_variableEnvironment->maxNumberOfArguments, argc);

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
    if (_variableEnvironment->isStrict) {
        _cg->throwSyntaxError(ast->withToken, QStringLiteral("'with' statement is not allowed in strict mode"));
        return false;
    }

    return true;
}

bool Codegen::ScanFunctions::visit(DoWhileStatement *ast) {
    {
        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_variableEnvironment->isStrict);
        Node::accept(ast->statement, this);
    }
    Node::accept(ast->expression, this);
    return false;
}

bool Codegen::ScanFunctions::visit(ForStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_variableEnvironment->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(LocalForStatement *ast) {
    Node::accept(ast->declarations, this);
    Node::accept(ast->condition, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_variableEnvironment->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(ForEachStatement *ast) {
    Node::accept(ast->initialiser, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_variableEnvironment->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(LocalForEachStatement *ast) {
    Node::accept(ast->declaration, this);
    Node::accept(ast->expression, this);

    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_variableEnvironment->isStrict);
    Node::accept(ast->statement, this);

    return false;
}

bool Codegen::ScanFunctions::visit(ThisExpression *)
{
    _variableEnvironment->usesThis = true;
    return false;
}

bool Codegen::ScanFunctions::visit(Block *ast) {
    TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, _variableEnvironment->isStrict ? false : _allowFuncDecls);
    Node::accept(ast->statements, this);
    return false;
}

void Codegen::ScanFunctions::enterFunction(Node *ast, const QString &name, FormalParameterList *formals, FunctionBody *body, FunctionExpression *expr, bool isExpression)
{
    bool wasStrict = false;
    if (_variableEnvironment) {
        _variableEnvironment->hasNestedFunctions = true;
        // The identifier of a function expression cannot be referenced from the enclosing environment.
        if (expr)
            _variableEnvironment->enter(name, Environment::FunctionDefinition, AST::VariableDeclaration::FunctionScope, expr);
        if (name == QLatin1String("arguments"))
            _variableEnvironment->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
        wasStrict = _variableEnvironment->isStrict;
    }

    enterEnvironment(ast, FunctionCode);
    checkForArguments(formals);

    _variableEnvironment->isNamedFunctionExpression = isExpression && !name.isEmpty();
    _variableEnvironment->formals = formals;

    if (body)
        checkDirectivePrologue(body->elements);

    if (wasStrict || _variableEnvironment->isStrict) {
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


Codegen::Codegen(QV4::Compiler::JSUnitGenerator *jsUnitGenerator, bool strict)
    : _module(0)
    , _function(0)
    , _block(0)
    , _exitBlock(0)
    , _returnAddress(0)
    , _variableEnvironment(0)
    , _loop(0)
    , _labelledStatement(0)
    , _scopeAndFinally(0)
    , jsUnitGenerator(jsUnitGenerator)
    , _strictMode(strict)
    , _fileNameIsUrl(false)
    , hasError(false)
{
}

void Codegen::generateFromProgram(const QString &fileName,
                                  const QString &sourceCode,
                                  Program *node,
                                  QV4::IR::Module *module,
                                  CompilationMode mode,
                                  const QStringList &inheritedLocals)
{
    Q_ASSERT(node);

    _module = module;
    _variableEnvironment = 0;

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
                                             QV4::IR::Module *module)
{
    _module = module;
    _module->setFileName(fileName);
    _variableEnvironment = 0;

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
    _variableEnvironment = _envMap.value(node);
    Q_ASSERT(_variableEnvironment);
}

void Codegen::leaveEnvironment()
{
    Q_ASSERT(_variableEnvironment);
    _variableEnvironment = _variableEnvironment->parent;
}

void Codegen::enterLoop(Statement *node, QV4::Moth::BytecodeGenerator::Label *breakLabel, QV4::Moth::BytecodeGenerator::Label *continueLabel)
{
    _loop = new Loop(node, breakLabel, continueLabel, _loop);
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

IR::Expr *Codegen::argument(IR::Expr *expr)
{
    if (expr && !expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

// keeps references alive, converts other expressions to temps
IR::Expr *Codegen::reference(IR::Expr *expr)
{
    if (hasError)
        return 0;

    if (expr && !expr->asTemp() && !expr->asArgLocal() && !expr->asName() && !expr->asMember() && !expr->asSubscript()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

Codegen::Reference Codegen::unop(IR::AluOp op, const Reference &expr, const SourceLocation &loc)
{
    if (hasError)
        return _expr.result;

#ifndef V4_BOOTSTRAP
    if (expr.type == Reference::Const) {
        auto v = Value::fromReturnedValue(expr.constant);
        if (v.isNumber()) {
            switch (op) {
            case IR::OpNot:
                return Reference::fromConst(this, Runtime::method_uNot(v));
            case IR::OpUMinus:
                return Reference::fromConst(this, Runtime::method_uMinus(v));
            case IR::OpUPlus:
                return expr;
            case IR::OpCompl:
                return Reference::fromConst(this, Runtime::method_complement(v));
            case IR::OpIncrement:
                return Reference::fromConst(this, Runtime::method_increment(v));
            case IR::OpDecrement:
                return Reference::fromConst(this, Runtime::method_decrement(v));
            default:
                break;
            }
        }
    }
#endif // V4_BOOTSTRAP

    auto dest = Reference::fromTemp(this, _block->newTemp());

    switch (op) {
    case IR::OpUMinus: {
        QV4::Moth::Instruction::UMinus uminus;
        uminus.source = expr.asRValue();
        uminus.result = dest.asLValue();
        bytecodeGenerator->addInstruction(uminus);
    } break;
    case IR::OpUPlus: {
        QV4::Moth::Instruction::UPlus uplus;
        uplus.source = expr.asRValue();
        uplus.result = dest.asLValue();
        bytecodeGenerator->addInstruction(uplus);
    } break;
    case IR::OpNot: {
        QV4::Moth::Instruction::UNot unot;
        unot.source = expr.asRValue();
        unot.result = dest.asLValue();
        bytecodeGenerator->addInstruction(unot);
    } break;
    case IR::OpCompl: {
        QV4::Moth::Instruction::UCompl ucompl;
        ucompl.source = expr.asRValue();
        ucompl.result = dest.asLValue();
        bytecodeGenerator->addInstruction(ucompl);
    } break;
    case IR::OpIncrement: {
        QV4::Moth::Instruction::Increment inc;
        inc.source = expr.asRValue();
        inc.result = dest.asLValue();
        bytecodeGenerator->addInstruction(inc);
    } break;
    case IR::OpDecrement: {
        QV4::Moth::Instruction::Decrement dec;
        dec.source = expr.asRValue();
        dec.result = dest.asLValue();
        bytecodeGenerator->addInstruction(dec);
    } break;
    default:
        Q_UNIMPLEMENTED();
    }

    return dest;
}

IR::Expr *Codegen::binop(IR::AluOp op, IR::Expr *left, IR::Expr *right, const AST::SourceLocation &loc)
{
    if (hasError)
        return 0;

    TempScope scope(_function);

    if (IR::Const *c1 = left->asConst()) {
        if (IR::Const *c2 = right->asConst()) {
            if ((c1->type & IR::NumberType) && (c2->type & IR::NumberType)) {
                switch (op) {
                case IR::OpAdd: return _block->CONST(IR::NumberType, c1->value + c2->value);
                case IR::OpAnd: return _block->CONST(IR::BoolType, c1->value ? c2->value : 0);
                case IR::OpBitAnd: return _block->CONST(IR::NumberType, int(c1->value) & int(c2->value));
                case IR::OpBitOr: return _block->CONST(IR::NumberType, int(c1->value) | int(c2->value));
                case IR::OpBitXor: return _block->CONST(IR::NumberType, int(c1->value) ^ int(c2->value));
                case IR::OpDiv: return _block->CONST(IR::NumberType, c1->value / c2->value);
                case IR::OpEqual: return _block->CONST(IR::BoolType, c1->value == c2->value);
                case IR::OpNotEqual: return _block->CONST(IR::BoolType, c1->value != c2->value);
                case IR::OpStrictEqual: return _block->CONST(IR::BoolType, c1->value == c2->value);
                case IR::OpStrictNotEqual: return _block->CONST(IR::BoolType, c1->value != c2->value);
                case IR::OpGe: return _block->CONST(IR::BoolType, c1->value >= c2->value);
                case IR::OpGt: return _block->CONST(IR::BoolType, c1->value > c2->value);
                case IR::OpLe: return _block->CONST(IR::BoolType, c1->value <= c2->value);
                case IR::OpLt: return _block->CONST(IR::BoolType, c1->value < c2->value);
                case IR::OpLShift: return _block->CONST(IR::NumberType, QV4::Primitive::toInt32(c1->value) << (QV4::Primitive::toUInt32(c2->value) & 0x1f));
                case IR::OpMod: return _block->CONST(IR::NumberType, std::fmod(c1->value, c2->value));
                case IR::OpMul: return _block->CONST(IR::NumberType, c1->value * c2->value);
                case IR::OpOr: return _block->CONST(IR::NumberType, c1->value ? c1->value : c2->value);
                case IR::OpRShift: return _block->CONST(IR::NumberType, QV4::Primitive::toInt32(c1->value) >> (QV4::Primitive::toUInt32(c2->value) & 0x1f));
                case IR::OpSub: return _block->CONST(IR::NumberType, c1->value - c2->value);
                case IR::OpURShift: return _block->CONST(IR::NumberType,QV4::Primitive::toUInt32(c1->value) >> (QV4::Primitive::toUInt32(c2->value) & 0x1f));

                case IR::OpInstanceof:
                case IR::OpIn:
                    break;

                case IR::OpIfTrue: // unary ops
                case IR::OpNot:
                case IR::OpUMinus:
                case IR::OpUPlus:
                case IR::OpCompl:
                case IR::OpIncrement:
                case IR::OpDecrement:
                case IR::OpInvalid:
                    break;
                }
            }
        }
    } else if (op == IR::OpAdd) {
        if (IR::String *s1 = left->asString()) {
            if (IR::String *s2 = right->asString()) {
                return _block->STRING(_function->newString(*s1->value + *s2->value));
            }
        }
    }

    if (!left->asTemp() && !left->asArgLocal() && !left->asConst()) {
        const unsigned t = _block->newTemp();
        setLocation(move(_block->TEMP(t), left), loc);
        left = _block->TEMP(t);
    }

    if (!right->asTemp() && !right->asArgLocal() && !right->asConst()) {
        const unsigned t = _block->newTemp();
        setLocation(move(_block->TEMP(t), right), loc);
        right = _block->TEMP(t);
    }

    Q_ASSERT(left->asTemp() || left->asArgLocal() || left->asConst());
    Q_ASSERT(right->asTemp() || right->asArgLocal() || right->asConst());

    return _block->BINOP(op, left, right);
}

IR::Expr *Codegen::call(IR::Expr *base, IR::ExprList *args)
{
    if (hasError)
        return 0;
    base = reference(base);
    return _block->CALL(base, args);
}

IR::Stmt *Codegen::move(IR::Expr *target, IR::Expr *source, IR::AluOp op)
{
    if (hasError)
        return 0;

    Q_ASSERT(target->isLValue());

    if (op != IR::OpInvalid) {
        return move(target, binop(op, target, source));
    }

    TempScope scope(_function);

    if (!source->asTemp() && !source->asConst() && !target->asTemp() && !source->asArgLocal() && !target->asArgLocal()) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }
    if (source->asConst() && !target->asTemp() && !target->asArgLocal()) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }

    return _block->MOVE(target, source);
}

IR::Stmt *Codegen::cjump(IR::Expr *cond, IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
{
    if (hasError)
        return 0;

    TempScope scope(_function);

    if (! (cond->asTemp() || (cond->asBinop() && cjumpCanHandle(cond->asBinop()->op)) )) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), cond);
        cond = _block->TEMP(t);
    }
    return _block->CJUMP(cond, iftrue, iffalse);
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
    TempScope scope(_function);

    _block->nextLocation = ast->firstSourceLocation();
    accept(ast);
}

void Codegen::statement(ExpressionNode *ast)
{
    TempScope scope(_function);

    if (! ast) {
        return;
    } else {
        Result r(nx);
        qSwap(_expr, r);
        accept(ast);
        if (hasError)
            return;
        qSwap(_expr, r);
//        if (r.format == ex) {
//            if (r->asCall()) {
//                _block->EXP(*r); // the nest nx representation for calls is EXP(CALL(c..))
//            } else if (r->asTemp() || r->asArgLocal()) {
//                // there is nothing to do
//            } else {
//                unsigned t = _block->newTemp();
//                move(_block->TEMP(t), *r);
//            }
//        }

        if (r.result.isValid() && !r.result.isTempLocalArg())
            r.result.asRValue(); // triggers side effects
    }
}

void Codegen::condition(ExpressionNode *ast, IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
{
    if (ast) {
        Result r(iftrue, iffalse);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            setLocation(cjump(*r, r.iftrue, r.iffalse), ast->firstSourceLocation());
        }
    }
}

Codegen::Reference Codegen::expression(ExpressionNode *ast)
{
    Result r;
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
    }
    return r.result;
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
    TempScope scope(_function);

    if (!ast->expression)
        return;
    Reference rhs = expression(ast->expression);
    if (hasError)
        return;
    Reference lhs = referenceForName(ast->name.toString(), true);
    lhs.storeConsume(rhs);
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

    const unsigned t = _block->newTemp();

    TempScope scope(_function);

    IR::ExprList *args = 0;
    IR::ExprList *current = 0;
    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next) {
            IR::ExprList *arg = _function->New<IR::ExprList>();
            if (!current) {
                args = arg;
            } else {
                current->next = arg;
            }
            current = arg;
            current->expr = _block->CONST(IR::MissingType, 0);
        }
        Result expr = expression(it->expression);
        if (hasError)
            return false;

        IR::ExprList *arg = _function->New<IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;

        IR::Expr *exp = *expr;
        if (exp->asTemp() || expr->asArgLocal() || exp->asConst()) {
            current->expr = exp;
        } else {
            unsigned value = _block->newTemp();
            move(_block->TEMP(value), exp);
            current->expr = _block->TEMP(value);
        }
    }
    for (Elision *elision = ast->elision; elision; elision = elision->next) {
        IR::ExprList *arg = _function->New<IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;
        current->expr = _block->CONST(IR::MissingType, 0);
    }

    move(_block->TEMP(t), _block->CALL(_block->NAME(IR::Name::builtin_define_array, 0, 0), args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(ArrayMemberExpression *ast)
{
    if (hasError)
        return false;

    Reference base = expression(ast->base);
    if (hasError)
        return false;
    Reference index = expression(ast->expression);
    _expr.result = Reference::fromSubscript(base, index);
    return false;
}

static IR::AluOp baseOp(int op)
{
    switch ((QSOperator::Op) op) {
    case QSOperator::InplaceAnd: return IR::OpBitAnd;
    case QSOperator::InplaceSub: return IR::OpSub;
    case QSOperator::InplaceDiv: return IR::OpDiv;
    case QSOperator::InplaceAdd: return IR::OpAdd;
    case QSOperator::InplaceLeftShift: return IR::OpLShift;
    case QSOperator::InplaceMod: return IR::OpMod;
    case QSOperator::InplaceMul: return IR::OpMul;
    case QSOperator::InplaceOr: return IR::OpBitOr;
    case QSOperator::InplaceRightShift: return IR::OpRShift;
    case QSOperator::InplaceURightShift: return IR::OpURShift;
    case QSOperator::InplaceXor: return IR::OpBitXor;
    default: return IR::OpInvalid;
    }
}

bool Codegen::visit(BinaryExpression *ast)
{
    if (hasError)
        return false;

    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            IR::BasicBlock *iftrue = _function->newBasicBlock(exceptionHandler());
            condition(ast->left, iftrue, _expr.iffalse);
            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            IR::BasicBlock *iftrue = _function->newBasicBlock(exceptionHandler());
            IR::BasicBlock *endif = _function->newBasicBlock(exceptionHandler());

            const unsigned r = _block->newTemp();

            Result lhs = expression(ast->left);
            if (hasError)
                return false;
            move(_block->TEMP(r), *lhs);
            setLocation(cjump(_block->TEMP(r), iftrue, endif), ast->operatorToken);
            _block = iftrue;
            Result rhs = expression(ast->right);
            if (hasError)
                return false;
            move(_block->TEMP(r), *rhs);
            _block->JUMP(endif);

            _expr.code = _block->TEMP(r);
            _block = endif;
        }
        return false;
    } else if (ast->op == QSOperator::Or) {
        if (_expr.accept(cx)) {
            IR::BasicBlock *iffalse = _function->newBasicBlock(exceptionHandler());
            condition(ast->left, _expr.iftrue, iffalse);
            _block = iffalse;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            IR::BasicBlock *iffalse = _function->newBasicBlock(exceptionHandler());
            IR::BasicBlock *endif = _function->newBasicBlock(exceptionHandler());

            const unsigned r = _block->newTemp();
            Result lhs = expression(ast->left);
            if (hasError)
                return false;
            move(_block->TEMP(r), *lhs);
            setLocation(cjump(_block->TEMP(r), endif, iffalse), ast->operatorToken);
            _block = iffalse;
            Result rhs = expression(ast->right);
            if (hasError)
                return false;
            move(_block->TEMP(r), *rhs);
            _block->JUMP(endif);

            _block = endif;
            _expr.code = _block->TEMP(r);
        }
        return false;
    }

    Reference left = expression(ast->left);
    if (hasError)
        return false;

    switch (ast->op) {
    case QSOperator::Or:
    case QSOperator::And:
        Q_UNREACHABLE(); // handled separately above
        break;

    case QSOperator::Assign: {
//        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
//            return false;
        Reference right = expression(ast->right);
        if (hasError)
            return false;
        if (!left.isLValue()) {
            throwReferenceError(ast->operatorToken, QStringLiteral("left-hand side of assignment operator is not an lvalue"));
            return false;
        }

        left.storeConsume(right);
        _expr.result = left;
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
//        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
//            return false;
//        Result right = expression(ast->right);
//        if (hasError)
//            return false;
//        if (!left->isLValue()) {
//            throwSyntaxError(ast->operatorToken, QStringLiteral("left-hand side of inplace operator is not an lvalue"));
//            return false;
//        }

//        TempScope scope(_function);
//        const unsigned t = _block->newTemp();
//        move(_block->TEMP(t), *right);
//        move(left, _block->TEMP(t), baseOp(ast->op));
//        if (!_expr.accept(nx))
//            _expr.code = left;
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
    case QSOperator::StrictNotEqual:
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
        if (left.isConst()) {
            //### TODO: try constant folding?
        }

        auto leftParam = left.asRValue();

        Reference right = expression(ast->right);
        if (hasError)
            return false;

        _expr.result = Reference::fromTemp(this, _block->newTemp());
        binopHelper(IR::binaryOperator(ast->op), leftParam, right.asRValue(), _expr.result.base);

        break;
    }

    } // switch

    return false;
}

QV4::Moth::Param Codegen::binopHelper(IR::AluOp oper, const QV4::Moth::Param &left,
                                      const QV4::Moth::Param &right, const QV4::Moth::Param &dest)
{
    if (oper == IR::OpAdd) {
        QV4::Moth::Instruction::Add add;
        add.lhs = left;
        add.rhs = right;
        add.result = dest;
        bytecodeGenerator->addInstruction(add);
        return add.result;
    }
    if (oper == IR::OpSub) {
        QV4::Moth::Instruction::Sub sub;
        sub.lhs = left;
        sub.rhs = right;
        sub.result = dest;
        bytecodeGenerator->addInstruction(sub);
        return sub.result;
    }
    if (oper == IR::OpMul) {
        QV4::Moth::Instruction::Mul mul;
        mul.lhs = left;
        mul.rhs = right;
        mul.result = dest;
        bytecodeGenerator->addInstruction(mul);
        return mul.result;
    }
    if (oper == IR::OpBitAnd) {
//        if (left.isConstant())
//            std::swap(left, right);
//        if (right.isConstant()) {
//            QV4::Moth::Instruction::BitAndConst bitAnd;
//            bitAnd.lhs = left;
//            bitAnd.rhs = Primitive::fromReturnedValue(jsUnitGenerator->constant(right.index)).toInteger();
//            bitAnd.result = dest;
//            bytecodeGenerator->addInstruction(bitAnd);
//            return bitAnd.result;
//        }
        QV4::Moth::Instruction::BitAnd bitAnd;
        bitAnd.lhs = left;
        bitAnd.rhs = right;
        bitAnd.result = dest;
        bytecodeGenerator->addInstruction(bitAnd);
        return bitAnd.result;
    }
    if (oper == IR::OpBitOr) {
//        if (left.isConstant())
//            std::swap(left, right);
//        if (right.isConstant()) {
//            QV4::Moth::Instruction::BitOrConst bitOr;
//            bitOr.lhs = left;
//            bitOr.rhs = Primitive::fromReturnedValue(jsUnitGenerator->constant(right.index)).toInteger();
//            bitOr.result = dest;
//            bytecodeGenerator->addInstruction(bitOr);
//            return bitOr.result;
//        }
        QV4::Moth::Instruction::BitOr bitOr;
        bitOr.lhs = left;
        bitOr.rhs = right;
        bitOr.result = dest;
        bytecodeGenerator->addInstruction(bitOr);
        return bitOr.result;
    }
    if (oper == IR::OpBitXor) {
//        if (leftSource->asConst())
//            qSwap(leftSource, rightSource);
//        if (IR::Const *c = rightSource->asConst()) {
//            QV4::Moth::Instruction::BitXorConst bitXor;
//            bitXor.lhs = left;
//            bitXor.rhs = convertToValue(c).Value::toInt32();
//            bitXor.result = dest;
//            bytecodeGenerator->addInstruction(bitXor);
//            return bitXor.result;
//        }
        QV4::Moth::Instruction::BitXor bitXor;
        bitXor.lhs = left;
        bitXor.rhs = right;
        bitXor.result = dest;
        bytecodeGenerator->addInstruction(bitXor);
        return bitXor.result;
    }
    if (oper == IR::OpRShift) {
//        if (IR::Const *c = rightSource->asConst()) {
//            QV4::Moth::Instruction::ShrConst shr;
//            shr.lhs = left;
//            shr.rhs = convertToValue(c).Value::toInt32() & 0x1f;
//            shr.result = dest;
//            bytecodeGenerator->addInstruction(shr);
//            return shr.result;
//        }
        QV4::Moth::Instruction::Shr shr;
        shr.lhs = left;
        shr.rhs = right;
        shr.result = dest;
        bytecodeGenerator->addInstruction(shr);
        return shr.result;
    }
    if (oper == IR::OpLShift) {
//        if (IR::Const *c = rightSource->asConst()) {
//            QV4::Moth::Instruction::ShlConst shl;
//            shl.lhs = left;
//            shl.rhs = convertToValue(c).Value::toInt32() & 0x1f;
//            shl.result = dest;
//            bytecodeGenerator->addInstruction(shl);
//            return shl.result;
//        }
        QV4::Moth::Instruction::Shl shl;
        shl.lhs = left;
        shl.rhs = right;
        shl.result = dest;
        bytecodeGenerator->addInstruction(shl);
        return shl.result;
    }

    if (oper == IR::OpInstanceof || oper == IR::OpIn || oper == IR::OpAdd) {
        QV4::Moth::Instruction::BinopContext binop;
        if (oper == IR::OpInstanceof)
            binop.alu = QV4::Runtime::instanceof;
        else if (oper == IR::OpIn)
            binop.alu = QV4::Runtime::in;
        else
            binop.alu = QV4::Runtime::add;
        binop.lhs = left;
        binop.rhs = right;
        binop.result = dest;
        Q_ASSERT(binop.alu != QV4::Runtime::InvalidRuntimeMethod);
        bytecodeGenerator->addInstruction(binop);
        return binop.result;
    } else {
        auto binopFunc = aluOpFunction(oper);
        Q_ASSERT(binopFunc != QV4::Runtime::InvalidRuntimeMethod);
        QV4::Moth::Instruction::Binop binop;
        binop.alu = binopFunc;
        binop.lhs = left;
        binop.rhs = right;
        binop.result = dest;
        bytecodeGenerator->addInstruction(binop);
        return binop.result;
    }

    Q_UNIMPLEMENTED();
}

bool Codegen::visit(CallExpression *ast)
{
    if (hasError)
        return false;

    Reference base = expression(ast->base);
    if (hasError)
        return false;

    Q_ASSERT(base.type == Reference::Name); //### TODO: support more calls

    auto argc = pushArgs(ast->arguments);
    if (hasError)
        return false;

    QV4::Moth::Instruction::CallActivationProperty call;
    call.name = base.nameIndex;
    call.argc = argc;
    call.callData = 0;
    call.result = Moth::Param::createTemp(0); bytecodeGenerator->addInstruction(call);
    _expr.result = Reference::fromTemp(this, 0);
    return false;
}

int Codegen::pushArgs(ArgumentList *args)
{
    int minNrOfStackEntries = offsetof(QV4::CallData, args)/sizeof(QV4::Value);
    int argc = 0;
    for (ArgumentList *it = args; it; it = it->next) {
        Reference arg = expression(it->expression);
        if (hasError)
            return -1;
        Reference::fromTemp(this, minNrOfStackEntries + argc).store(arg);
        argc += 1;
    }
    return argc;
}

bool Codegen::visit(ConditionalExpression *ast)
{
    if (hasError)
        return true;

    const unsigned t = _block->newTemp();
    _expr = Reference::fromTemp(this, t);

    TempScope scope(_function);

    Reference r = expression(ast->expression);

    // ### handle const Reference

    Moth::BytecodeGenerator::Jump jump_else = bytecodeGenerator->jumpNe(r.asRValue());

    _expr.result.store(expression(ast->ok));

    Moth::BytecodeGenerator::Jump jump_endif = bytecodeGenerator->jump();

    jump_else.link();

    _expr.result.store(expression(ast->ko));

    jump_endif.link();

    return false;
}

bool Codegen::visit(DeleteExpression *ast)
{
//    if (hasError)
//        return false;

//    IR::Expr* expr = *expression(ast->expression);
//    if (hasError)
//        return false;
//    // Temporaries cannot be deleted
//    IR::ArgLocal *al = expr->asArgLocal();
//    if (al && al->index < static_cast<unsigned>(_variableEnvironment->members.size())) {
//        // Trying to delete a function argument might throw.
//        if (_function->isStrict) {
//            throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
//            return false;
//        }
//        _expr.code = _block->CONST(IR::BoolType, 0);
//        return false;
//    }
//    if (_function->isStrict && expr->asName()) {
//        throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
//        return false;
//    }

//    // [[11.4.1]] Return true if it's not a reference
//    if (expr->asConst() || expr->asString()) {
//        _expr.code = _block->CONST(IR::BoolType, 1);
//        return false;
//    }

//    // Return values from calls are also not a reference, but we have to
//    // perform the call to allow for side effects.
//    if (expr->asCall()) {
//        _block->EXP(expr);
//        _expr.code = _block->CONST(IR::BoolType, 1);
//        return false;
//    }
//    if (expr->asTemp() ||
//            (expr->asArgLocal() &&
//             expr->asArgLocal()->index >= static_cast<unsigned>(_variableEnvironment->members.size()))) {
//        _expr.code = _block->CONST(IR::BoolType, 1);
//        return false;
//    }

//    IR::ExprList *args = _function->New<IR::ExprList>();
//    args->init(reference(expr));
//    _expr.code = call(_block->NAME(IR::Name::builtin_delete, ast->deleteToken.startLine, ast->deleteToken.startColumn), args);
    return false;
}

bool Codegen::visit(FalseLiteral *)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromConst(this, QV4::Encode(false));
    return false;
}

bool Codegen::visit(FieldMemberExpression *ast)
{
    if (hasError)
        return false;

    Reference base = expression(ast->base);
    if (hasError)
        return false;
    _expr.result = Reference::fromMember(base, ast->name.toString());
    return false;
}

bool Codegen::visit(FunctionExpression *ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

    int function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    _expr.code = _block->CLOSURE(function);
    return false;
}

Moth::Param Codegen::paramForNull()
{
    return paramForConst(QV4::Encode::null());
}

Moth::Param Codegen::paramForUndefined()
{
    return paramForConst(QV4::Encode::undefined());
}

Moth::Param Codegen::paramForBool(bool b)
{
    return paramForConst(QV4::Encode(b));
}

Moth::Param Codegen::paramForNumber(double d)
{
    // ### specialize for integers
    return paramForConst(QV4::Encode(d));
}

Moth::Param Codegen::paramForConst(QV4::ReturnedValue v)
{
    int idx = jsUnitGenerator->registerConstant(v);
    return Moth::Param::createConstant(idx);
}

Codegen::Reference Codegen::referenceForName(const QString &name, bool isLhs)
{
    uint scope = 0;
    Environment *e = _variableEnvironment;
    IR::Function *f = _function;

    while (f && e->parent) {
        if (f->insideWithOrCatch || (f->isNamedExpression && QStringRef(f->name) == name))
            goto loadByName;

        int index = e->findMember(name);
        Q_ASSERT (index < e->members.size());
        if (index != -1) {
            if (isLhs && f->isStrict && (name == QLatin1String("arguments") || name == QLatin1String("eval"))) {
                // ### check that this converts correctly
                // throwSyntaxError(loc, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
            }
            return Reference::fromLocal(this, index, scope);
        }
        const int argIdx = f->indexOfArgument(QStringRef(&name));
        if (argIdx != -1)
            return Reference::fromArgument(this, argIdx, scope);

        if (!e->isStrict && e->hasDirectEval)
            goto loadByName;

        ++scope;
        e = e->parent;
        f = f->outer;
    }

    // This hook allows implementing QML lookup semantics
// ####
//    if (IR::Expr *fallback = fallbackNameLookup(name, line, col))
//        return fallback;

// ###
//    if (!e->parent && (!f || !f->insideWithOrCatch) && _variableEnvironment->compilationMode != EvalCode && e->compilationMode != QmlBinding)
//        return _block->GLOBALNAME(name, line, col);

    // global context or with. Lookup by name
  loadByName:
    return Reference::fromName(this, name);
}

IR::Expr *Codegen::fallbackNameLookup(const QString &name, int line, int col)
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

    _expr.result = referenceForName(ast->name.toString(), false);
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
    TempScope scope(_function);

    Result base = expression(ast->expression);
    if (hasError)
        return false;
    IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asArgLocal() && !expr->asName() && !expr->asMember()) {
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

    const unsigned t = _block->newTemp();

    TempScope scope(_function);

    Result base = expression(ast->base);
    if (hasError)
        return false;
    IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asArgLocal() && !expr->asName() && !expr->asMember()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }

    IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        if (hasError)
            return false;
        IR::Expr *actual = argument(*arg);
        *args_it = _function->New<IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    move(_block->TEMP(t), _block->NEW(expr, args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(NotExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(IR::OpNot, expression(ast->expression), ast->notToken);
    return false;
}

bool Codegen::visit(NullExpression *)
{
    if (hasError)
        return false;

    if (_expr.accept(cx)) _block->JUMP(_expr.iffalse);
    else _expr.code = _block->CONST(IR::NullType, 0);

    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromConst(this, QV4::Encode(ast->value));
    return false;
}

struct ObjectPropertyValue {
    ObjectPropertyValue()
        : value(0)
        , getter(-1)
        , setter(-1)
    {}

    IR::Expr *value;
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

    const unsigned t = _block->newTemp();
    TempScope scope(_function);

    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        QString name = it->assignment->name->asString();
        if (PropertyNameAndValue *nv = AST::cast<AST::PropertyNameAndValue *>(it->assignment)) {
            Result value = expression(nv->value);
            if (hasError)
                return false;
            ObjectPropertyValue &v = valueMap[name];
            if (v.hasGetter() || v.hasSetter() || (_function->isStrict && v.value)) {
                throwSyntaxError(nv->lastSourceLocation(),
                                 QStringLiteral("Illegal duplicate key '%1' in object literal").arg(name));
                return false;
            }

            if (IR::Const *c = (*value)->asConst()) {
                valueMap[name].value = c;
            } else {
                unsigned t = _block->newTemp();
                move(_block->TEMP(t), *value);
                valueMap[name].value = _block->TEMP(t);
            }
        } else if (PropertyGetterSetter *gs = AST::cast<AST::PropertyGetterSetter *>(it->assignment)) {
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

    // The linked-list arguments to builtin_define_object_literal
    // begin with a CONST counting the number of key/value pairs, followed by the
    // key value pairs, followed by the array entries.
    IR::ExprList *args = _function->New<IR::ExprList>();

    IR::Const *entryCountParam = _function->New<IR::Const>();
    entryCountParam->init(IR::SInt32Type, 0);
    args->expr = entryCountParam;
    args->next = 0;

    IR::ExprList *keyValueEntries = 0;
    IR::ExprList *currentKeyValueEntry = 0;
    int keyValueEntryCount = 0;
    IR::ExprList *arrayEntries = 0;

    IR::ExprList *currentArrayEntry = 0;

    for (QMap<QString, ObjectPropertyValue>::iterator it = valueMap.begin(); it != valueMap.end(); ) {
        IR::ExprList **currentPtr = 0;
        uint keyAsIndex = QV4::String::toArrayIndex(it.key());
        if (keyAsIndex != UINT_MAX) {
            if (!arrayEntries) {
                arrayEntries = _function->New<IR::ExprList>();
                currentArrayEntry = arrayEntries;
            } else {
                currentArrayEntry->next = _function->New<IR::ExprList>();
                currentArrayEntry = currentArrayEntry->next;
            }
            currentPtr = &currentArrayEntry;
            IR::Const *idx = _function->New<IR::Const>();
            idx->init(IR::UInt32Type, keyAsIndex);
            (*currentPtr)->expr = idx;
        } else {
            if (!keyValueEntries) {
                keyValueEntries = _function->New<IR::ExprList>();
                currentKeyValueEntry = keyValueEntries;
            } else {
                currentKeyValueEntry->next = _function->New<IR::ExprList>();
                currentKeyValueEntry = currentKeyValueEntry->next;
            }
            currentPtr = &currentKeyValueEntry;
            (*currentPtr)->expr = _block->NAME(it.key(), 0, 0);
            keyValueEntryCount++;
        }

        IR::ExprList *&current = *currentPtr;
        if (it->value) {
            current->next = _function->New<IR::ExprList>();
            current = current->next;
            current->expr = _block->CONST(IR::BoolType, true);

            current->next = _function->New<IR::ExprList>();
            current = current->next;
            current->expr = it->value;
        } else {
            current->next = _function->New<IR::ExprList>();
            current = current->next;
            current->expr = _block->CONST(IR::BoolType, false);

            unsigned getter = _block->newTemp();
            unsigned setter = _block->newTemp();
            move(_block->TEMP(getter), it->hasGetter() ? _block->CLOSURE(it->getter) : _block->CONST(IR::UndefinedType, 0));
            move(_block->TEMP(setter), it->hasSetter() ? _block->CLOSURE(it->setter) : _block->CONST(IR::UndefinedType, 0));

            current->next = _function->New<IR::ExprList>();
            current = current->next;
            current->expr = _block->TEMP(getter);
            current->next = _function->New<IR::ExprList>();
            current = current->next;
            current->expr = _block->TEMP(setter);
        }

        it = valueMap.erase(it);
    }

    entryCountParam->value = keyValueEntryCount;

    if (keyValueEntries)
        args->next = keyValueEntries;
    if (arrayEntries) {
        if (currentKeyValueEntry)
            currentKeyValueEntry->next = arrayEntries;
        else
            args->next = arrayEntries;
    }

    move(_block->TEMP(t), _block->CALL(_block->NAME(IR::Name::builtin_define_object_literal,
         ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), args));

    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(PostDecrementExpression *ast)
{
//    if (hasError)
//        return false;

//    Result expr = expression(ast->base);
//    if (hasError)
//        return false;
//    if (!expr->isLValue()) {
//        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
//        return false;
//    }
//    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken))
//        return false;

//    const unsigned oldValue = _block->newTemp();
//    setLocation(move(_block->TEMP(oldValue), unop(IR::OpUPlus, *expr, ast->decrementToken)), ast->decrementToken);

//    TempScope scope(_function);
//    const unsigned  newValue = _block->newTemp();
//    setLocation(move(_block->TEMP(newValue), binop(IR::OpSub, _block->TEMP(oldValue), _block->CONST(IR::NumberType, 1), ast->decrementToken)), ast->decrementToken);
//    setLocation(move(*expr, _block->TEMP(newValue)), ast->decrementToken);

//    if (!_expr.accept(nx))
//        _expr.code = _block->TEMP(oldValue);

    return false;
}

bool Codegen::visit(PostIncrementExpression *ast)
{
//    if (hasError)
//        return false;

//    Result expr = expression(ast->base);
//    if (hasError)
//        return false;
//    if (!expr->isLValue()) {
//        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
//        return false;
//    }
//    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken))
//        return false;

//    const unsigned oldValue = _block->newTemp();
//    setLocation(move(_block->TEMP(oldValue), unop(IR::OpUPlus, *expr, ast->incrementToken)), ast->incrementToken);

//    TempScope scope(_function);
//    const unsigned  newValue = _block->newTemp();
//    setLocation(move(_block->TEMP(newValue), binop(IR::OpAdd, _block->TEMP(oldValue), _block->CONST(IR::NumberType, 1), ast->incrementToken)), ast->incrementToken);
//    setLocation(move(*expr, _block->TEMP(newValue)), ast->incrementToken);

//    if (!_expr.accept(nx))
//        _expr.code = _block->TEMP(oldValue);

    return false;
}

bool Codegen::visit(PreDecrementExpression *ast)
{    if (hasError)
        return false;

    Reference expr = expression(ast->expression);
    if (hasError)
        return false;
    if (!expr.isLValue()) {
        throwReferenceError(ast->expression->lastSourceLocation(), QStringLiteral("Prefix ++ operator applied to value that is not a reference."));
        return false;
    }

    //### TODO
//    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken))
//        return false;
    auto tmp = unop(IR::OpDecrement, expr, ast->decrementToken);
    if (_expr.accept(nx)) {
//        setLocation(move(*expr, op), ast->incrementToken);
        expr.store(tmp);
    } else {
        if (!tmp.isTempLocalArg()) {
            auto tmp2 = Reference::fromTemp(this, _block->newTemp());
            tmp2.store(tmp);
            tmp = tmp2;
        }
        expr.store(tmp);
        _expr.result = tmp;
    }
    return false;
}

bool Codegen::visit(PreIncrementExpression *ast)
{
    if (hasError)
        return false;

    Reference expr = expression(ast->expression);
    if (hasError)
        return false;
    if (!expr.isLValue()) {
        throwReferenceError(ast->expression->lastSourceLocation(), QStringLiteral("Prefix ++ operator applied to value that is not a reference."));
        return false;
    }

    //### TODO
//    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken))
//        return false;
    auto tmp = unop(IR::OpIncrement, expr, ast->incrementToken);
    if (_expr.accept(nx)) {
//        setLocation(move(*expr, op), ast->incrementToken);
        expr.store(tmp);
    } else {
        if (!tmp.isTempLocalArg()) {
            auto tmp2 = Reference::fromTemp(this, _block->newTemp());
            tmp2.store(tmp);
            tmp = tmp2;
        }
        expr.store(tmp);
        _expr.result = tmp;
    }
    return false;
}

bool Codegen::visit(RegExpLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromTemp(this, bytecodeGenerator->newTemp());

    Moth::Instruction::LoadRegExp instr;
    instr.result = _expr.result.asLValue();
    instr.regExpId = jsUnitGenerator->registerRegExp(ast);
    bytecodeGenerator->addInstruction(instr);
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromTemp(this, bytecodeGenerator->newTemp());

    Moth::Instruction::LoadRuntimeString instr;
    instr.result = _expr.result.asLValue();
    instr.stringId = jsUnitGenerator->registerString(ast->value.toString());
    bytecodeGenerator->addInstruction(instr);
    return false;
}

bool Codegen::visit(ThisExpression *ast)
{
    if (hasError)
        return false;

    Reference r = Reference::fromTemp(this, bytecodeGenerator->newTemp());

    Moth::Instruction::LoadThis loadThis;
    loadThis.result = r.asLValue();
    bytecodeGenerator->addInstruction(loadThis);

    _expr.result = r;
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(IR::OpCompl, expression(ast->expression), ast->tildeToken);
    return false;
}

bool Codegen::visit(TrueLiteral *)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromConst(this, QV4::Encode(true));
    return false;
}

bool Codegen::visit(TypeOfExpression *ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

    Result expr = expression(ast->expression);
    if (hasError)
        return false;
    IR::ExprList *args = _function->New<IR::ExprList>();
    args->init(reference(*expr));
    _expr.code = call(_block->NAME(IR::Name::builtin_typeof, ast->typeofToken.startLine, ast->typeofToken.startColumn), args);
    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(IR::OpUMinus, expression(ast->expression), ast->minusToken);
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(IR::OpUPlus, expression(ast->expression), ast->plusToken);
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

    statement(ast->expression);
    _expr.result = Reference::fromConst(this, Encode::undefined());
    return false;
}

bool Codegen::visit(FunctionDeclaration * ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

    if (_variableEnvironment->compilationMode == QmlBinding) {
        Reference source = Reference::fromName(this, ast->name.toString());
        Reference target = Reference::fromTemp(this, _returnAddress);
        target.store(source);
    }
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
    QStack<IR::BasicBlock *> exceptionHandlers;
    qSwap(_exceptionHandlers, exceptionHandlers);

    ScopeAndFinally *scopeAndFinally = 0;

    enterEnvironment(ast);
    IR::Function *function = _module->newFunction(name, _function);
    int functionIndex = _module->functions.count() - 1;

    IR::BasicBlock *entryBlock = function->newBasicBlock(0);
    IR::BasicBlock *exitBlock = function->newBasicBlock(0, IR::Function::DontInsertBlock);
    function->hasDirectEval = _variableEnvironment->hasDirectEval || _variableEnvironment->compilationMode == EvalCode
            || _module->debugMode; // Conditional breakpoints are like eval in the function
    function->usesArgumentsObject = _variableEnvironment->parent && (_variableEnvironment->usesArgumentsObject == Environment::ArgumentsObjectUsed);
    function->usesThis = _variableEnvironment->usesThis;
    function->maxNumberOfArguments = qMax(_variableEnvironment->maxNumberOfArguments, (int)QV4::Global::ReservedArgumentCount);
    function->isStrict = _variableEnvironment->isStrict;
    function->isNamedExpression = _variableEnvironment->isNamedFunctionExpression;
    function->isQmlBinding = _variableEnvironment->compilationMode == QmlBinding;

    AST::SourceLocation loc = ast->firstSourceLocation();
    function->line = loc.startLine;
    function->column = loc.startColumn;

    QV4::Moth::BytecodeGenerator bytecode(function);
    QV4::Moth::BytecodeGenerator *savedBytecodeGenerator;
    savedBytecodeGenerator = bytecodeGenerator;
    bytecodeGenerator = &bytecode;


    if (function->usesArgumentsObject)
        _variableEnvironment->enter(QStringLiteral("arguments"), Environment::VariableDeclaration, AST::VariableDeclaration::FunctionScope);

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (_variableEnvironment->compilationMode == FunctionCode || _variableEnvironment->compilationMode == QmlBinding) {
        unsigned t = 0;
        for (Environment::MemberMap::iterator it = _variableEnvironment->members.begin(), end = _variableEnvironment->members.end(); it != end; ++it) {
            const QString &local = it.key();
            function->LOCAL(local);
            (*it).index = t;
            entryBlock->MOVE(entryBlock->LOCAL(t, 0), entryBlock->CONST(IR::UndefinedType, 0));
            ++t;
        }
    } else {
        if (!_variableEnvironment->isStrict) {
            for (const QString &inheritedLocal : qAsConst(inheritedLocals)) {
                function->LOCAL(inheritedLocal);
                unsigned tempIndex = entryBlock->newTemp();
                Environment::Member member = { Environment::UndefinedMember,
                                               static_cast<int>(tempIndex), 0,
                                               AST::VariableDeclaration::VariableScope::FunctionScope };
                _variableEnvironment->members.insert(inheritedLocal, member);
            }
        }

        IR::ExprList *args = 0;
        for (Environment::MemberMap::const_iterator it = _variableEnvironment->members.constBegin(), cend = _variableEnvironment->members.constEnd(); it != cend; ++it) {
            const QString &local = it.key();
            IR::ExprList *next = function->New<IR::ExprList>();
            next->expr = entryBlock->NAME(local, 0, 0);
            next->next = args;
            args = next;
        }
        if (args) {
            IR::ExprList *next = function->New<IR::ExprList>();
            next->expr = entryBlock->CONST(IR::BoolType, false); // ### Investigate removal of bool deletable
            next->next = args;
            args = next;

            entryBlock->EXP(entryBlock->CALL(entryBlock->NAME(IR::Name::builtin_declare_vars, 0, 0), args));
        }
    }

    { // reserve space for outgoing call arguments
        Q_ASSERT(function->tempCount == 0);
        int minNrOfStackEntries = offsetof(QV4::CallData, args)/sizeof(QV4::Value);
        function->tempCount = minNrOfStackEntries + _variableEnvironment->maxNumberOfArguments;
        function->currentTemp = function->tempCount;
    }

    unsigned returnAddress = entryBlock->newTemp();

//###    setLocation(exitBlock->RET(exitBlock->TEMP(returnAddress)), ast->lastSourceLocation());

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);

    {
        auto retTemp = Reference::fromTemp(this, _returnAddress);
        retTemp.store(Reference::fromConst(this, Encode::undefined()));
    }

    for (FormalParameterList *it = formals; it; it = it->next) {
        _function->RECEIVE(it->name.toString());
    }

    for (const Environment::Member &member : qAsConst(_variableEnvironment->members)) {
        if (member.function) {
            const int function = defineFunction(member.function->name.toString(), member.function, member.function->formals,
                                                member.function->body ? member.function->body->elements : 0);
            auto func = Reference::fromClosure(this, function);
            if (! _variableEnvironment->parent) {
                Reference name = Reference::fromName(this, member.function->name.toString());
                name.store(func);
            } else {
                Q_ASSERT(member.index >= 0);
                Reference local = Reference::fromLocal(this, member.index, 0);
                local.store(func);
            }
        }
    }
    if (_function->usesArgumentsObject) {
        //### TODO
//        move(identifier(QStringLiteral("arguments"), ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn),
//             _block->CALL(_block->NAME(IR::Name::builtin_setup_argument_object,
//                     ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), 0));
    }
    if (_function->usesThis && !_function->isStrict) {
        // make sure we convert this to an object
        _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_convert_this_to_object,
                ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), 0));
    }

    beginFunctionBodyHook();

    sourceElements(body);

    _function->addBasicBlock(_exitBlock);

    _block->JUMP(_exitBlock);

    {
        QV4::Moth::Instruction::Ret ret;
        ret.result = Reference::fromTemp(this, _returnAddress).base;
        bytecodeGenerator->addInstruction(ret);
    }

    _function->code = bytecodeGenerator->finalize();
    static const bool showCode = qEnvironmentVariableIsSet("QV4_SHOW_BYTECODE");
    if (showCode) {
        qDebug() << "=== Bytecode for" << *_function->name;
        QV4::Moth::dumpBytecode(_function->code);
        qDebug();
    }

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);
    qSwap(_exceptionHandlers, exceptionHandlers);
    qSwap(_loop, loop);

    leaveEnvironment();

    bytecodeGenerator = savedBytecodeGenerator;

    return functionIndex;
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

    TempScope scope(_function);

    for (StatementList *it = ast->statements; it; it = it->next) {
        statement(it->statement);
    }
    return false;
}

bool Codegen::visit(BreakStatement *ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

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
    bytecodeGenerator->jump().link(*loop->breakLabel);

    return false;
}

bool Codegen::visit(ContinueStatement *ast)
{
    if (hasError)
        return false;

    TempScope scope(_function);

    Loop *loop = 0;
    if (ast->label.isEmpty()) {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->continueLabel)
                break;
        }
    } else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label) {
                if (!loop->continueLabel)
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
    bytecodeGenerator->jump().link(*loop->continueLabel);

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

    TempScope scope(_function);

    Moth::BytecodeGenerator::Label body = bytecodeGenerator->label();
    Moth::BytecodeGenerator::Label cond = bytecodeGenerator->newLabel();
    Moth::BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    enterLoop(ast, &end, &cond);

    statement(ast->statement);

    cond.link();
    Reference r = expression(ast->expression);
    bytecodeGenerator->jumpEq(r.asRValue()).link(body);
    end.link();

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

    TempScope scope(_function);

    if (_variableEnvironment->compilationMode == EvalCode || _variableEnvironment->compilationMode == QmlBinding) {
        Reference e = expression(ast->expression);
        Reference retVal = Reference::fromTemp(this, _returnAddress);
        retVal.store(e);
    } else {
        statement(ast->expression);
    }
    return false;
}

bool Codegen::visit(ForEachStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

#if 0
    IR::BasicBlock *foreachin = _function->newBasicBlock(exceptionHandler());
    IR::BasicBlock *foreachbody = _function->newBasicBlock(exceptionHandler());
    IR::BasicBlock *foreachend = _function->newBasicBlock(exceptionHandler());

    int objectToIterateOn = _block->newTemp();
    Result expr = expression(ast->expression);
    if (hasError)
        return false;
    move(_block->TEMP(objectToIterateOn), *expr);
    IR::ExprList *args = _function->New<IR::ExprList>();
    args->init(_block->TEMP(objectToIterateOn));

    int iterator = _block->newTemp();
    move(_block->TEMP(iterator), _block->CALL(_block->NAME(IR::Name::builtin_foreach_iterator_object, 0, 0), args));

    enterLoop(ast, foreachend, foreachin);
    _block->JUMP(foreachin);

    _block = foreachbody;
    int temp = _block->newTemp();
    Result init = expression(ast->initialiser);
    if (hasError)
        return false;
    move(*init, _block->TEMP(temp));
    statement(ast->statement);
    setJumpOutLocation(_block->JUMP(foreachin), ast->statement, ast->forToken);

    _block = foreachin;

    args = _function->New<IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(temp), _block->CALL(_block->NAME(IR::Name::builtin_foreach_next_property_name, 0, 0), args));
    int null = _block->newTemp();
    move(_block->TEMP(null), _block->CONST(IR::NullType, 0));
    setLocation(cjump(_block->BINOP(IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend), ast->forToken);
    _block = foreachend;

    leaveLoop();
#endif
    return false;
}

bool Codegen::visit(ForStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    statement(ast->initialiser);

    Moth::BytecodeGenerator::Label cond = bytecodeGenerator->label();
    Moth::BytecodeGenerator::Label step = bytecodeGenerator->newLabel();
    Moth::BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    enterLoop(ast, &end, &step);

    Reference r = expression(ast->condition);
    bytecodeGenerator->jumpNe(r.asRValue()).link(end);

    statement(ast->statement);

    step.link();

    statement(ast->expression);
    bytecodeGenerator->jump().link(cond);
    end.link();

    leaveLoop();

    return false;
}

bool Codegen::visit(IfStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    Reference r = expression(ast->expression);

    // ### handle const Reference

    Moth::BytecodeGenerator::Jump jump_else = bytecodeGenerator->jumpNe(r.asRValue());
    statement(ast->ok);
    if (ast->ko) {
        Moth::BytecodeGenerator::Jump jump_endif = bytecodeGenerator->jump();
        jump_else.link();
        statement(ast->ko);
        jump_endif.link();
    } else {
        jump_else.link();
    }

    return false;
}

bool Codegen::visit(LabelledStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

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
#if 0
        IR::BasicBlock *breakBlock = _function->newBasicBlock(exceptionHandler());
        enterLoop(ast->statement, breakBlock, /*continueBlock*/ 0);
        statement(ast->statement);
        _block->JUMP(breakBlock);
        _block = breakBlock;
        leaveLoop();
#endif
    }

    return false;
}

bool Codegen::visit(LocalForEachStatement *ast)
{
//    if (hasError)
//        return true;

//    TempScope scope(_function);

//    IR::BasicBlock *foreachin = _function->newBasicBlock(exceptionHandler());
//    IR::BasicBlock *foreachbody = _function->newBasicBlock(exceptionHandler());
//    IR::BasicBlock *foreachend = _function->newBasicBlock(exceptionHandler());

//    variableDeclaration(ast->declaration);

//    int iterator = _block->newTemp();
//    move(_block->TEMP(iterator), *expression(ast->expression));
//    IR::ExprList *args = _function->New<IR::ExprList>();
//    args->init(_block->TEMP(iterator));
//    move(_block->TEMP(iterator), _block->CALL(_block->NAME(IR::Name::builtin_foreach_iterator_object, 0, 0), args));

//    _block->JUMP(foreachin);
//    enterLoop(ast, foreachend, foreachin);

//    _block = foreachbody;
//    int temp = _block->newTemp();
//    move(identifier(ast->declaration->name.toString()), _block->TEMP(temp));
//    statement(ast->statement);
//    setJumpOutLocation(_block->JUMP(foreachin), ast->statement, ast->forToken);

//    _block = foreachin;

//    args = _function->New<IR::ExprList>();
//    args->init(_block->TEMP(iterator));
//    move(_block->TEMP(temp), _block->CALL(_block->NAME(IR::Name::builtin_foreach_next_property_name, 0, 0), args));
//    int null = _block->newTemp();
//    move(_block->TEMP(null), _block->CONST(IR::NullType, 0));
//    setLocation(cjump(_block->BINOP(IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend), ast->forToken);
//    _block = foreachend;

//    leaveLoop();
    return false;
}

bool Codegen::visit(LocalForStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    variableDeclarationList(ast->declarations);

    Moth::BytecodeGenerator::Label cond = bytecodeGenerator->label();
    Moth::BytecodeGenerator::Label step = bytecodeGenerator->newLabel();
    Moth::BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    enterLoop(ast, &end, &step);

    Reference r = expression(ast->condition);
    bytecodeGenerator->jumpNe(r.asRValue()).link(end);

    statement(ast->statement);

    step.link();

    statement(ast->expression);
    bytecodeGenerator->jump().link(cond);
    end.link();

    leaveLoop();

    return false;
}

bool Codegen::visit(ReturnStatement *ast)
{
    if (hasError)
        return true;

    if (_variableEnvironment->compilationMode != FunctionCode && _variableEnvironment->compilationMode != QmlBinding) {
        throwSyntaxError(ast->returnToken, QStringLiteral("Return statement outside of function"));
        return false;
    }
    if (ast->expression) {
        Reference expr = expression(ast->expression);
        Reference::fromTemp(this, _returnAddress).storeConsume(expr);
    }

    // Since we're leaving, don't let any finally statements we emit as part of the unwinding
    // jump to exception handlers at run-time if they throw.
    IR::BasicBlock *unwindBlock = _function->newBasicBlock(/*no exception handler*/Q_NULLPTR);
    _block->JUMP(unwindBlock);
    _block = unwindBlock;

    unwindException(0);

    _block->JUMP(_exitBlock);
    return false;
}

bool Codegen::visit(SwitchStatement *ast)
{
//    if (hasError)
//        return true;

//    TempScope scope(_function);

//    IR::BasicBlock *switchend = _function->newBasicBlock(exceptionHandler());

//    if (ast->block) {
//        int lhs = _block->newTemp();
//        move(_block->TEMP(lhs), *expression(ast->expression));
//        IR::BasicBlock *switchcond = _function->newBasicBlock(exceptionHandler());
//        _block->JUMP(switchcond);
//        IR::BasicBlock *previousBlock = 0;

//        QHash<Node *, IR::BasicBlock *> blockMap;

//        enterLoop(ast, switchend, 0);

//        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
//            CaseClause *clause = it->clause;

//            _block = _function->newBasicBlock(exceptionHandler());
//            blockMap[clause] = _block;

//            if (previousBlock && !previousBlock->isTerminated())
//                previousBlock->JUMP(_block);

//            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
//                statement(it2->statement);

//            previousBlock = _block;
//        }

//        if (ast->block->defaultClause) {
//            _block = _function->newBasicBlock(exceptionHandler());
//            blockMap[ast->block->defaultClause] = _block;

//            if (previousBlock && !previousBlock->isTerminated())
//                previousBlock->JUMP(_block);

//            for (StatementList *it2 = ast->block->defaultClause->statements; it2; it2 = it2->next)
//                statement(it2->statement);

//            previousBlock = _block;
//        }

//        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
//            CaseClause *clause = it->clause;

//            _block = _function->newBasicBlock(exceptionHandler());
//            blockMap[clause] = _block;

//            if (previousBlock && !previousBlock->isTerminated())
//                previousBlock->JUMP(_block);

//            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
//                statement(it2->statement);

//            previousBlock = _block;
//        }

//        leaveLoop();

//        _block->JUMP(switchend);

//        _block = switchcond;
//        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
//            CaseClause *clause = it->clause;
//            Result rhs = expression(clause->expression);
//            IR::BasicBlock *iftrue = blockMap[clause];
//            IR::BasicBlock *iffalse = _function->newBasicBlock(exceptionHandler());
//            setLocation(cjump(binop(IR::OpStrictEqual, _block->TEMP(lhs), *rhs), iftrue, iffalse), clause->caseToken);
//            _block = iffalse;
//        }

//        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
//            CaseClause *clause = it->clause;
//            Result rhs = expression(clause->expression);
//            IR::BasicBlock *iftrue = blockMap[clause];
//            IR::BasicBlock *iffalse = _function->newBasicBlock(exceptionHandler());
//            setLocation(cjump(binop(IR::OpStrictEqual, _block->TEMP(lhs), *rhs), iftrue, iffalse), clause->caseToken);
//            _block = iffalse;
//        }

//        if (DefaultClause *defaultClause = ast->block->defaultClause) {
//            setLocation(_block->JUMP(blockMap[ast->block->defaultClause]), defaultClause->defaultToken);
//        }
//    }

//    _block->JUMP(switchend);

//    _block = switchend;
    return false;
}

bool Codegen::visit(ThrowStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    Result expr = expression(ast->expression);
    move(_block->TEMP(_returnAddress), *expr);
    IR::ExprList *throwArgs = _function->New<IR::ExprList>();
    throwArgs->expr = _block->TEMP(_returnAddress);
    _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    return false;
}

bool Codegen::visit(TryStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    _function->hasTry = true;

    if (_function->isStrict && ast->catchExpression &&
            (ast->catchExpression->name == QLatin1String("eval") || ast->catchExpression->name == QLatin1String("arguments"))) {
        throwSyntaxError(ast->catchExpression->identifierToken, QStringLiteral("Catch variable name may not be eval or arguments in strict mode"));
        return false;
    }

    IR::BasicBlock *surroundingExceptionHandler = exceptionHandler();

    // We always need a finally body to clean up the exception handler
    // exceptions thrown in finally get caught by the surrounding catch block
    IR::BasicBlock *finallyBody = 0;
    IR::BasicBlock *catchBody = 0;
    IR::BasicBlock *catchExceptionHandler = 0;
    IR::BasicBlock *end = _function->newBasicBlock(surroundingExceptionHandler, IR::Function::DontInsertBlock);

    if (ast->finallyExpression)
        finallyBody = _function->newBasicBlock(surroundingExceptionHandler, IR::Function::DontInsertBlock);

    if (ast->catchExpression) {
        // exception handler for the catch body
        catchExceptionHandler = _function->newBasicBlock(0, IR::Function::DontInsertBlock);
        pushExceptionHandler(catchExceptionHandler);
        catchBody =  _function->newBasicBlock(catchExceptionHandler, IR::Function::DontInsertBlock);
        popExceptionHandler();
        pushExceptionHandler(catchBody);
    } else {
        Q_ASSERT(finallyBody);
        pushExceptionHandler(finallyBody);
    }

    IR::BasicBlock *tryBody = _function->newBasicBlock(exceptionHandler());
    _block->JUMP(tryBody);

    ScopeAndFinally tcf(_scopeAndFinally, ast->finallyExpression);
    _scopeAndFinally = &tcf;

    _block = tryBody;
    statement(ast->statement);
    _block->JUMP(finallyBody ? finallyBody : end);

    popExceptionHandler();

    if (ast->catchExpression) {
        pushExceptionHandler(catchExceptionHandler);
        _function->addBasicBlock(catchBody);
        _block = catchBody;

        ++_function->insideWithOrCatch;
        IR::ExprList *catchArgs = _function->New<IR::ExprList>();
        catchArgs->init(_block->STRING(_function->newString(ast->catchExpression->name.toString())));
        _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_push_catch_scope, 0, 0), catchArgs));
        {
            ScopeAndFinally scope(_scopeAndFinally, ScopeAndFinally::CatchScope);
            _scopeAndFinally = &scope;
            statement(ast->catchExpression->statement);
            _scopeAndFinally = scope.parent;
        }
        _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_pop_scope, 0, 0), 0));
        --_function->insideWithOrCatch;
        _block->JUMP(finallyBody ? finallyBody : end);
        popExceptionHandler();

        _function->addBasicBlock(catchExceptionHandler);
        catchExceptionHandler->EXP(catchExceptionHandler->CALL(catchExceptionHandler->NAME(IR::Name::builtin_pop_scope, 0, 0), 0));
        if (finallyBody || surroundingExceptionHandler)
            catchExceptionHandler->JUMP(finallyBody ? finallyBody : surroundingExceptionHandler);
        else
            catchExceptionHandler->EXP(catchExceptionHandler->CALL(catchExceptionHandler->NAME(IR::Name::builtin_rethrow, 0, 0), 0));
    }

    _scopeAndFinally = tcf.parent;

    if (finallyBody) {
        _function->addBasicBlock(finallyBody);
        _block = finallyBody;

        TempScope scope(_function);

        int hasException = _block->newTemp();
        move(_block->TEMP(hasException), _block->CALL(_block->NAME(IR::Name::builtin_unwind_exception, /*line*/0, /*column*/0), 0));

        if (ast->finallyExpression && ast->finallyExpression->statement)
            statement(ast->finallyExpression->statement);

        IR::ExprList *arg = _function->New<IR::ExprList>();
        arg->expr = _block->TEMP(hasException);
        _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_throw, /*line*/0, /*column*/0), arg));
        _block->JUMP(end);
    }

    _function->addBasicBlock(end);
    _block = end;

    return false;
}

void Codegen::unwindException(QV4::ScopeAndFinally *outest)
{
    int savedDepthForWidthOrCatch = _function->insideWithOrCatch;
    ScopeAndFinally *scopeAndFinally = _scopeAndFinally;
    qSwap(_scopeAndFinally, scopeAndFinally);
    while (_scopeAndFinally != outest) {
        switch (_scopeAndFinally->type) {
        case ScopeAndFinally::WithScope:
            // fall through
        case ScopeAndFinally::CatchScope:
            _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_pop_scope, 0, 0)));
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

    Moth::BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    Moth::BytecodeGenerator::Label cond = bytecodeGenerator->label();
    enterLoop(ast, &end, &cond);

    Reference r = expression(ast->expression);
    Moth::BytecodeGenerator::Jump jump_end = bytecodeGenerator->jumpNe(r.asRValue());

    statement(ast->statement);
    bytecodeGenerator->jump().link(cond);

    leaveLoop();
    end.link();
    jump_end.link();

    return false;
}

bool Codegen::visit(WithStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(_function);

    _function->hasWith = true;

    const int withObject = _block->newTemp();
    Result src = expression(ast->expression);
    if (hasError)
        return false;
    _block->MOVE(_block->TEMP(withObject), *src);

    // need an exception handler for with to cleanup the with scope
    IR::BasicBlock *withExceptionHandler = _function->newBasicBlock(exceptionHandler());
    withExceptionHandler->EXP(withExceptionHandler->CALL(withExceptionHandler->NAME(IR::Name::builtin_pop_scope, 0, 0), 0));
    if (!exceptionHandler())
        withExceptionHandler->EXP(withExceptionHandler->CALL(withExceptionHandler->NAME(IR::Name::builtin_rethrow, 0, 0), 0));
    else
        withExceptionHandler->JUMP(exceptionHandler());

    pushExceptionHandler(withExceptionHandler);

    IR::BasicBlock *withBlock = _function->newBasicBlock(exceptionHandler());

    _block->JUMP(withBlock);
    _block = withBlock;
    IR::ExprList *args = _function->New<IR::ExprList>();
    args->init(_block->TEMP(withObject));
    _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_push_with_scope, 0, 0), args));

    ++_function->insideWithOrCatch;
    {
        ScopeAndFinally scope(_scopeAndFinally);
        _scopeAndFinally = &scope;
        statement(ast->statement);
        _scopeAndFinally = scope.parent;
    }
    --_function->insideWithOrCatch;
    _block->EXP(_block->CALL(_block->NAME(IR::Name::builtin_pop_scope, 0, 0), 0));
    popExceptionHandler();

    IR::BasicBlock *next = _function->newBasicBlock(exceptionHandler());
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

bool Codegen::throwSyntaxErrorOnEvalOrArgumentsInStrictMode(IR::Expr *expr, const SourceLocation& loc)
{
    if (!_variableEnvironment->isStrict)
        return false;
    if (IR::Name *n = expr->asName()) {
        if (*n->id != QLatin1String("eval") && *n->id != QLatin1String("arguments"))
            return false;
    } else if (IR::ArgLocal *al = expr->asArgLocal()) {
        if (!al->isArgumentsOrEval)
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
    QQmlJS::DiagnosticMessage error;
    error.message = detail;
    error.loc = loc;
    _errors << error;
}

void Codegen::throwReferenceError(const SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;

    hasError = true;
    QQmlJS::DiagnosticMessage error;
    error.message = detail;
    error.loc = loc;
    _errors << error;
}

QList<QQmlJS::DiagnosticMessage> Codegen::errors() const
{
    return _errors;
}

#ifndef V4_BOOTSTRAP

QList<QQmlError> Codegen::qmlErrors() const
{
    QList<QQmlError> qmlErrors;

    // Short circuit to avoid costly (de)heap allocation of QUrl if there are no errors.
    if (_errors.size() == 0)
        return qmlErrors;

    qmlErrors.reserve(_errors.size());

    QUrl url(_fileNameIsUrl ? QUrl(_module->fileName) : QUrl::fromLocalFile(_module->fileName));
    for (const QQmlJS::DiagnosticMessage &msg: qAsConst(_errors)) {
        QQmlError e;
        e.setUrl(url);
        e.setLine(msg.loc.startLine);
        e.setColumn(msg.loc.startColumn);
        e.setDescription(msg.message);
        qmlErrors << e;
    }

    return qmlErrors;
}

void RuntimeCodegen::throwSyntaxError(const AST::SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;
    hasError = true;
    engine->throwSyntaxError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

void RuntimeCodegen::throwReferenceError(const AST::SourceLocation &loc, const QString &detail)
{
    if (hasError)
        return;
    hasError = true;
    engine->throwReferenceError(detail, _module->fileName, loc.startLine, loc.startColumn);
}

#endif // V4_BOOTSTRAP

Codegen::Reference::~Reference()
{
    writeBack();
}

bool Codegen::Reference::operator==(const Codegen::Reference &other) const
{
    if (type != other.type)
        return false;
    switch (type) {
    case Invalid:
        return true;
    case Temp:
    case Local:
    case Argument:
        return base == other.base;
    case Name:
        return nameIndex == other.nameIndex;
    case Member:
        return base == other.base && nameIndex == other.nameIndex;
    case Subscript:
        return base == other.base && subscript == other.subscript;
    case Const:
        return constant == other.constant;
    }
}

void Codegen::Reference::storeConsume(Reference &r) const
{
    if (*this == r)
        return;

    writeBack();

    if (!isSimple() && !r.isSimple()) {
        r.asRValue(); // trigger load

        Q_ASSERT(r.tempIndex >= 0);
        tempIndex = r.tempIndex;
        r.tempIndex = -1;
        needsWriteBack = true;
        return;
    }

    store(r);
}

void Codegen::Reference::store(const Reference &r) const
{
    Q_ASSERT(type != Const);

    if (*this == r)
        return;

    writeBack();

    Moth::Param b = base;
    if (!isSimple()) {
        if (tempIndex < 0)
            tempIndex = codegen->bytecodeGenerator->newTemp();
        if (!r.isSimple() && r.tempIndex == -1) {
            r.tempIndex = tempIndex;
            r.asRValue(); // trigger load
            r.tempIndex = -1;
            needsWriteBack = true;
            return;
        }
        b = Moth::Param::createTemp(tempIndex);
        needsWriteBack = true;
    }

    if (r.type == Const) {
        QV4::Moth::Instruction::MoveConst move;
        move.source = r.constant;
        move.result = b;
        codegen->bytecodeGenerator->addInstruction(move);
        return;
    }
    Moth::Param x = r.asRValue();
    Q_ASSERT(base != x);
    QV4::Moth::Instruction::Move move;
    move.source = x;
    move.result = b;
    codegen->bytecodeGenerator->addInstruction(move);
}

Moth::Param Codegen::Reference::asRValue() const
{
    writeBack(); // because of possible side effects

    Q_ASSERT(type != Invalid);
    if (type <= Argument)
        return base;

    // need a temp to hold the value
    if (tempIndex < 0)
        tempIndex = codegen->bytecodeGenerator->newTemp();
    Moth::Param temp = Moth::Param::createTemp(tempIndex);
    if (type == Const) {
        QV4::Moth::Instruction::MoveConst move;
        move.source = constant;
        move.result = temp;
        codegen->bytecodeGenerator->addInstruction(move);
    } else if (type == Name) {
        QV4::Moth::Instruction::LoadName load;
        load.name = nameIndex;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == Member) {
//        if (useFastLookups) {
//            Instruction::GetLookup load;
//            load.base = getParam(base);
//            load.index = registerGetterLookup(name);
//            load.result = getResultParam(target);
//            addInstruction(load);
//            return;
//        }
        QV4::Moth::Instruction::LoadProperty load;
        load.base = base;
        load.name = nameIndex;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == Subscript) {
        QV4::Moth::Instruction::LoadElement load;
        load.base = base;
        load.index = subscript;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == Closure) {
        QV4::Moth::Instruction::LoadClosure load;
        load.value = closureId;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else {
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }
    return temp;
}

Moth::Param Codegen::Reference::asLValue() const
{
    Q_ASSERT(type <= LastLValue);

    if (isSimple())
        return base;

    if (tempIndex < 0)
        tempIndex = codegen->bytecodeGenerator->newTemp();
    needsWriteBack = true;
    return Moth::Param::createTemp(tempIndex);
}

void Codegen::Reference::writeBack() const
{
    if (!needsWriteBack)
        return;

    Q_ASSERT(!isSimple());
    Q_ASSERT(tempIndex >= 0);
    needsWriteBack = false;

    Moth::Param temp = Moth::Param::createTemp(tempIndex);
    if (type == Name) {
        QV4::Moth::Instruction::StoreName store;
        store.source = temp;
        store.name = nameIndex;
        codegen->bytecodeGenerator->addInstruction(store);
    } else if (type == Member) {
//        if (useFastLookups) {
//            Instruction::SetLookup store;
//            store.base = getParam(targetBase);
//            store.index = registerSetterLookup(targetName);
//            store.source = getParam(source);
//            addInstruction(store);
//            return;
//        }
        QV4::Moth::Instruction::StoreProperty store;
        store.base = base;
        store.name = nameIndex;
        store.source = temp;
        codegen->bytecodeGenerator->addInstruction(store);
    } else if (type == Subscript) {
        QV4::Moth::Instruction::StoreElement store;
        store.base = base;
        store.index = subscript;
        store.source = temp;
        codegen->bytecodeGenerator->addInstruction(store);
    } else {
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }
}
