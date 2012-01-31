/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativerewrite_p.h"

#include "qdeclarativeglobal_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(rewriteDump, QML_REWRITE_DUMP);

namespace QDeclarativeRewrite {

bool SharedBindingTester::isSharable(const QString &code)
{
    Engine engine;
    Lexer lexer(&engine);
    Parser parser(&engine);
    lexer.setCode(code, 0);
    parser.parseStatement();
    if (!parser.statement()) 
        return false;

    return isSharable(parser.statement());
}

bool SharedBindingTester::isSharable(AST::Node *node)
{
    _sharable = true;
    AST::Node::acceptChild(node, this);
    return _sharable;
}

QString RewriteBinding::operator()(const QString &code, bool *ok, bool *sharable)
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
        if (sharable) {
            SharedBindingTester tester;
            *sharable = tester.isSharable(parser.statement());
        }
    }
    return rewrite(code, 0, parser.statement());
}

QString RewriteBinding::operator()(QDeclarativeJS::AST::Node *node, const QString &code, bool *sharable)
{
    if (!node)
        return code;

    if (sharable) {
        SharedBindingTester tester;
        *sharable = tester.isSharable(node);
    }

    QDeclarativeJS::AST::ExpressionNode *expression = node->expressionCast();
    QDeclarativeJS::AST::Statement *statement = node->statementCast();
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
    /* When rewriting the code for bindings, we have to remove ILLEGAL JS tokens like newlines.
       They're still in multi-line strings, because the QML parser allows them, but we have to
       rewrite them to be JS compliant.

       For performance reasons, we don't go for total correctness. \r is only replaced if a
       \n was found (since most line endings are \n or \r\n) and QChar::LineSeparator is not
       even considered. QTBUG-24064.

       Note that rewriteSignalHandler has a function just like this one, for the same reason.
    */

    unsigned startOfString = ast->firstSourceLocation().begin() + 1 - _position;
    unsigned stringLength = ast->firstSourceLocation().length - 2;

    int lastIndex = -1;
    bool foundNewLine = false;
    QStringRef subStr(_code, startOfString, stringLength);
    while (true) {
        lastIndex = subStr.indexOf(QLatin1Char('\n'), lastIndex + 1);
        if (lastIndex == -1)
            break;
        foundNewLine = true;
        _writer->replace(startOfString+lastIndex, 1, QLatin1String("\\n"));
    }

    if (foundNewLine) {
        while (true) {
            lastIndex = subStr.indexOf(QLatin1Char('\r'), lastIndex + 1);
            if (lastIndex == -1)
                break;
            _writer->replace(startOfString+lastIndex, 1, QLatin1String("\\r"));
        }
    }

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

void RewriteSignalHandler::accept(AST::Node *node)
{
    AST::Node::acceptChild(node, this);
}

bool RewriteSignalHandler::visit(AST::StringLiteral *ast)
{
    unsigned startOfExpressionStatement = ast->firstSourceLocation().begin() - _position;
    _strStarts << startOfExpressionStatement + 1;
    _strLens << ast->firstSourceLocation().length - 2;

    return false;
}

void RewriteSignalHandler::rewriteMultilineStrings(QString &code)
{
    QList<int> replaceR, replaceN;
    for (int i=0; i < _strStarts.count(); i++) {
        QStringRef curSubstr = QStringRef(&code, _strStarts[i], _strLens[i]);
        int lastIndex = -1;
        while (true) {
            lastIndex = curSubstr.indexOf(QLatin1Char('\n'), lastIndex + 1);
            if (lastIndex == -1)
                break;
            replaceN << _strStarts[i]+lastIndex;
        }

        if (replaceN.count()) {
            while (true) {
                lastIndex = curSubstr.indexOf(QLatin1Char('\r'), lastIndex + 1);
                if (lastIndex == -1)
                    break;
                replaceR << _strStarts[i]+lastIndex;
            }
        }
    }
    for (int ii = replaceN.count() - 1; ii >= 0; ii--)
        code.replace(replaceN[ii], 1, QLatin1String("\\n"));
    if (replaceR.count())
        for (int ii = replaceR.count() - 1; ii >= 0; ii--)
            code.replace(replaceR[ii], 1, QLatin1String("\\r"));
}

QString RewriteSignalHandler::operator()(QDeclarativeJS::AST::Node *node, const QString &code, const QString &name)
{
    if (rewriteDump()) {
        qWarning() << "=============================================================";
        qWarning() << "Rewrote:";
        qWarning() << qPrintable(code);
    }

    QDeclarativeJS::AST::ExpressionNode *expression = node->expressionCast();
    QDeclarativeJS::AST::Statement *statement = node->statementCast();
    if (!expression && !statement)
        return code;

    _strStarts.clear();
    _strLens.clear();
    _position = expression ? expression->firstSourceLocation().begin() : statement->firstSourceLocation().begin();
    accept(node);

    QString rewritten = code;
    rewriteMultilineStrings(rewritten);

    rewritten = QStringLiteral("(function ") + name + QStringLiteral("() { ") + rewritten + QStringLiteral(" })");

    if (rewriteDump()) {
        qWarning() << "To:";
        qWarning() << qPrintable(rewritten);
        qWarning() << "=============================================================";
    }

    return rewritten;
}

} // namespace QDeclarativeRewrite

QT_END_NAMESPACE
