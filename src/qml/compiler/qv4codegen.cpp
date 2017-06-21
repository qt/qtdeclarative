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

struct ControlFlow {
    using Reference = Codegen::Reference;
    using BytecodeGenerator = Moth::BytecodeGenerator;
    using Instruction = Moth::Instruction;

    enum Type {
        Loop,
        With,
        Finally,
        Catch
    };

    enum HandlerType {
        Invalid,
        Break,
        Continue,
        Return,
        Throw
    };

    struct Handler {
        HandlerType type;
        QString label;
        BytecodeGenerator::Label linkLabel;
        int tempIndex;
        int value;
    };

    Codegen *cg;
    ControlFlow *parent;
    Type type;

    ControlFlow(Codegen *cg, Type type)
        : cg(cg), parent(cg->_controlFlow), type(type)
    {
        cg->_controlFlow = this;
    }

    virtual ~ControlFlow() {
        cg->_controlFlow = parent;
    }

    void jumpToHandler(const Handler &h) {
        if (h.tempIndex >= 0) {
            Reference val = Reference::fromConst(cg, QV4::Encode(h.value));
            Reference temp = Reference::fromTemp(cg, h.tempIndex);
            temp.store(val);
        }
        cg->bytecodeGenerator->jump().link(h.linkLabel);
    }

    virtual QString label() const { return QString(); }

    bool isSimple() const {
        return type == Loop;
    }

    Handler getParentHandler(HandlerType type, const QString &label = QString()) {
        if (parent)
            return parent->getHandler(type, label);
        switch (type) {
        case Break:
        case Continue:
            return { Invalid, QString(), {}, -1, 0 };
        case Return:
        case Throw:
            return { type, QString(), cg->_exitBlock, -1, 0 };
        case Invalid:
            break;
        }
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        return getParentHandler(type, label);
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return parent ? parent->exceptionHandler() : 0;
    }
    BytecodeGenerator::ExceptionHandler *parentExceptionHandler() {
        return parent ? parent->exceptionHandler() : 0;
    }

    virtual void handleThrow(const Reference &expr) {
        Handler h = getHandler(ControlFlow::Throw);
        if (h.tempIndex >= 0) {
            Reference val = Reference::fromConst(cg, QV4::Encode(h.value));
            Reference temp = Reference::fromTemp(cg, h.tempIndex);
            temp.store(val);
        }
        Instruction::CallBuiltinThrow instr;
        instr.arg = expr.asRValue();
        generator()->addInstruction(instr);
    }

protected:
    QString loopLabel() const {
        QString label;
        if (cg->_labelledStatement) {
            label = cg->_labelledStatement->label.toString();
            cg->_labelledStatement = 0;
        }
        return label;
    }
    BytecodeGenerator *generator() const {
        return cg->bytecodeGenerator;
    }
};

struct ControlFlowLoop : public ControlFlow
{
    QString loopLabel;
    BytecodeGenerator::Label *breakLabel = 0;
    BytecodeGenerator::Label *continueLabel = 0;

    ControlFlowLoop(Codegen *cg, BytecodeGenerator::Label *breakLabel, BytecodeGenerator::Label *continueLabel = 0)
        : ControlFlow(cg, Loop), loopLabel(ControlFlow::loopLabel()), breakLabel(breakLabel), continueLabel(continueLabel)
    {
    }

    virtual QString label() const { return loopLabel; }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        switch (type) {
        case Break:
            if (breakLabel && (label.isEmpty() || label == loopLabel))
                return { type, loopLabel, *breakLabel, -1, 0 };
            break;
        case Continue:
            if (continueLabel && (label.isEmpty() || label == loopLabel))
                return { type, loopLabel, *continueLabel, -1, 0 };
            break;
        case Return:
        case Throw:
            break;
        case Invalid:
            Q_ASSERT(false);
            Q_UNREACHABLE();
        }
        return ControlFlow::getHandler(type, label);
    }

};

struct ControlFlowUnwind : public ControlFlow
{
    BytecodeGenerator::ExceptionHandler unwindLabel;
    int controlFlowTemp;
    QVector<Handler> handlers;

    ControlFlowUnwind(Codegen *cg, Type type)
        : ControlFlow(cg, type), unwindLabel(generator()->newExceptionHandler())
    {
        Q_ASSERT(type != Loop);
        controlFlowTemp = static_cast<int>(generator()->newTemp());
        Reference::fromTemp(cg, controlFlowTemp).store(Reference::fromConst(cg, QV4::Encode::undefined()));
        // we'll need at least a handler for throw
        getHandler(Throw);
    }

    void emitUnwindHandler()
    {
        Q_ASSERT(!isSimple());

        Reference temp = Reference::fromTemp(cg, controlFlowTemp);
        for (const auto &h : qAsConst(handlers)) {
            Codegen::TempScope tempScope(cg);
            Handler parentHandler = getParentHandler(h.type, h.label);


            if (h.type == Throw || parentHandler.tempIndex >= 0) {
                BytecodeGenerator::Label skip = generator()->newLabel();
                generator()->jumpStrictNotEqual(temp.asRValue(), Reference::fromConst(cg, QV4::Encode(h.value)).asRValue())
                        .link(skip);
                if (h.type == Throw)
                    emitForThrowHandling();
                Reference parentTemp = Reference::fromTemp(cg, parentHandler.tempIndex);
                parentTemp.store(Reference::fromConst(cg, QV4::Encode(parentHandler.value)));
                generator()->jump().link(parentHandler.linkLabel);
                skip.link();
            } else {
                generator()->jumpStrictEqual(temp.asRValue(), Reference::fromConst(cg, QV4::Encode(h.value)).asRValue())
                        .link(parentHandler.linkLabel);
            }
        }
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        for (const auto &h : qAsConst(handlers)) {
            if (h.type == type && h.label == label)
                return h;
        }
        Handler h = {
            type,
            label,
            unwindLabel,
            controlFlowTemp,
            handlers.size()
        };
        handlers.append(h);
        return h;
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return &unwindLabel;
    }

    virtual void emitForThrowHandling() { }
};

struct ControlFlowWith : public ControlFlowUnwind
{
    ControlFlowWith(Codegen *cg)
        : ControlFlowUnwind(cg, With)
    {
        generator()->setExceptionHandler(&unwindLabel);
    }

    virtual ~ControlFlowWith() {
        // emit code for unwinding
        unwindLabel.link();

        generator()->setExceptionHandler(parentExceptionHandler());
        Instruction::CallBuiltinPopScope pop;
        generator()->addInstruction(pop);

        emitUnwindHandler();
    }
};

struct ControlFlowCatch : public ControlFlowUnwind
{
    AST::Catch *catchExpression;
    bool insideCatch = false;
    BytecodeGenerator::ExceptionHandler exceptionLabel;
    BytecodeGenerator::ExceptionHandler catchUnwindLabel;

    ControlFlowCatch(Codegen *cg, AST::Catch *catchExpression)
        : ControlFlowUnwind(cg, Catch), catchExpression(catchExpression),
          exceptionLabel(generator()->newExceptionHandler()),
          catchUnwindLabel(generator()->newExceptionHandler())
    {
        generator()->setExceptionHandler(&exceptionLabel);
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        Handler h = ControlFlowUnwind::getHandler(type, label);
        if (insideCatch)
            // if we're inside the catch block, we need to jump to the pop scope
            // instruction at the end of the catch block, not the unwind handler
            h.linkLabel = catchUnwindLabel;
        else if (type == Throw)
            // if we're inside the try block, we need to jump to the catch block,
            // not the unwind handler
            h.linkLabel = exceptionLabel;
        return h;
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return insideCatch ? &catchUnwindLabel : &exceptionLabel;
    }

    ~ControlFlowCatch() {
        // emit code for unwinding

        ++cg->_function->insideWithOrCatch;
        insideCatch = true;

        // exceptions inside the try block go here
        exceptionLabel.link();
        Reference name = Reference::fromName(cg, catchExpression->name.toString());
        Instruction::CallBuiltinPushCatchScope pushCatchScope;
        pushCatchScope.name = name.nameIndex;
        generator()->addInstruction(pushCatchScope);
        // clear the unwind temp for exceptions, we want to resume normal code flow afterwards
        Reference::fromTemp(cg, controlFlowTemp).store(Reference::fromConst(cg, QV4::Encode::undefined()));
        generator()->setExceptionHandler(&catchUnwindLabel);

        cg->statement(catchExpression->statement);

        --cg->_function->insideWithOrCatch;
        insideCatch = false;

        // exceptions inside catch and break/return statements go here
        catchUnwindLabel.link();
        Instruction::CallBuiltinPopScope pop;
        generator()->addInstruction(pop);

        // break/continue/return statements in try go here
        unwindLabel.link();
        generator()->setExceptionHandler(parentExceptionHandler());

        emitUnwindHandler();
    }
};

struct ControlFlowFinally : public ControlFlowUnwind
{
    AST::Finally *finally;
    bool insideFinally = false;
    int exceptionTemp = -1;

    ControlFlowFinally(Codegen *cg, AST::Finally *finally)
        : ControlFlowUnwind(cg, Finally), finally(finally)
    {
        Q_ASSERT(finally != 0);
        generator()->setExceptionHandler(&unwindLabel);
    }

    virtual Handler getHandler(HandlerType type, const QString &label = QString()) {
        // if we're inside the finally block, any exceptions etc. should
        // go directly to the parent handler
        if (insideFinally)
            return ControlFlow::getHandler(type, label);
        return ControlFlowUnwind::getHandler(type, label);
    }

    virtual BytecodeGenerator::ExceptionHandler *exceptionHandler() {
        return insideFinally ? parentExceptionHandler() : ControlFlowUnwind::exceptionHandler();
    }

    ~ControlFlowFinally() {
        // emit code for unwinding
        unwindLabel.link();

        Codegen::TempScope scope(cg);

        insideFinally = true;
        exceptionTemp = generator()->newTemp();
        Reference exception = Reference::fromTemp(cg, exceptionTemp);
        Instruction::GetException instr;
        instr.result = exception.asLValue();
        generator()->addInstruction(instr);

        generator()->setExceptionHandler(parentExceptionHandler());
        cg->statement(finally->statement);
        insideFinally = false;

        emitUnwindHandler();
    }

    virtual void emitForThrowHandling() {
        // reset the exception flag, that got cleared before executing the statements in finally
        Instruction::SetException setException;
        Q_ASSERT(exceptionTemp != -1);
        setException.exception = Reference::fromTemp(cg, exceptionTemp).asRValue();
        generator()->addInstruction(setException);
    }
};

} // QV4 namespace
QT_END_NAMESPACE

static inline QV4::Runtime::RuntimeMethods aluOpFunction(QSOperator::Op op)
{
    switch (op) {
    case QSOperator::Invalid:
        return QV4::Runtime::InvalidRuntimeMethod;
    case QSOperator::BitAnd:
        return QV4::Runtime::bitAnd;
    case QSOperator::BitOr:
        return QV4::Runtime::bitOr;
    case QSOperator::BitXor:
        return QV4::Runtime::bitXor;
    case QSOperator::Add:
        return QV4::Runtime::InvalidRuntimeMethod;
    case QSOperator::Sub:
        return QV4::Runtime::sub;
    case QSOperator::Mul:
        return QV4::Runtime::mul;
    case QSOperator::Div:
        return QV4::Runtime::div;
    case QSOperator::Mod:
        return QV4::Runtime::mod;
    case QSOperator::LShift:
        return QV4::Runtime::shl;
    case QSOperator::RShift:
        return QV4::Runtime::shr;
    case QSOperator::URShift:
        return QV4::Runtime::ushr;
    case QSOperator::Gt:
        return QV4::Runtime::greaterThan;
    case QSOperator::Lt:
        return QV4::Runtime::lessThan;
    case QSOperator::Ge:
        return QV4::Runtime::greaterEqual;
    case QSOperator::Le:
        return QV4::Runtime::lessEqual;
    case QSOperator::Equal:
        return QV4::Runtime::equal;
    case QSOperator::NotEqual:
        return QV4::Runtime::notEqual;
    case QSOperator::StrictEqual:
        return QV4::Runtime::strictEqual;
    case QSOperator::StrictNotEqual:
        return QV4::Runtime::strictNotEqual;
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
    , _returnAddress(0)
    , _variableEnvironment(0)
    , _controlFlow(0)
    , _labelledStatement(0)
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

Codegen::Reference Codegen::unop(UnaryOperation op, const Reference &expr)
{
    if (hasError)
        return _expr.result;

#ifndef V4_BOOTSTRAP
    if (expr.type == Reference::Const) {
        auto v = Value::fromReturnedValue(expr.constant);
        if (v.isNumber()) {
            switch (op) {
            case Not:
                return Reference::fromConst(this, Runtime::method_uNot(v));
            case UMinus:
                return Reference::fromConst(this, Runtime::method_uMinus(v));
            case UPlus:
                return expr;
            case Compl:
                return Reference::fromConst(this, Runtime::method_complement(v));
            default:
                break;
            }
        }
    }
#endif // V4_BOOTSTRAP

    auto dest = Reference::fromTemp(this);

    switch (op) {
    case UMinus: {
        Instruction::UMinus uminus;
        uminus.source = expr.asRValue();
        uminus.result = dest.asLValue();
        bytecodeGenerator->addInstruction(uminus);
    } break;
    case UPlus: {
        Instruction::UPlus uplus;
        uplus.source = expr.asRValue();
        uplus.result = dest.asLValue();
        bytecodeGenerator->addInstruction(uplus);
    } break;
    case Not: {
        Instruction::UNot unot;
        unot.source = expr.asRValue();
        unot.result = dest.asLValue();
        bytecodeGenerator->addInstruction(unot);
    } break;
    case Compl: {
        Instruction::UCompl ucompl;
        ucompl.source = expr.asRValue();
        ucompl.result = dest.asLValue();
        bytecodeGenerator->addInstruction(ucompl);
    } break;
    case PreIncrement: {
        Instruction::PreIncrement inc;
        inc.source = expr.asRValue();
        inc.result = dest.asLValue();
        bytecodeGenerator->addInstruction(inc);
        expr.store(dest);
    } break;
    case PreDecrement: {
        Instruction::PreDecrement dec;
        dec.source = expr.asRValue();
        dec.result = dest.asLValue();
        bytecodeGenerator->addInstruction(dec);
        expr.store(dest);
    } break;
    case PostIncrement: {
        Instruction::PostIncrement inc;
        inc.source = expr.asRValue();
        inc.result = dest.asLValue();
        bytecodeGenerator->addInstruction(inc);
        expr.asLValue(); // mark expr as needsWriteBack
        expr.writeBack();
    } break;
    case PostDecrement: {
        Instruction::PostDecrement dec;
        dec.source = expr.asRValue();
        dec.result = dest.asLValue();
        bytecodeGenerator->addInstruction(dec);
        expr.asLValue(); // mark expr as needsWriteBack
        expr.writeBack();
    } break;
    }

    return dest;
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
    TempScope scope(this);

    bytecodeGenerator->setLocation(ast->firstSourceLocation());
    accept(ast);
}

void Codegen::statement(ExpressionNode *ast)
{
    TempScope scope(this);

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
//                unsigned t = bytecodeGenerator->newTemp();
//                move(_block->TEMP(t), *r);
//            }
//        }

        if (r.result.isValid() && !r.result.isTempLocalArg())
            r.result.asRValue(); // triggers side effects
    }
}

void Codegen::condition(ExpressionNode *ast, const BytecodeGenerator::Label *iftrue,
                        const BytecodeGenerator::Label *iffalse, bool trueBlockFollowsCondition)
{
    if (ast) {
        Result r(iftrue, iffalse, trueBlockFollowsCondition);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            bytecodeGenerator->setLocation(ast->firstSourceLocation());
            auto cond = r.result.asRValue();
            if (r.trueBlockFollowsCondition)
                bytecodeGenerator->jumpNe(cond).link(*r.iffalse);
            else
                bytecodeGenerator->jumpEq(cond).link(*r.iftrue);
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
    TempScope scope(this);

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

    auto result = Reference::fromTemp(this);
    TempScope scope(this);

    int argc = 0;
    Reference _args[16];
    Reference *args = _args;
    if (_variableEnvironment->maxNumberOfArguments > 16)
        args = new Reference[_variableEnvironment->maxNumberOfArguments];
    auto empty = [this](){ return Reference::fromConst(this, Primitive::emptyValue().asReturnedValue()); };
    auto push = [this, &argc, args](const Reference &arg) {
        args[argc] = arg;
        if (arg.type != Reference::Const)
            args[argc].asRValue();
        argc += 1;
    };

    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next)
            push(empty());

        Reference expr = expression(it->expression);
        if (hasError)
            return false;

        push(expr);
    }
    for (Elision *elision = ast->elision; elision; elision = elision->next)
        push(empty());

    for (int i = 0; i < argc; ++i)
        Reference::fromTemp(this, i).store(args[i]);

    Instruction::CallBuiltinDefineArray call;
    call.argc = argc;
    call.args = 0;
    call.result = result.asLValue();
    bytecodeGenerator->addInstruction(call);
    _expr.result = result;

    if (args != _args)
        delete [] args;
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

static QSOperator::Op baseOp(int op)
{
    switch ((QSOperator::Op) op) {
    case QSOperator::InplaceAnd: return QSOperator::BitAnd;
    case QSOperator::InplaceSub: return QSOperator::Sub;
    case QSOperator::InplaceDiv: return QSOperator::Div;
    case QSOperator::InplaceAdd: return QSOperator::Add;
    case QSOperator::InplaceLeftShift: return QSOperator::LShift;
    case QSOperator::InplaceMod: return QSOperator::Mod;
    case QSOperator::InplaceMul: return QSOperator::Mul;
    case QSOperator::InplaceOr: return QSOperator::BitOr;
    case QSOperator::InplaceRightShift: return QSOperator::RShift;
    case QSOperator::InplaceURightShift: return QSOperator::URShift;
    case QSOperator::InplaceXor: return QSOperator::BitXor;
    default: return QSOperator::Invalid;
    }
}

bool Codegen::visit(BinaryExpression *ast)
{
    if (hasError)
        return false;

    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            auto iftrue = bytecodeGenerator->newLabel();
            condition(ast->left, &iftrue, _expr.iffalse, true);
            iftrue.link();
            condition(ast->right, _expr.iftrue, _expr.iffalse, _expr.trueBlockFollowsCondition);
        } else {
            auto iftrue = bytecodeGenerator->newLabel();
            auto endif = bytecodeGenerator->newLabel();

            auto r = Reference::fromTemp(this);

            Reference lhs = expression(ast->left);
            if (hasError)
                return false;

            r.store(lhs);

            bytecodeGenerator->setLocation(ast->operatorToken);
            bytecodeGenerator->jumpNe(r.asRValue()).link(endif);
            iftrue.link();

            Reference rhs = expression(ast->right);
            if (hasError)
                return false;

            r.store(rhs);
            endif.link();

            _expr.result = r;
        }
        return false;
    } else if (ast->op == QSOperator::Or) {
        if (_expr.accept(cx)) {
            auto iffalse = bytecodeGenerator->newLabel();
            condition(ast->left, _expr.iftrue, &iffalse, false);
            iffalse.link();
            condition(ast->right, _expr.iftrue, _expr.iffalse, _expr.trueBlockFollowsCondition);
        } else {
            auto iffalse = bytecodeGenerator->newLabel();
            auto endif = bytecodeGenerator->newLabel();

            auto r = Reference::fromTemp(this);

            Reference lhs = expression(ast->left);
            if (hasError)
                return false;

            r.store(lhs);

            bytecodeGenerator->setLocation(ast->operatorToken);
            bytecodeGenerator->jumpEq(r.asRValue()).link(endif);
            iffalse.link();

            Reference rhs = expression(ast->right);
            if (hasError)
                return false;

            r.store(rhs);
            endif.link();

            _expr.result = r;
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
        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
            return false;
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
        if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation()))
            return false;

        if (!left.isLValue()) {
            throwSyntaxError(ast->operatorToken, QStringLiteral("left-hand side of inplace operator is not an lvalue"));
            return false;
        }

        Reference right = expression(ast->right);

        if (hasError)
            return false;

        _expr.result = Reference::fromTemp(this);
        binopHelper(baseOp(ast->op), left, right, _expr.result);
        left.store(_expr.result);

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

        Reference right = expression(ast->right);
        if (hasError)
            return false;

        _expr.result = Reference::fromTemp(this);
        binopHelper(static_cast<QSOperator::Op>(ast->op), left, right, _expr.result);

        break;
    }

    } // switch

    return false;
}

QV4::Moth::Param Codegen::binopHelper(QSOperator::Op oper, Reference &left,
                                      Reference &right, Reference &dest)
{
    if (oper == QSOperator::Add) {
        Instruction::Add add;
        add.lhs = left.asRValue();
        add.rhs = right.asRValue();
        add.result = dest.asLValue();
        bytecodeGenerator->addInstruction(add);
        return add.result;
    }
    if (oper == QSOperator::Sub) {
        Instruction::Sub sub;
        sub.lhs = left.asRValue();
        sub.rhs = right.asRValue();
        sub.result = dest.asLValue();
        bytecodeGenerator->addInstruction(sub);
        return sub.result;
    }
    if (oper == QSOperator::Mul) {
        Instruction::Mul mul;
        mul.lhs = left.asRValue();
        mul.rhs = right.asRValue();
        mul.result = dest.asLValue();
        bytecodeGenerator->addInstruction(mul);
        return mul.result;
    }
    if (oper == QSOperator::BitAnd) {
        Reference *l = &left;
        Reference *r = &right;
        if (l->type == Reference::Const)
            std::swap(l, r);
        if (r->type == Reference::Const) {
            Instruction::BitAndConst bitAnd;
            bitAnd.lhs = l->asRValue();
            bitAnd.rhs = Primitive::fromReturnedValue(r->constant).toInt32();
            bitAnd.result = dest.asLValue();
            bytecodeGenerator->addInstruction(bitAnd);
            return bitAnd.result;
        }
        Instruction::BitAnd bitAnd;
        bitAnd.lhs = left.asRValue();
        bitAnd.rhs = right.asRValue();
        bitAnd.result = dest.asLValue();
        bytecodeGenerator->addInstruction(bitAnd);
        return bitAnd.result;
    }
    if (oper == QSOperator::BitOr) {
        Reference *l = &left;
        Reference *r = &right;
        if (l->type == Reference::Const)
            std::swap(l, r);
        if (r->type == Reference::Const) {
            Instruction::BitOrConst bitOr;
            bitOr.lhs = l->asRValue();
            bitOr.rhs = Primitive::fromReturnedValue(r->constant).toInt32();
            bitOr.result = dest.asLValue();
            bytecodeGenerator->addInstruction(bitOr);
            return bitOr.result;
        }
        Instruction::BitOr bitOr;
        bitOr.lhs = left.asRValue();
        bitOr.rhs = right.asRValue();
        bitOr.result = dest.asLValue();
        bytecodeGenerator->addInstruction(bitOr);
        return bitOr.result;
    }
    if (oper == QSOperator::BitXor) {
        Reference *l = &left;
        Reference *r = &right;
        if (l->type == Reference::Const)
            std::swap(l, r);
        if (r->type == Reference::Const) {
            Instruction::BitXorConst bitXor;
            bitXor.lhs = l->asRValue();
            bitXor.rhs = Primitive::fromReturnedValue(r->constant).toInt32();
            bitXor.result = dest.asLValue();
            bytecodeGenerator->addInstruction(bitXor);
            return bitXor.result;
        }
        Instruction::BitXor bitXor;
        bitXor.lhs = left.asRValue();
        bitXor.rhs = right.asRValue();
        bitXor.result = dest.asLValue();
        bytecodeGenerator->addInstruction(bitXor);
        return bitXor.result;
    }
    if (oper == QSOperator::RShift) {
        if (right.type == Reference::Const) {
            Instruction::ShrConst shr;
            shr.lhs = left.asRValue();
            shr.rhs = Primitive::fromReturnedValue(right.constant).toInt32() & 0x1f;
            shr.result = dest.asLValue();
            bytecodeGenerator->addInstruction(shr);
            return shr.result;
        }
        Instruction::Shr shr;
        shr.lhs = left.asRValue();
        shr.rhs = right.asRValue();
        shr.result = dest.asLValue();
        bytecodeGenerator->addInstruction(shr);
        return shr.result;
    }
    if (oper == QSOperator::LShift) {
        if (right.type == Reference::Const) {
            Instruction::ShlConst shl;
            shl.lhs = left.asRValue();
            shl.rhs = Primitive::fromReturnedValue(right.constant).toInt32() & 0x1f;
            shl.result = dest.asLValue();
            bytecodeGenerator->addInstruction(shl);
            return shl.result;
        }
        Instruction::Shl shl;
        shl.lhs = left.asRValue();
        shl.rhs = right.asRValue();
        shl.result = dest.asLValue();
        bytecodeGenerator->addInstruction(shl);
        return shl.result;
    }

    if (oper == QSOperator::InstanceOf || oper == QSOperator::In || oper == QSOperator::Add) {
        Instruction::BinopContext binop;
        if (oper == QSOperator::InstanceOf)
            binop.alu = QV4::Runtime::instanceof;
        else if (oper == QSOperator::In)
            binop.alu = QV4::Runtime::in;
        else
            binop.alu = QV4::Runtime::add;
        binop.lhs = left.asRValue();
        binop.rhs = right.asRValue();
        binop.result = dest.asLValue();
        Q_ASSERT(binop.alu != QV4::Runtime::InvalidRuntimeMethod);
        bytecodeGenerator->addInstruction(binop);
        return binop.result;
    } else {
        auto binopFunc = aluOpFunction(oper);
        Q_ASSERT(binopFunc != QV4::Runtime::InvalidRuntimeMethod);
        Instruction::Binop binop;
        binop.alu = binopFunc;
        binop.lhs = left.asRValue();
        binop.rhs = right.asRValue();
        binop.result = dest.asLValue();
        bytecodeGenerator->addInstruction(binop);
        return binop.result;
    }

    Q_UNIMPLEMENTED();
}

bool Codegen::visit(CallExpression *ast)
{
    if (hasError)
        return false;

    Reference r = Reference::fromTemp(this);

    TempScope scope(this);

    Reference base = expression(ast->base);
    if (hasError)
        return false;

    auto argc = pushArgs(ast->arguments);
    if (hasError)
        return false;

    if (base.type == Reference::Member) {
        Instruction::CallProperty call;
        call.base = base.base;
        call.name = base.nameIndex;
        call.argc = argc;
        call.callData = 0;
        call.result = r.asLValue();
        bytecodeGenerator->addInstruction(call);
    } else if (base.type == Reference::Subscript) {
        Instruction::CallElement call;
        call.base = base.base;
        call.index = base.subscript;
        call.argc = argc;
        call.callData = 0;
        call.result = r.asLValue();
        bytecodeGenerator->addInstruction(call);
    } else if (base.type == Reference::Name) {
        if (useFastLookups && base.global) {
            Instruction::CallGlobalLookup call;
            call.index = registerGlobalGetterLookup(base.nameIndex);
            call.argc = argc;
            call.callData = 0;
            call.result = r.asLValue();
            bytecodeGenerator->addInstruction(call);
        } else {
            Instruction::CallActivationProperty call;
            call.name = base.nameIndex;
            call.argc = argc;
            call.callData = 0;
            call.result = r.asLValue();
            bytecodeGenerator->addInstruction(call);
        }
    } else {
        Instruction::CallValue call;
        call.dest = base.asRValue();
        call.argc = argc;
        call.callData = 0;
        call.result = r.asLValue();
        bytecodeGenerator->addInstruction(call);
    }
    _expr.result = r;
    return false;
}

int Codegen::pushArgs(ArgumentList *args)
{
    int minNrOfStackEntries = offsetof(QV4::CallData, args)/sizeof(QV4::Value);
    int argc = 0;
    Reference _rargs[16];
    Reference *rargs = _rargs;
    if (_variableEnvironment->maxNumberOfArguments > 16)
        rargs = new Reference[_variableEnvironment->maxNumberOfArguments];
    for (ArgumentList *it = args; it; it = it->next) {
        rargs[argc] = expression(it->expression);
        rargs[argc].asRValue();
        if (hasError)
            return -1;
        argc += 1;
    }

    for (int i = 0; i < argc; ++i)
        Reference::fromTemp(this, minNrOfStackEntries + i).store(rargs[i]);

    if (rargs != _rargs)
        delete [] rargs;
    return argc;
}

bool Codegen::visit(ConditionalExpression *ast)
{
    if (hasError)
        return true;

    const unsigned t = bytecodeGenerator->newTemp();
    _expr = Reference::fromTemp(this, t);

    TempScope scope(this);

    Reference r = expression(ast->expression);

    // ### handle const Reference

    BytecodeGenerator::Jump jump_else = bytecodeGenerator->jumpNe(r.asRValue());

    _expr.result.store(expression(ast->ok));

    BytecodeGenerator::Jump jump_endif = bytecodeGenerator->jump();

    jump_else.link();

    _expr.result.store(expression(ast->ko));

    jump_endif.link();

    return false;
}

bool Codegen::visit(DeleteExpression *ast)
{
    if (hasError)
        return false;

    Reference expr = expression(ast->expression);
    if (hasError)
        return false;

    switch (expr.type) {
    case Reference::Local:
    case Reference::Argument:
        // Trying to delete a function argument might throw.
        if (_variableEnvironment->isStrict) {
            throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
            return false;
        }
        _expr.result = Reference::fromConst(this, QV4::Encode(false));
        return false;
    case Reference::Name: {
        if (_variableEnvironment->isStrict) {
            throwSyntaxError(ast->deleteToken, QStringLiteral("Delete of an unqualified identifier in strict mode."));
            return false;
        }
        _expr.result = Reference::fromTemp(this);
        Instruction::CallBuiltinDeleteName del;
        del.name = expr.nameIndex;
        del.result = _expr.result.asLValue();
        bytecodeGenerator->addInstruction(del);
        return false;
    }
    case Reference::Member: {
        _expr.result = Reference::fromTemp(this);
        Instruction::CallBuiltinDeleteMember del;
        del.base = expr.base;
        del.member = expr.nameIndex;
        del.result = _expr.result.asLValue();
        bytecodeGenerator->addInstruction(del);
        return false;
    }
    case Reference::Subscript: {
        _expr.result = Reference::fromTemp(this);
        Instruction::CallBuiltinDeleteSubscript del;
        del.base = expr.base;
        del.index = expr.subscript;
        del.result = _expr.result.asLValue();
        bytecodeGenerator->addInstruction(del);
        return false;
    }
    default:
        // [[11.4.1]] Return true if it's not a reference
        _expr.result = Reference::fromConst(this, QV4::Encode(true));
        return false;
    }
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

    TempScope scope(this);

    int function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    _expr.result = Reference::fromClosure(this, function);
    return false;
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
            Reference r = Reference::fromLocal(this, index, scope);
            if (name == QLatin1String("arguments") || name == QLatin1String("eval")) {
                r.isArgOrEval = true;
                if (isLhs && f->isStrict)
                    // ### add correct source location
                    throwSyntaxError(SourceLocation(), QStringLiteral("Variable name may not be eval or arguments in strict mode"));
            }
            return r;
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

    {
        // This hook allows implementing QML lookup semantics
        Reference fallback = fallbackNameLookup(name);
        if (fallback.type != Reference::Invalid)
            return fallback;
    }

    if (!e->parent && (!f || !f->insideWithOrCatch) &&
        _variableEnvironment->compilationMode != EvalCode && e->compilationMode != QmlBinding) {
        Reference r = Reference::fromName(this, name);
        r.global = true;
        return r;
    }

    // global context or with. Lookup by name
  loadByName:
    return Reference::fromName(this, name);
}

Codegen::Reference Codegen::fallbackNameLookup(const QString &name)
{
    Q_UNUSED(name)
    return Reference();
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

    Reference r = Reference::fromTemp(this);
    TempScope scope(this);

    Reference base = expression(ast->expression);
    if (hasError)
        return false;

    Instruction::CreateValue create;
    create.func = base.asRValue();
    create.argc = 0;
    create.callData = 0;
    create.result = r.asLValue();
    bytecodeGenerator->addInstruction(create);
    _expr.result = r;
    return false;
}

bool Codegen::visit(NewMemberExpression *ast)
{
    if (hasError)
        return false;

    Reference r = Reference::fromTemp(this);
    TempScope scope(this);

    Reference base = expression(ast->base);
    if (hasError)
        return false;

    auto argc = pushArgs(ast->arguments);
    if (hasError)
        return false;

    Instruction::CreateValue create;
    create.func = base.asRValue();
    create.argc = argc;
    create.callData = 0;
    create.result = r.asRValue();
    bytecodeGenerator->addInstruction(create);
    _expr.result = r;
    return false;
}

bool Codegen::visit(NotExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(Not, expression(ast->expression));
    return false;
}

bool Codegen::visit(NullExpression *)
{
    if (hasError)
        return false;

    if (_expr.accept(cx))
        bytecodeGenerator->jump().link(*_expr.iffalse);
    else
        _expr.result = Reference::fromConst(this, Encode::null());

    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromConst(this, QV4::Encode(ast->value));
    return false;
}

bool Codegen::visit(ObjectLiteral *ast)
{
    if (hasError)
        return false;

    QMap<QString, ObjectPropertyValue> valueMap;

    auto result = Reference::fromTemp(this);
    TempScope scope(this);

    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        QString name = it->assignment->name->asString();
        if (PropertyNameAndValue *nv = AST::cast<AST::PropertyNameAndValue *>(it->assignment)) {
            Reference value = expression(nv->value);
            if (hasError)
                return false;

            ObjectPropertyValue &v = valueMap[name];
            if (v.hasGetter() || v.hasSetter() || (_variableEnvironment->isStrict && v.rvalue.isValid())) {
                throwSyntaxError(nv->lastSourceLocation(),
                                 QStringLiteral("Illegal duplicate key '%1' in object literal").arg(name));
                return false;
            }

            v.rvalue = value;
            if (v.rvalue.type != Reference::Const)
                v.rvalue.asRValue();
        } else if (PropertyGetterSetter *gs = AST::cast<AST::PropertyGetterSetter *>(it->assignment)) {
            const int function = defineFunction(name, gs, gs->formals, gs->functionBody ? gs->functionBody->elements : 0);
            ObjectPropertyValue &v = valueMap[name];
            if (v.rvalue.isValid() ||
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

    QVector<QString> nonArrayKey, arrayKeyWithValue, arrayKeyWithGetterSetter;
    bool needSparseArray = false; // set to true if any array index is bigger than 16

    for (QMap<QString, ObjectPropertyValue>::iterator it = valueMap.begin(), eit = valueMap.end();
            it != eit; ++it) {
        QString name = it.key();
        uint keyAsIndex = QV4::String::toArrayIndex(name);
        if (keyAsIndex != std::numeric_limits<uint>::max()) {
            it->keyAsIndex = keyAsIndex;
            if (keyAsIndex > 16)
                needSparseArray = true;
            if (it->hasSetter() || it->hasGetter())
                arrayKeyWithGetterSetter.append(name);
            else
                arrayKeyWithValue.append(name);
        } else {
            nonArrayKey.append(name);
        }
    }

    int argc = 0;
    auto push = [this, &argc](const Reference &arg) {
        Reference::fromTemp(this, argc).store(arg);
        argc += 1;
    };

    auto undefined = [this](){ return Reference::fromConst(this, Encode::undefined()); };
    QVector<QV4::Compiler::JSUnitGenerator::MemberInfo> members;

    // generate the key/value pairs
    for (const QString &key : qAsConst(nonArrayKey)) {
        const ObjectPropertyValue &prop = valueMap[key];

        if (prop.hasGetter() || prop.hasSetter()) {
            Q_ASSERT(!prop.rvalue.isValid());
            push(prop.hasGetter() ? Reference::fromClosure(this, prop.getter) : undefined());
            push(prop.hasSetter() ? Reference::fromClosure(this, prop.setter) : undefined());
            members.append({ key, true });
        } else {
            Q_ASSERT(prop.rvalue.isValid());
            push(prop.rvalue);
            members.append({ key, false });
        }
    }

    // generate array entries with values
    for (const QString &key : qAsConst(arrayKeyWithValue)) {
        const ObjectPropertyValue &prop = valueMap[key];
        Q_ASSERT(!prop.hasGetter() && !prop.hasSetter());
        push(Reference::fromConst(this, Encode(prop.keyAsIndex)));
        push(prop.rvalue);
    }

    // generate array entries with both a value and a setter
    for (const QString &key : qAsConst(arrayKeyWithGetterSetter)) {
        const ObjectPropertyValue &prop = valueMap[key];
        Q_ASSERT(!prop.rvalue.isValid());
        push(Reference::fromConst(this, Encode(prop.keyAsIndex)));
        push(prop.hasGetter() ? Reference::fromClosure(this, prop.getter) : undefined());
        push(prop.hasSetter() ? Reference::fromClosure(this, prop.setter) : undefined());
    }

    int classId = jsUnitGenerator->registerJSClass(members);

    uint arrayGetterSetterCountAndFlags = arrayKeyWithGetterSetter.size();
    arrayGetterSetterCountAndFlags |= needSparseArray << 30;

    Instruction::CallBuiltinDefineObjectLiteral call;
    call.internalClassId = classId;
    call.arrayValueCount = arrayKeyWithValue.size();
    call.arrayGetterSetterCountAndFlags = arrayGetterSetterCountAndFlags;
    call.args = 0;
    call.result = result.asLValue();
    bytecodeGenerator->addInstruction(call);

    _expr.result = result;
    return false;
}

bool Codegen::visit(PostDecrementExpression *ast)
{
    if (hasError)
        return false;

    Reference expr = expression(ast->base);
    if (hasError)
        return false;
    if (!expr.isLValue()) {
        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
        return false;
    }
    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(expr, ast->decrementToken))
        return false;

    _expr.result = unop(PostDecrement, expr);

    return false;
}

bool Codegen::visit(PostIncrementExpression *ast)
{
    if (hasError)
        return false;

    Reference expr = expression(ast->base);
    if (hasError)
        return false;
    if (!expr.isLValue()) {
        throwReferenceError(ast->base->lastSourceLocation(), QStringLiteral("Invalid left-hand side expression in postfix operation"));
        return false;
    }
    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(expr, ast->incrementToken))
        return false;

    _expr.result = unop(PostIncrement, expr);
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

    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(expr, ast->decrementToken))
        return false;
    _expr.result = unop(PreDecrement, expr);
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

    if (throwSyntaxErrorOnEvalOrArgumentsInStrictMode(expr, ast->incrementToken))
        return false;
    _expr.result = unop(PreIncrement, expr);
    return false;
}

bool Codegen::visit(RegExpLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromTemp(this);
    _expr.result.isLiteral = true;

    Instruction::LoadRegExp instr;
    instr.result = _expr.result.asLValue();
    instr.regExpId = jsUnitGenerator->registerRegExp(ast);
    bytecodeGenerator->addInstruction(instr);
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromTemp(this);
    _expr.result.isLiteral = true;

    Instruction::LoadRuntimeString instr;
    instr.result = _expr.result.asLValue();
    instr.stringId = registerString(ast->value.toString());
    bytecodeGenerator->addInstruction(instr);
    return false;
}

bool Codegen::visit(ThisExpression *)
{
    if (hasError)
        return false;

    _expr.result = Reference::fromThis(this);
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(Compl, expression(ast->expression));
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

    _expr.result = Reference::fromTemp(this);

    TempScope scope(this);

    Reference expr = expression(ast->expression);
    if (hasError)
        return false;

    if (expr.type == Reference::Name) {
        // special handling as typeof doesn't throw here
        Instruction::CallBuiltinTypeofName instr;
        instr.name = expr.nameIndex;
        instr.result = _expr.result.asLValue();
        bytecodeGenerator->addInstruction(instr);
    } else {
        Instruction::CallBuiltinTypeofValue instr;
        instr.value = expr.asRValue();
        instr.result = _expr.result.asLValue();
        bytecodeGenerator->addInstruction(instr);
    }

    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(UMinus, expression(ast->expression));
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    if (hasError)
        return false;

    _expr.result = unop(UPlus, expression(ast->expression));
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    if (hasError)
        return false;

    TempScope scope(this);

    statement(ast->expression);
    _expr.result = Reference::fromConst(this, Encode::undefined());
    return false;
}

bool Codegen::visit(FunctionDeclaration * ast)
{
    if (hasError)
        return false;

    TempScope scope(this);

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
    ControlFlow *loop = 0;
    qSwap(_controlFlow, loop);

    enterEnvironment(ast);
    IR::Function *function = _module->newFunction(name, _function);
    int functionIndex = _module->functions.count() - 1;

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

    BytecodeGenerator bytecode(function);
    BytecodeGenerator *savedBytecodeGenerator;
    savedBytecodeGenerator = bytecodeGenerator;
    bytecodeGenerator = &bytecode;

    { // reserve space for outgoing call arguments
        Q_ASSERT(function->tempCount == 0);
        int minNrOfStackEntries = offsetof(QV4::CallData, args)/sizeof(QV4::Value);
        function->tempCount = minNrOfStackEntries + _variableEnvironment->maxNumberOfArguments;
        function->currentTemp = function->tempCount;
    }

    unsigned returnAddress = bytecodeGenerator->newTemp();
    if (function->usesArgumentsObject)
        _variableEnvironment->enter(QStringLiteral("arguments"), Environment::VariableDeclaration, AST::VariableDeclaration::FunctionScope);

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (_variableEnvironment->compilationMode == FunctionCode || _variableEnvironment->compilationMode == QmlBinding) {
        unsigned t = 0;
        Reference undef = Reference::fromConst(this, QV4::Encode::undefined());
        for (Environment::MemberMap::iterator it = _variableEnvironment->members.begin(), end = _variableEnvironment->members.end(); it != end; ++it) {
            const QString &local = it.key();
            function->LOCAL(local);
            (*it).index = t;
            ++t;
        }
    } else {
        if (!_variableEnvironment->isStrict) {
            for (const QString &inheritedLocal : qAsConst(inheritedLocals)) {
                function->LOCAL(inheritedLocal);
                unsigned tempIndex = bytecodeGenerator->newTemp();
                Environment::Member member = { Environment::UndefinedMember,
                                               static_cast<int>(tempIndex), 0,
                                               AST::VariableDeclaration::VariableScope::FunctionScope };
                _variableEnvironment->members.insert(inheritedLocal, member);
            }
        }

        for (Environment::MemberMap::const_iterator it = _variableEnvironment->members.constBegin(), cend = _variableEnvironment->members.constEnd(); it != cend; ++it) {
            const QString &local = it.key();

            Instruction::CallBuiltinDeclareVar declareVar;
            declareVar.isDeletable = false;
            declareVar.varName = registerString(local);
            bytecodeGenerator->addInstruction(declareVar);
        }
    }

    auto exitBlock = bytecodeGenerator->newLabel();

    qSwap(_function, function);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);

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
        Instruction::CallBuiltinSetupArgumentsObject setup;
        setup.result = referenceForName(QStringLiteral("arguments"), false).asLValue();
        bytecodeGenerator->addInstruction(setup);
    }
    if (_function->usesThis && !_variableEnvironment->isStrict) {
        // make sure we convert this to an object
        Instruction::CallBuiltinConvertThisToObject convert;
        bytecodeGenerator->addInstruction(convert);
    }

    beginFunctionBodyHook();

    sourceElements(body);

    _exitBlock.link();
    bytecodeGenerator->setLocation(ast->lastSourceLocation());

    {
        Instruction::Ret ret;
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
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_controlFlow, loop);

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

    TempScope scope(this);

    for (StatementList *it = ast->statements; it; it = it->next) {
        statement(it->statement);
    }
    return false;
}

bool Codegen::visit(BreakStatement *ast)
{
    if (hasError)
        return false;

    if (!_controlFlow) {
        throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Break outside of loop"));
        return false;
    }

    ControlFlow::Handler h = _controlFlow->getHandler(ControlFlow::Break, ast->label.toString());
    if (h.type == ControlFlow::Invalid) {
        if (ast->label.isEmpty())
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Break outside of loop"));
        else
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Undefined label '%1'").arg(ast->label.toString()));
        return false;
    }

    _controlFlow->jumpToHandler(h);

    return false;
}

bool Codegen::visit(ContinueStatement *ast)
{
    if (hasError)
        return false;

    TempScope scope(this);

    if (!_controlFlow) {
        throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Continue outside of loop"));
        return false;
    }

    ControlFlow::Handler h = _controlFlow->getHandler(ControlFlow::Continue, ast->label.toString());
    if (h.type == ControlFlow::Invalid) {
        if (ast->label.isEmpty())
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("Undefined label '%1'").arg(ast->label.toString()));
        else
            throwSyntaxError(ast->lastSourceLocation(), QStringLiteral("continue outside of loop"));
        return false;
    }

    _controlFlow->jumpToHandler(h);

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

    TempScope scope(this);

    BytecodeGenerator::Label body = bytecodeGenerator->label();
    BytecodeGenerator::Label cond = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    ControlFlowLoop flow(this, &end, &cond);

    statement(ast->statement);

    cond.link();
    condition(ast->expression, &body, &end, false);

    end.link();

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

    TempScope scope(this);

    if (_variableEnvironment->compilationMode == EvalCode || _variableEnvironment->compilationMode == QmlBinding) {
        Reference e = expression(ast->expression);
        if (hasError)
            return false;
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

    TempScope scope(this);

    Reference obj = Reference::fromTemp(this);
    Reference expr = expression(ast->expression);
    if (hasError)
        return true;

    Instruction::CallBuiltinForeachIteratorObject iteratorObjInstr;
    iteratorObjInstr.result = obj.asLValue();
    iteratorObjInstr.arg = expr.asRValue();
    bytecodeGenerator->addInstruction(iteratorObjInstr);

    BytecodeGenerator::Label in = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    bytecodeGenerator->jump().link(in);

    ControlFlowLoop flow(this, &end, &in);

    BytecodeGenerator::Label body = bytecodeGenerator->label();

    statement(ast->statement);

    in.link();

    Reference lhs = expression(ast->initialiser);

    Instruction::CallBuiltinForeachNextPropertyName nextPropInstr;
    nextPropInstr.result = lhs.asLValue();
    nextPropInstr.arg = obj.asRValue();
    bytecodeGenerator->addInstruction(nextPropInstr);

    lhs.writeBack();

    Reference null = Reference::fromConst(this, QV4::Encode::null());
    bytecodeGenerator->jumpStrictNotEqual(lhs.asRValue(), null.asRValue()).link(body);

    end.link();

    return false;
}

bool Codegen::visit(ForStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    statement(ast->initialiser);

    BytecodeGenerator::Label cond = bytecodeGenerator->label();
    BytecodeGenerator::Label body = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label step = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    ControlFlowLoop flow(this, &end, &step);

    condition(ast->condition, &body, &end, true);

    body.link();
    statement(ast->statement);

    step.link();
    statement(ast->expression);
    bytecodeGenerator->jump().link(cond);

    end.link();

    return false;
}

bool Codegen::visit(IfStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    BytecodeGenerator::Label trueLabel = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label falseLabel = bytecodeGenerator->newLabel();
    condition(ast->expression, &trueLabel, &falseLabel, true);

    trueLabel.link();
    statement(ast->ok);
    if (ast->ko) {
        BytecodeGenerator::Jump jump_endif = bytecodeGenerator->jump();
        falseLabel.link();
        statement(ast->ko);
        jump_endif.link();
    } else {
        falseLabel.link();
    }

    return false;
}

bool Codegen::visit(LabelledStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    // check that no outer loop contains the label
    ControlFlow *l = _controlFlow;
    while (l) {
        if (l->label() == ast->label) {
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
        BytecodeGenerator::Label breakLabel = bytecodeGenerator->newLabel();
        ControlFlowLoop flow(this, &breakLabel);
        statement(ast->statement);
        breakLabel.link();
    }

    return false;
}

bool Codegen::visit(LocalForEachStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    Reference obj = Reference::fromTemp(this);
    Reference expr = expression(ast->expression);
    if (hasError)
        return true;

    variableDeclaration(ast->declaration);

    Instruction::CallBuiltinForeachIteratorObject iteratorObjInstr;
    iteratorObjInstr.result = obj.asLValue();
    iteratorObjInstr.arg = expr.asRValue();
    bytecodeGenerator->addInstruction(iteratorObjInstr);

    BytecodeGenerator::Label in = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    bytecodeGenerator->jump().link(in);
    ControlFlowLoop flow(this, &end, &in);

    BytecodeGenerator::Label body = bytecodeGenerator->label();

    Reference it = referenceForName(ast->declaration->name.toString(), true);
    statement(ast->statement);

    in.link();

    Instruction::CallBuiltinForeachNextPropertyName nextPropInstr;
    nextPropInstr.result = it.asLValue();
    nextPropInstr.arg = obj.asRValue();
    bytecodeGenerator->addInstruction(nextPropInstr);

    it.writeBack();

    Reference null = Reference::fromConst(this, QV4::Encode::null());
    bytecodeGenerator->jumpStrictNotEqual(it.asRValue(), null.asRValue()).link(body);

    end.link();

    return false;
}

bool Codegen::visit(LocalForStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    variableDeclarationList(ast->declarations);

    BytecodeGenerator::Label cond = bytecodeGenerator->label();
    BytecodeGenerator::Label body = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label step = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();

    ControlFlowLoop flow(this, &end, &step);

    condition(ast->condition, &body, &end, true);

    body.link();
    statement(ast->statement);

    step.link();
    statement(ast->expression);
    bytecodeGenerator->jump().link(cond);
    end.link();

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

    if (_controlFlow) {
        ControlFlow::Handler h = _controlFlow->getHandler(ControlFlow::Return);
        _controlFlow->jumpToHandler(h);
    } else {
        bytecodeGenerator->jump().link(_exitBlock);
    }
    return false;
}

bool Codegen::visit(SwitchStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    if (ast->block) {
        BytecodeGenerator::Label switchEnd = bytecodeGenerator->newLabel();

        Reference lhs = expression(ast->expression);

        // set up labels for all clauses
        QHash<Node *, BytecodeGenerator::Label> blockMap;
        for (CaseClauses *it = ast->block->clauses; it; it = it->next)
            blockMap[it->clause] = bytecodeGenerator->newLabel();
        if (ast->block->defaultClause)
            blockMap[ast->block->defaultClause] = bytecodeGenerator->newLabel();
        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next)
            blockMap[it->clause] = bytecodeGenerator->newLabel();

        // do the switch conditions
        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Reference rhs = expression(clause->expression);
            bytecodeGenerator->jumpStrictEqual(lhs.asRValue(), rhs.asRValue()).link(blockMap.value(clause));
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Reference rhs = expression(clause->expression);
            bytecodeGenerator->jumpStrictEqual(lhs.asRValue(), rhs.asRValue()).link(blockMap.value(clause));
        }

        if (DefaultClause *defaultClause = ast->block->defaultClause)
            bytecodeGenerator->jump().link(blockMap.value(defaultClause));
        else
            bytecodeGenerator->jump().link(switchEnd);

        ControlFlowLoop flow(this, &switchEnd);

        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            blockMap[clause].link();

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);
        }

        if (ast->block->defaultClause) {
            DefaultClause *clause = ast->block->defaultClause;
            blockMap[clause].link();

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            blockMap[clause].link();

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);
        }

        switchEnd.link();

    }

    return false;
}

bool Codegen::visit(ThrowStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    Reference expr = expression(ast->expression);

    if (_controlFlow) {
        _controlFlow->handleThrow(expr);
    } else {
        Instruction::CallBuiltinThrow instr;
        instr.arg = expr.asRValue();
        bytecodeGenerator->addInstruction(instr);
    }
    return false;
}

void Codegen::handleTryCatch(TryStatement *ast)
{
    Q_ASSERT(ast);
    if (_variableEnvironment->isStrict &&
        (ast->catchExpression->name == QLatin1String("eval") || ast->catchExpression->name == QLatin1String("arguments"))) {
        throwSyntaxError(ast->catchExpression->identifierToken, QStringLiteral("Catch variable name may not be eval or arguments in strict mode"));
        return;
    }

    TempScope scope(this);
    BytecodeGenerator::Label noException = bytecodeGenerator->newLabel();
    {
        ControlFlowCatch catchFlow(this, ast->catchExpression);
        TempScope scope(this);
        statement(ast->statement);
        bytecodeGenerator->jump().link(noException);
    }
    noException.link();
}

void Codegen::handleTryFinally(TryStatement *ast)
{
    TempScope scope(this);
    ControlFlowFinally finally(this, ast->finallyExpression);

    if (ast->catchExpression) {
        handleTryCatch(ast);
    } else {
        TempScope scope(this);
        statement(ast->statement);
    }
}

bool Codegen::visit(TryStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    _function->hasTry = true;

    if (ast->finallyExpression && ast->finallyExpression->statement) {
        handleTryFinally(ast);
    } else {
        handleTryCatch(ast);
    }

    return false;
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

    BytecodeGenerator::Label start = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label end = bytecodeGenerator->newLabel();
    BytecodeGenerator::Label cond = bytecodeGenerator->label();
    ControlFlowLoop flow(this, &end, &cond);

    condition(ast->expression, &start, &end, true);

    start.link();
    statement(ast->statement);
    bytecodeGenerator->jump().link(cond);

    end.link();

    return false;
}

bool Codegen::visit(WithStatement *ast)
{
    if (hasError)
        return true;

    TempScope scope(this);

    _function->hasWith = true;

    Reference src = expression(ast->expression);
    if (hasError)
        return false;

    ControlFlowWith flow(this);
    ++_function->insideWithOrCatch;

    Instruction::CallBuiltinPushScope pushScope;
    pushScope.arg = src.asRValue();
    bytecodeGenerator->addInstruction(pushScope);

    statement(ast->statement);
    --_function->insideWithOrCatch;

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

bool Codegen::throwSyntaxErrorOnEvalOrArgumentsInStrictMode(const Reference &r, const SourceLocation& loc)
{
    if (!_variableEnvironment->isStrict)
        return false;
    bool isArgOrEval = false;
    if (r.type == Reference::Name) {
        QString str = jsUnitGenerator->stringForIndex(r.nameIndex);
        if (str == QLatin1String("eval") || str == QLatin1String("arguments")) {
            isArgOrEval = true;
        }
    } else if (r.type == Reference::Local) {
        isArgOrEval = r.isArgOrEval;
    }
    if (isArgOrEval)
        throwSyntaxError(loc, QStringLiteral("Variable name may not be eval or arguments in strict mode"));
    return isArgOrEval;
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

Codegen::Reference::Reference(const Codegen::Reference &other)
{
    *this = other;
}

Codegen::Reference &Codegen::Reference::operator =(const Reference &other)
{
    other.writeBack();

    type = other.type;
    base = other.base;

    switch (type) {
    case Invalid:
        break;
    case Temp:
    case Local:
    case Argument:
        break;
    case Name:
    case Member:
        nameIndex = other.nameIndex;
        break;
    case Subscript:
        subscript = other.subscript;
        break;
    case Const:
        constant = other.constant;
        break;
    case Closure:
        closureId = other.closureId;
    case QmlScopeObject:
    case QmlContextObject:
        closureId = other.qmlIndex;
        break;
    case This:
        break;
    }

    // keep loaded reference
    tempIndex = other.tempIndex;
    needsWriteBack = false;
    isArgOrEval = other.isArgOrEval;
    codegen = other.codegen;
    isLiteral = other.isLiteral;
    global = other.global;
    return *this;
}

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
        break;
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
    case Closure:
        return closureId == other.closureId;
    case QmlScopeObject:
    case QmlContextObject:
        return qmlIndex == other.qmlIndex;
    case This:
        return true;
    }
    return true;
}

void Codegen::Reference::storeConsume(Reference &r) const
{
    if (*this == r)
        return;

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
    Q_ASSERT(!needsWriteBack);

    if (*this == r)
        return;

    Moth::Param b = base;
    if (!isSimple()) {
        if (tempIndex < 0)
            tempIndex = codegen->bytecodeGenerator->newTemp();
        if (!r.isSimple() && r.tempIndex == -1) {
            r.load(tempIndex);
            needsWriteBack = true;
            return;
        }
        b = Moth::Param::createTemp(tempIndex);
        needsWriteBack = true;
    }

    if (r.type == Const) {
        Instruction::MoveConst move;
        move.source = r.constant;
        move.result = b;
        codegen->bytecodeGenerator->addInstruction(move);
        return;
    }
    Moth::Param x = r.asRValue();
    Q_ASSERT(b != x);
    Instruction::Move move;
    move.source = x;
    move.result = b;
    codegen->bytecodeGenerator->addInstruction(move);
}

Moth::Param Codegen::Reference::asRValue() const
{
    Q_ASSERT(!needsWriteBack);

    Q_ASSERT(type != Invalid);
    if (type <= Argument)
        return base;

    // need a temp to hold the value
    if (tempIndex >= 0)
        return Moth::Param::createTemp(tempIndex);
    tempIndex = codegen->bytecodeGenerator->newTemp();
    load(tempIndex);
    return Moth::Param::createTemp(tempIndex);
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
        Instruction::StoreName store;
        store.source = temp;
        store.name = nameIndex;
        codegen->bytecodeGenerator->addInstruction(store);
    } else if (type == Member) {
        if (codegen->useFastLookups) {
            Instruction::SetLookup store;
            store.base = base;
            store.index = codegen->registerSetterLookup(nameIndex);
            store.source = temp;
            codegen->bytecodeGenerator->addInstruction(store);
        } else {
            Instruction::StoreProperty store;
            store.base = base;
            store.name = nameIndex;
            store.source = temp;
            codegen->bytecodeGenerator->addInstruction(store);
        }
    } else if (type == Subscript) {
        Instruction::StoreElement store;
        store.base = base;
        store.index = subscript;
        store.source = temp;
        codegen->bytecodeGenerator->addInstruction(store);
    } else if (type == QmlScopeObject) {
        Instruction::StoreScopeObjectProperty store;
        store.base = base;
        store.propertyIndex = qmlIndex;
        store.source = temp;
        codegen->bytecodeGenerator->addInstruction(store);
    } else if (type == QmlContextObject) {
        Instruction::StoreContextObjectProperty store;
        store.base = base;
        store.propertyIndex = qmlIndex;
        store.source = temp;
        codegen->bytecodeGenerator->addInstruction(store);
    } else {
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }
}

void Codegen::Reference::load(uint tmp) const
{
    Moth::Param temp = Moth::Param::createTemp(tmp);
    if (type == Const) {
        Instruction::MoveConst move;
        move.source = constant;
        move.result = temp;
        codegen->bytecodeGenerator->addInstruction(move);
    } else if (type == Name) {
        if (codegen->useFastLookups && global) {
            Instruction::GetGlobalLookup load;
            load.index = codegen->registerGlobalGetterLookup(nameIndex);
            load.result = temp;
            codegen->bytecodeGenerator->addInstruction(load);
        } else {
            Instruction::LoadName load;
            load.name = nameIndex;
            load.result = temp;
            codegen->bytecodeGenerator->addInstruction(load);
        }
    } else if (type == Member) {
        if (codegen->useFastLookups) {
            Instruction::GetLookup load;
            load.base = base;
            load.index = codegen->registerGetterLookup(nameIndex);
            load.result = temp;
            codegen->bytecodeGenerator->addInstruction(load);
        } else {
            Instruction::LoadProperty load;
            load.base = base;
            load.name = nameIndex;
            load.result = temp;
            codegen->bytecodeGenerator->addInstruction(load);
        }
    } else if (type == Subscript) {
        Instruction::LoadElement load;
        load.base = base;
        load.index = subscript;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == Closure) {
        Instruction::LoadClosure load;
        load.value = closureId;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == QmlScopeObject) {
        Instruction::LoadScopeObjectProperty load;
        load.base = base;
        load.propertyIndex = qmlIndex;
        load.captureRequired = true; // ### captureRequired;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == QmlContextObject) {
        Instruction::LoadContextObjectProperty load;
        load.base = base;
        load.propertyIndex = qmlIndex;
        load.captureRequired = true; // ### captureRequired;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else if (type == This) {
        Instruction::LoadThis load;
        load.result = temp;
        codegen->bytecodeGenerator->addInstruction(load);
    } else {
        Q_ASSERT(false);
        Q_UNREACHABLE();
    }
}
