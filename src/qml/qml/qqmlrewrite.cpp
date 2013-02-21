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

#include "qqmlrewrite_p.h"

#include <private/qqmlglobal_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(rewriteDump, QML_REWRITE_DUMP)

namespace QQmlRewrite {

static void rewriteStringLiteral(AST::StringLiteral *ast, const QString *code, int startPosition, TextWriter *writer)
{
    const unsigned position = ast->firstSourceLocation().begin() - startPosition + 1;
    const unsigned length = ast->literalToken.length - 2;
    const QStringRef spell = code->midRef(position, length);
    const int end = spell.size();
    int index = 0;

    while (index < end) {
        const QChar ch = spell.at(index++);

        if (index < end && ch == QLatin1Char('\\')) {
            int pos = index;

            // skip a possibly empty sequence of \r characters
            while (pos < end && spell.at(pos) == QLatin1Char('\r'))
                ++pos;

            if (pos < end && spell.at(pos) == QLatin1Char('\n')) {
                // This is a `\' followed by a newline terminator.
                // In this case there's nothing to replace. We keep the code
                // as it is and we resume the searching.
                index = pos + 1; // refresh the index
            }
        } else if (ch == QLatin1Char('\r') || ch == QLatin1Char('\n')) {
            const QString sep = ch == QLatin1Char('\r') ? QLatin1String("\\r") : QLatin1String("\\n");
            const int pos = index - 1;
            QString s = sep;

            while (index < end && spell.at(index) == ch) {
                s += sep;
                ++index;
            }

            writer->replace(position + pos, index - pos, s);
        }
    }
}

SharedBindingTester::SharedBindingTester()
: _sharable(false), _safe(false)
{
}

void SharedBindingTester::parse(const QString &code)
{
    _sharable = _safe = false;

    Engine engine;
    Lexer lexer(&engine);
    Parser parser(&engine);
    lexer.setCode(code, 0);
    parser.parseStatement();
    if (!parser.statement()) 
        return;

    return parse(parser.statement());
}

void SharedBindingTester::parse(AST::Node *node)
{
    _sharable = true;
    _safe = true;

    AST::Node::acceptChild(node, this);
}

bool SharedBindingTester::visit(AST::FunctionDeclaration *)
{
    _sharable = false;
    return false;
}

bool SharedBindingTester::visit(AST::FunctionExpression *)
{
    _sharable = false;
    return false;
}

bool SharedBindingTester::visit(AST::CallExpression *e)
{
    static const QString mathString = QStringLiteral("Math");

    if (AST::IdentifierExpression *ie = AST::cast<AST::IdentifierExpression *>(e->base)) {
        if (ie->name == mathString)
            return true;
    }

    _safe = false;
    return true;
}

bool SharedBindingTester::visit(AST::IdentifierExpression *e)
{
    static const QString evalString = QStringLiteral("eval");
    if (e->name == evalString)
        _sharable = false;

    return false; // IdentifierExpression is a leaf node anyway
}

bool SharedBindingTester::visit(AST::PostDecrementExpression *)
{
    _safe = false;
    return true;
}

bool SharedBindingTester::visit(AST::PostIncrementExpression *)
{
    _safe = false;
    return true;
}

bool SharedBindingTester::visit(AST::PreDecrementExpression *)
{
    _safe = false;
    return true;
}

bool SharedBindingTester::visit(AST::PreIncrementExpression *)
{
    _safe = false;
    return true;
}

bool SharedBindingTester::visit(AST::BinaryExpression *e)
{
    if (e->op == QSOperator::InplaceAnd ||
        e->op == QSOperator::Assign ||
        e->op == QSOperator::InplaceSub ||
        e->op == QSOperator::InplaceDiv ||
        e->op == QSOperator::InplaceAdd ||
        e->op == QSOperator::InplaceLeftShift ||
        e->op == QSOperator::InplaceMod ||
        e->op == QSOperator::InplaceMul ||
        e->op == QSOperator::InplaceOr ||
        e->op == QSOperator::InplaceRightShift ||
        e->op == QSOperator::InplaceURightShift ||
        e->op == QSOperator::InplaceXor)
        _safe = false;

    return true;
}

QString RewriteBinding::operator()(const QString &code, bool *ok, bool *sharable, bool *safe)
{
    Engine engine;
    Lexer lexer(&engine);
    Parser parser(&engine);
    lexer.setCode(code, 0);
    parser.parseStatement();
    if (!parser.statement()) {
        if (ok) *ok = false;
        return QString();
    } else {
        if (ok) *ok = true;
        if (sharable || safe) {
            SharedBindingTester tester;
            tester.parse(parser.statement());
            if (sharable) *sharable = tester.isSharable();
            if (safe) *safe = tester.isSafe();
        }
    }
    return rewrite(code, 0, parser.statement());
}

QString RewriteBinding::operator()(QQmlJS::AST::Node *node, const QString &code, bool *sharable, bool *safe)
{
    if (!node)
        return code;

    if (sharable || safe) {
        SharedBindingTester tester;
        tester.parse(node);
        if (sharable) *sharable = tester.isSharable();
        if (safe) *safe = tester.isSafe();
    }

    QQmlJS::AST::ExpressionNode *expression = node->expressionCast();
    QQmlJS::AST::Statement *statement = node->statementCast();
    if(!expression && !statement)
        return code;

    TextWriter w;
    _writer = &w;
    _position = expression ? expression->firstSourceLocation().begin() : statement->firstSourceLocation().begin();
    _inLoop = 0;
    _code = &code;

    accept(node);

    unsigned startOfStatement = 0;
    unsigned endOfStatement = (expression ? expression->lastSourceLocation().end() : statement->lastSourceLocation().end()) - _position;

    QString startString = QLatin1String("(function ") + _name + QLatin1String("() { ");
    if (expression)
        startString += QLatin1String("return ");
    _writer->replace(startOfStatement, 0, startString);
    _writer->replace(endOfStatement, 0, QLatin1String(" })"));

    if (rewriteDump()) {
        qWarning() << "=============================================================";
        qWarning() << "Rewrote:";
        qWarning() << qPrintable(code);
    }

    QString codeCopy = code;
    w.write(&codeCopy);

    if (rewriteDump()) {
        qWarning() << "To:";
        qWarning() << qPrintable(codeCopy);
        qWarning() << "=============================================================";
    }

    return codeCopy;
}

void RewriteBinding::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

QString RewriteBinding::rewrite(QString code, unsigned position,
                                AST::Statement *node)
{
    TextWriter w;
    _writer = &w;
    _position = position;
    _inLoop = 0;
    _code = &code;

    accept(node);

    unsigned startOfStatement = node->firstSourceLocation().begin() - _position;
    unsigned endOfStatement = node->lastSourceLocation().end() - _position;

    _writer->replace(startOfStatement, 0, QLatin1String("(function ") + _name + QLatin1String("() { "));
    _writer->replace(endOfStatement, 0, QLatin1String(" })"));

    if (rewriteDump()) {
        qWarning() << "=============================================================";
        qWarning() << "Rewrote:";
        qWarning() << qPrintable(code);
    }

    w.write(&code);

    if (rewriteDump()) {
        qWarning() << "To:";
        qWarning() << qPrintable(code);
        qWarning() << "=============================================================";
    }

    return code;
}

bool RewriteBinding::visit(AST::Block *ast)
{
    for (AST::StatementList *it = ast->statements; it; it = it->next) {
        if (! it->next) {
            // we need to rewrite only the last statement of a block.
            accept(it->statement);
        }
    }

    return false;
}

bool RewriteBinding::visit(AST::ExpressionStatement *ast)
{
    if (! _inLoop) {
        unsigned startOfExpressionStatement = ast->firstSourceLocation().begin() - _position;
        _writer->replace(startOfExpressionStatement, 0, QLatin1String("return "));
    }

    return false;
}

bool RewriteBinding::visit(AST::StringLiteral *ast)
{
    rewriteStringLiteral(ast, _code, _position, _writer);
    return false;
}

bool RewriteBinding::visit(AST::DoWhileStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::DoWhileStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::WhileStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::WhileStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::ForStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::ForStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::LocalForStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::LocalForStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::ForEachStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::ForEachStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::LocalForEachStatement *)
{
    ++_inLoop;
    return true;
}

void RewriteBinding::endVisit(AST::LocalForEachStatement *)
{
    --_inLoop;
}

bool RewriteBinding::visit(AST::FunctionExpression *)
{
    return false;
}

bool RewriteBinding::visit(AST::FunctionDeclaration *)
{
    return false;
}

bool RewriteBinding::visit(AST::CaseBlock *ast)
{
    // Process the initial sequence of the case clauses.
    for (AST::CaseClauses *it = ast->clauses; it; it = it->next) {
        // Return the value of the last statement in the block, if this is the last `case clause'
        // of the switch statement.
        bool returnTheValueOfLastStatement = (it->next == 0) && (ast->defaultClause == 0) && (ast->moreClauses == 0);

        if (AST::CaseClause *clause = it->clause) {
            accept(clause->expression);
            rewriteCaseStatements(clause->statements, returnTheValueOfLastStatement);
        }
    }

    // Process the default case clause
    if (ast->defaultClause) {
        // Return the value of the last statement in the block, if this is the last `case clause'
        // of the switch statement.
        bool rewriteTheLastStatement = (ast->moreClauses == 0);

        rewriteCaseStatements(ast->defaultClause->statements, rewriteTheLastStatement);
    }

    // Process trailing `case clauses'
    for (AST::CaseClauses *it = ast->moreClauses; it; it = it->next) {
        // Return the value of the last statement in the block, if this is the last `case clause'
        // of the switch statement.
        bool returnTheValueOfLastStatement = (it->next == 0);

        if (AST::CaseClause *clause = it->clause) {
            accept(clause->expression);
            rewriteCaseStatements(clause->statements, returnTheValueOfLastStatement);
        }
    }

    return false;
}

void RewriteBinding::rewriteCaseStatements(AST::StatementList *statements, bool rewriteTheLastStatement)
{
    for (AST::StatementList *it = statements; it; it = it->next) {
        if (it->next && AST::cast<AST::BreakStatement *>(it->next->statement) != 0) {
            // The value of the first statement followed by a `break'.
            accept(it->statement);
            break;
        } else if (!it->next) {
            if (rewriteTheLastStatement)
                accept(it->statement);
            else if (AST::Block *block = AST::cast<AST::Block *>(it->statement))
                rewriteCaseStatements(block->statements, rewriteTheLastStatement);
        }
    }
}


/*
    RewriteSignalHandler performs two different types of rewrites, depending on what information
    is available.

    When the target object is known, the rewriter can be provided a list of parameter names (and an
    optional preconstructed parameter string), which allows us to:
        1. Check whether the parameters are used
        2. Rewrite with the parameters included in the rewrite
    When this information is not available, we do a more generic rewrite, and rely on the expression
    to perform a second rewrite with the parameter information (using createParameterString)
    once the target object is known.
*/
RewriteSignalHandler::RewriteSignalHandler()
    : _writer(0)
    , _code(0)
    , _position(0)
    , _parameterAccess(UnknownAccess)
    , _parameterCountForJS(0)
{
}

void RewriteSignalHandler::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

bool RewriteSignalHandler::visit(AST::StringLiteral *ast)
{
    rewriteStringLiteral(ast, _code, _position, _writer);
    return false;
}

//if we never make use of the signal parameters in our expression,
//there is no need to provide them
bool RewriteSignalHandler::visit(AST::IdentifierExpression *e)
{
    //optimization: don't need to compare strings if a parameter has already been marked as used.
    if (_parameterAccess == ParametersAccessed)
        return false;

    static const QString argumentsString = QStringLiteral("arguments");
    if (_parameterNames.contains(e->name) || e->name == argumentsString)
        _parameterAccess = ParametersAccessed;
    return false;
}

static inline QString msgUnnamedErrorString()
{
    return QCoreApplication::translate("QQmlRewrite", "Signal uses unnamed parameter followed by named parameter.");
}

static inline QString msgGlobalErrorString(const QString &p)
{
    return QCoreApplication::translate("QQmlRewrite", "Signal parameter \"%1\" hides global variable.").arg(p);
}

#define EXIT_ON_ERROR(error) \
{ \
    _error = error; \
    return QString(); \
}

//create a parameter string which can be inserted into a generic rewrite
QString RewriteSignalHandler::createParameterString(const QList<QByteArray> &parameterNameList,
                                                    const QStringHash<bool> &illegalNames)
{
    QList<QHashedString> hashedParameterNameList;
    for (int i = 0; i < parameterNameList.count(); ++i)
        hashedParameterNameList.append(QString::fromUtf8(parameterNameList.at(i).constData()));

    return createParameterString(hashedParameterNameList, illegalNames);
}

QString RewriteSignalHandler::createParameterString(const QList<QHashedString> &parameterNameList,
                                                    const QStringHash<bool> &illegalNames)
{
    QString parameters;
    bool unnamedParam = false;
    for (int i = 0; i < parameterNameList.count(); ++i) {
        const QHashedString &param = parameterNameList.at(i);
        if (param.isEmpty())
            unnamedParam = true;
        else if (unnamedParam)
            EXIT_ON_ERROR(msgUnnamedErrorString())
        else if (illegalNames.contains(param))
            EXIT_ON_ERROR(msgGlobalErrorString(param))
        ++_parameterCountForJS;
        parameters += param;
        if (i < parameterNameList.count()-1)
            parameters += QStringLiteral(",");
    }
    if (parameters.endsWith(QLatin1Char(',')))
        parameters.resize(parameters.length() - 1);
    return parameters;
}

/*
    If \a parameterString is provided, use \a parameterNameList to test whether the
    parameters are used in the body of the function
      * if unused, the rewrite will not include parameters, else
      * if used, the rewrite will use \a parameterString
    If \a parameterString is not provided, it is constructed from \a parameterNameList
    as needed.
*/
QString RewriteSignalHandler::operator()(QQmlJS::AST::Node *node, const QString &code, const QString &name,
                                         const QString &parameterString,
                                         const QList<QByteArray> &parameterNameList,
                                         const QStringHash<bool> &illegalNames)
{
    if (rewriteDump()) {
        qWarning() << "=============================================================";
        qWarning() << "Rewrote:";
        qWarning() << qPrintable(code);
    }

    bool hasParameterString = !parameterString.isEmpty();

    QQmlJS::AST::ExpressionNode *expression = node->expressionCast();
    QQmlJS::AST::Statement *statement = node->statementCast();
    if (!expression && !statement)
        return code;

    if (!parameterNameList.isEmpty()) {
        for (int i = 0; i < parameterNameList.count(); ++i) {
            QHashedString param(QString::fromUtf8(parameterNameList.at(i).constData()));
            _parameterNames.insert(param, i);
            if (!hasParameterString)
                _parameterNameList.append(param);
        }

        //this is set to Unaccessed here, and will be set to Accessed
        //if we detect that a parameter has been used
        _parameterAccess = ParametersUnaccessed;
    }

    TextWriter w;
    _writer = &w;
    _code = &code;

    _position = expression ? expression->firstSourceLocation().begin() : statement->firstSourceLocation().begin();
    accept(node);

    QString rewritten = code;
    w.write(&rewritten);

    QString parameters = (_parameterAccess == ParametersUnaccessed) ? QString()
                                                                    : hasParameterString ? parameterString
                                                                                         : createParameterString(_parameterNameList, illegalNames);
    rewritten = QStringLiteral("(function ") + name + QStringLiteral("(") + parameters + QStringLiteral(") { ") + rewritten + QStringLiteral(" })");

    if (rewriteDump()) {
        qWarning() << "To:";
        qWarning() << qPrintable(rewritten);
        qWarning() << "=============================================================";
    }

    return rewritten;
}

QString RewriteSignalHandler::operator()(const QString &code, const QString &name, bool *ok,
                                         const QList<QByteArray> &parameterNameList,
                                         const QStringHash<bool> &illegalNames)
{
    Engine engine;
    Lexer lexer(&engine);
    Parser parser(&engine);
    lexer.setCode(code, 0);
    parser.parseStatement();
    if (!parser.statement()) {
        if (ok) *ok = false;
        return QString();
    }
    if (ok) *ok = true;
    return operator()(parser.statement(), code, name, QString(), parameterNameList, illegalNames);
}

} // namespace QQmlRewrite

QT_END_NAMESPACE
