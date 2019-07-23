/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SCOPETREE_H
#define SCOPETREE_H

#include "fakemetaobject.h"
#include "private/qqmljsast_p.h"
#include "private/qqmljssourcelocation_p.h"

#include <QSet>
#include <QString>
#include <QMap>

enum MessageColors{
    Error,
    Warning,
    Info,
    Normal,
    Hint
};

enum class ScopeType
{
    JSFunctionScope,
    JSLexicalScope,
    QMLScope
};

struct MethodUsage
{
    LanguageUtils::FakeMetaMethod method;
    QQmlJS::AST::SourceLocation loc;
    bool hasMultilineHandlerBody;
};

class ColorOutput;

class ScopeTree {
public:
    ScopeTree(ScopeType type, QString name="<none given>", ScopeTree* parentScope=nullptr);
    ~ScopeTree() {qDeleteAll(m_childScopes);}

    ScopeTree* createNewChildScope(ScopeType type, QString name);
    ScopeTree* parentScope();

    void insertJSIdentifier(QString id, QQmlJS::AST::VariableScope scope);
    void insertQMLIdentifier(QString id);
    void insertSignalIdentifier(QString id, LanguageUtils::FakeMetaMethod method, QQmlJS::AST::SourceLocation loc, bool hasMultilineHandlerBody);
    void insertPropertyIdentifier(QString id); // inserts property as qml identifier as well as the corresponding

    bool isIdInCurrentScope(QString const &id) const;
    void addIdToAccssedIfNotInParentScopes(QPair<QString, QQmlJS::AST::SourceLocation> const& id_loc_pair, const QSet<QString>& unknownImports);

    bool isVisualRootScope() const;
    QString name() const;

    bool recheckIdentifiers(const QString &code,  const QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr>& qmlIDs, const ScopeTree *root, const QString& rootId, ColorOutput &colorOut) const;
    ScopeType scopeType();
    void addMethod(LanguageUtils::FakeMetaMethod);
    void addMethodsFromMetaObject(LanguageUtils::FakeMetaObject::ConstPtr metaObject);
    QMap<QString, LanguageUtils::FakeMetaMethod>const & methods() const;

private:
    QSet<QString> m_currentScopeJSIdentifiers;
    QSet<QString> m_currentScopeQMLIdentifiers;
    QMultiHash<QString, MethodUsage> m_injectedSignalIdentifiers;
    QMap<QString, LanguageUtils::FakeMetaMethod> m_methods;
    QVector<QPair<QString, QQmlJS::AST::SourceLocation>> m_accessedIdentifiers;
    QVector<ScopeTree*> m_childScopes;
    ScopeTree *m_parentScope;
    QString m_name;
    ScopeType m_scopeType;

    bool isIdInCurrentQMlScopes(QString id) const;
    bool isIdInCurrentJSScopes(QString id) const;
    bool isIdInjectedFromSignal(QString id) const;
    const ScopeTree* getCurrentQMLScope() const;
    ScopeTree* getCurrentQMLScope();
};
#endif // SCOPETREE_H
