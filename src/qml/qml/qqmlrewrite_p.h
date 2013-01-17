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

#ifndef QQMLREWRITE_P_H
#define QQMLREWRITE_P_H

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

#include <private/textwriter_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsmemorypool_p.h>
#include <private/qhashedstring_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlRewrite {
using namespace QQmlJS;

class SharedBindingTester : protected AST::Visitor
{
    bool _sharable;
    bool _safe;
public:
    SharedBindingTester();

    bool isSharable() const { return _sharable; }
    bool isSafe() const { return isSharable() && _safe; }

    void parse(const QString &code);
    void parse(AST::Node *Node);
    
    virtual bool visit(AST::FunctionDeclaration *);
    virtual bool visit(AST::FunctionExpression *);
    virtual bool visit(AST::IdentifierExpression *);
    virtual bool visit(AST::CallExpression *);
    virtual bool visit(AST::PostDecrementExpression *);
    virtual bool visit(AST::PostIncrementExpression *);
    virtual bool visit(AST::PreDecrementExpression *);
    virtual bool visit(AST::PreIncrementExpression *);
    virtual bool visit(AST::BinaryExpression *);
};

class RewriteBinding: protected AST::Visitor
{
    unsigned _position;
    TextWriter *_writer;
    QString _name;
    const QString *_code;

public:
    QString operator()(const QString &code, bool *ok = 0, bool *sharable = 0, bool *safe = 0);
    QString operator()(QQmlJS::AST::Node *node, const QString &code, bool *sharable = 0, bool *safe = 0);

    //name of the function:  used for the debugger
    void setName(const QString &name) { _name = name; }

protected:
    using AST::Visitor::visit;

    void accept(AST::Node *node);
    QString rewrite(QString code, unsigned position, AST::Statement *node);
    void rewriteCaseStatements(AST::StatementList *statements, bool rewriteTheLastStatement);

    virtual bool visit(AST::StringLiteral *ast);
    virtual bool visit(AST::Block *ast);
    virtual bool visit(AST::ExpressionStatement *ast);

    virtual bool visit(AST::DoWhileStatement *ast);
    virtual void endVisit(AST::DoWhileStatement *ast);

    virtual bool visit(AST::WhileStatement *ast);
    virtual void endVisit(AST::WhileStatement *ast);

    virtual bool visit(AST::ForStatement *ast);
    virtual void endVisit(AST::ForStatement *ast);

    virtual bool visit(AST::LocalForStatement *ast);
    virtual void endVisit(AST::LocalForStatement *ast);

    virtual bool visit(AST::ForEachStatement *ast);
    virtual void endVisit(AST::ForEachStatement *ast);

    virtual bool visit(AST::LocalForEachStatement *ast);
    virtual void endVisit(AST::LocalForEachStatement *ast);

    virtual bool visit(AST::CaseBlock *ast);

    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::FunctionDeclaration *ast);

private:
    int _inLoop;
};

class RewriteSignalHandler: protected AST::Visitor
{
public:
    RewriteSignalHandler();
    QString operator()(QQmlJS::AST::Node *node, const QString &code, const QString &name,
                       const QString &parameterString = QString(),
                       const QList<QByteArray> &parameterNameList = QList<QByteArray>(),
                       const QStringHash<bool> &illegalNames = QStringHash<bool>());
    QString operator()(const QString &code, const QString &name, bool *ok = 0,
                       const QList<QByteArray> &parameterNameList = QList<QByteArray>(),
                       const QStringHash<bool> &illegalNames = QStringHash<bool>());

    enum ParameterAccess {
        ParametersAccessed,
        ParametersUnaccessed,
        UnknownAccess
    };

    //returns the first n signal parameters that are used in the expression
    int parameterCountForJS() const { return _parameterCountForJS; }
    ParameterAccess parameterAccess() const { return _parameterAccess; }
    QString createParameterString(const QList<QByteArray> &parameterNameList,
                                  const QStringHash<bool> &illegalNames);

    bool hasParameterError() { return !_error.isEmpty(); }
    QString parameterError() const { return _error; }

protected:
    void rewriteMultilineStrings(QString &code);

    using AST::Visitor::visit;
    void accept(AST::Node *node);
    virtual bool visit(AST::StringLiteral *ast);
    virtual bool visit(AST::IdentifierExpression *);

private:
    QString createParameterString(const QList<QHashedString> &parameterNameList,
                                  const QStringHash<bool> &illegalNames);

    TextWriter *_writer;
    const QString *_code;
    int _position;
    QStringHash<int> _parameterNames;
    QList<QHashedString> _parameterNameList;
    ParameterAccess _parameterAccess;
    int _parameterCountForJS;
    QString _error;
};

} // namespace QQmlRewrite

QT_END_NAMESPACE

#endif // QQMLREWRITE_P_H

