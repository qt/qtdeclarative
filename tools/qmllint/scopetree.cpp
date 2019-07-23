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

#include "scopetree.h"

#include "qcoloroutput_p.h"

#include <algorithm>

#include <QQueue>

ScopeTree::ScopeTree(ScopeType type, QString name, ScopeTree *parentScope)
    : m_parentScope(parentScope), m_name(name), m_scopeType(type) {}

ScopeTree *ScopeTree::createNewChildScope(ScopeType type, QString name) {
    Q_ASSERT(type != ScopeType::QMLScope|| !m_parentScope || m_parentScope->m_scopeType == ScopeType::QMLScope || m_parentScope->m_name == "global");
    auto childScope = new ScopeTree{type, name, this};
    m_childScopes.push_back(childScope);
    return childScope;
}

ScopeTree *ScopeTree::parentScope() {
    return m_parentScope;
}

void ScopeTree::insertJSIdentifier(QString id, QQmlJS::AST::VariableScope scope)
{
    Q_ASSERT(m_scopeType != ScopeType::QMLScope);
    if (scope == QQmlJS::AST::VariableScope::Var) {
        auto targetScope = this;
        while (targetScope->scopeType() != ScopeType::JSFunctionScope) {
            targetScope = targetScope->m_parentScope;
        }
        targetScope->m_currentScopeJSIdentifiers.insert(id);
    } else {
        m_currentScopeJSIdentifiers.insert(id);
    }
}

void ScopeTree::insertQMLIdentifier(QString id)
{
    Q_ASSERT(m_scopeType == ScopeType::QMLScope);
    m_currentScopeQMLIdentifiers.insert(id);
}

void ScopeTree::insertSignalIdentifier(QString id, LanguageUtils::FakeMetaMethod method, QQmlJS::AST::SourceLocation loc, bool hasMultilineHandlerBody)
{
    Q_ASSERT(m_scopeType == ScopeType::QMLScope);
    m_injectedSignalIdentifiers.insert(id, {method, loc, hasMultilineHandlerBody});
}

void ScopeTree::insertPropertyIdentifier(QString id)
{
    this->insertQMLIdentifier(id);
    LanguageUtils::FakeMetaMethod method( id + QLatin1String("Changed"), "void");
    this->addMethod(method);
}

bool ScopeTree::isIdInCurrentScope(const QString &id) const
{
    return isIdInCurrentQMlScopes(id) || isIdInCurrentJSScopes(id);
}

void ScopeTree::addIdToAccssedIfNotInParentScopes(const QPair<QString, QQmlJS::AST::SourceLocation> &id_loc_pair, const QSet<QString>& unknownImports) {
    // also do not add id if it is parent
    // parent is almost always defined valid in QML, and if we could not find a definition for the current QML component
    // not skipping "parent" will lead to many false positives
    // Moreover, if the top level item is Item or inherits from it, it will have a parent property to which we would point the user
    // which makes for a very nonsensical warning
    auto qmlScope = getCurrentQMLScope();
    if (!isIdInCurrentScope(id_loc_pair.first) && !(id_loc_pair.first == QLatin1String("parent") && qmlScope && unknownImports.contains(qmlScope->name()))) {
        m_accessedIdentifiers.push_back(id_loc_pair);
    }
}

bool ScopeTree::isVisualRootScope() const
{
    return m_parentScope && m_parentScope->m_parentScope && m_parentScope->m_parentScope->m_parentScope == nullptr;
}

QString ScopeTree::name() const
{
    return m_name;
}

struct IssueLocationWithContext
{
    IssueLocationWithContext(const QString& code, QQmlJS::AST::SourceLocation location) {
        int before = std::max(0,code.lastIndexOf('\n', location.offset));
        beforeText = code.midRef(before+1, location.offset - (before+1) );
        issueText = code.midRef(location.offset, location.length);
        int after = code.indexOf('\n', location.offset + location.length);
        afterText = code.midRef(location.offset+location.length, after - (location.offset+location.length));
    }

    QStringRef beforeText;
    QStringRef issueText;
    QStringRef afterText;
};

bool ScopeTree::recheckIdentifiers(const QString& code, const QHash<QString, LanguageUtils::FakeMetaObject::ConstPtr> &qmlIDs, const ScopeTree *root, const QString& rootId, ColorOutput& colorOut) const
{
    bool noUnqualifiedIdentifier = true;

    // revisit all scopes
    QQueue<const ScopeTree*> workQueue;
    workQueue.enqueue(this);
    while (!workQueue.empty()) {
        const ScopeTree* currentScope = workQueue.dequeue();
        for (auto idLocationPair : currentScope->m_accessedIdentifiers) {
            if (qmlIDs.contains(idLocationPair.first))
                continue;
            if (currentScope->isIdInCurrentScope(idLocationPair.first)) {
                continue;
            }
            noUnqualifiedIdentifier = false;
            colorOut.write("Warning: ", Warning);
            auto location = idLocationPair.second;
            colorOut.write(QString::asprintf("unqualified access at %d:%d\n", location.startLine, location.startColumn), Normal);
            IssueLocationWithContext issueLocationWithContext {code, location};
            colorOut.write(issueLocationWithContext.beforeText.toString(), Normal);
            colorOut.write(issueLocationWithContext.issueText.toString(), Error);
            colorOut.write(issueLocationWithContext.afterText.toString() + QLatin1Char('\n'), Normal);
            int tabCount = issueLocationWithContext.beforeText.count(QLatin1Char('\t'));
            colorOut.write(QString(" ").repeated(issueLocationWithContext.beforeText.length() - tabCount) + QString("\t").repeated(tabCount) + QString("^").repeated(location.length) + QLatin1Char('\n'), Normal);
            // root(JS) --> program(qml) --> (first element)
            if (root->m_childScopes[0]->m_childScopes[0]->m_currentScopeQMLIdentifiers.contains(idLocationPair.first)) {
                ScopeTree *parentScope = currentScope->m_parentScope;
                while (parentScope && parentScope->scopeType() != ScopeType::QMLScope) {
                    parentScope = parentScope->m_parentScope;
                }
                colorOut.write("Note: ", Info);
                colorOut.write( idLocationPair.first + QLatin1String(" is a meber of the root element\n"), Normal );
                colorOut.write(QLatin1String("      You can qualify the access with its id to avoid this warning:\n"), Normal);
                if (rootId == QLatin1String("<id>")) {
                    colorOut.write("Note: ", Warning);
                    colorOut.write(("You first have to give the root element an id\n"));
                }
                colorOut.write(issueLocationWithContext.beforeText.toString(), Normal);
                colorOut.write(rootId + QLatin1Char('.'), Hint);
                colorOut.write(issueLocationWithContext.issueText.toString(), Normal);
                colorOut.write(issueLocationWithContext.afterText + QLatin1Char('\n'), Normal);
            } else if (currentScope->isIdInjectedFromSignal(idLocationPair.first)) {
                auto qmlScope = currentScope->getCurrentQMLScope();
                auto methodUsages = qmlScope->m_injectedSignalIdentifiers.values(idLocationPair.first);
                auto location = idLocationPair.second;
                // sort the list of signal handlers by their occurrence in the source code
                // then, we select the first one whose location is after the unqualified id
                // and go one step backwards to get the one which we actually need
                std::sort(methodUsages.begin(), methodUsages.end(), [](const MethodUsage m1, const MethodUsage m2) {
                    return m1.loc.startLine < m2.loc.startLine || (m1.loc.startLine == m2.loc.startLine && m1.loc.startColumn < m2.loc.startColumn);
                });
                auto oneBehindIt = std::find_if(methodUsages.begin(), methodUsages.end(), [&location](MethodUsage methodUsage) {
                    return location.startLine < methodUsage.loc.startLine || (location.startLine == methodUsage.loc.startLine && location.startColumn < methodUsage.loc.startColumn);
                });
                auto methodUsage = *(--oneBehindIt);
                colorOut.write("Note:", Info);
                colorOut.write(idLocationPair.first + QString::asprintf(" is accessible in this scope because you are handling a signal at %d:%d\n", methodUsage.loc.startLine, methodUsage.loc.startColumn), Normal);
                colorOut.write("Consider using a function instead\n", Normal);
                IssueLocationWithContext context {code, methodUsage.loc};
                colorOut.write(context.beforeText + QLatin1Char(' '));
                colorOut.write(methodUsage.hasMultilineHandlerBody ? "function(" : "(", Hint);
                const auto parameters = methodUsage.method.parameterNames();
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    colorOut.write(parameters.at(parameters.size() - numParams), Hint);
                    if (numParams > 1) {
                        colorOut.write(", ", Hint);
                    }
                }
                colorOut.write(methodUsage.hasMultilineHandlerBody ? ")" : ") => ", Hint);
                colorOut.write(" {...", Normal);
            }
            colorOut.write("\n\n\n", Normal);
        }
        for (auto const& childScope: currentScope->m_childScopes) {
            workQueue.enqueue(childScope);
        }
    }
    return noUnqualifiedIdentifier;
}

QMap<QString, LanguageUtils::FakeMetaMethod>const &ScopeTree::methods() const
{
    return m_methods;
}

bool ScopeTree::isIdInCurrentQMlScopes(QString id) const
{
    auto qmlScope = getCurrentQMLScope();
    return qmlScope->m_currentScopeQMLIdentifiers.contains(id);
}

bool ScopeTree::isIdInCurrentJSScopes(QString id) const
{
    auto jsScope = this;
    while (jsScope) {
        if (jsScope->m_scopeType != ScopeType::QMLScope && jsScope->m_currentScopeJSIdentifiers.contains(id))
            return true;
        jsScope = jsScope->m_parentScope;
    }
    return false;
}

bool ScopeTree::isIdInjectedFromSignal(QString id) const
{
    auto qmlScope = getCurrentQMLScope();
    return qmlScope->m_injectedSignalIdentifiers.contains(id);
}

const ScopeTree *ScopeTree::getCurrentQMLScope() const
{
    auto qmlScope = this;
    while (qmlScope && qmlScope->m_scopeType != ScopeType::QMLScope) {
        qmlScope = qmlScope->m_parentScope;
    }
    return qmlScope;
}

ScopeTree *ScopeTree::getCurrentQMLScope()
{
    auto qmlScope = this;
    while (qmlScope && qmlScope->m_scopeType != ScopeType::QMLScope) {
        qmlScope = qmlScope->m_parentScope;
    }
    return qmlScope;
}

ScopeType ScopeTree::scopeType() {return m_scopeType;}

void ScopeTree::addMethod(LanguageUtils::FakeMetaMethod method)
{
    m_methods.insert(method.methodName(), method);
}

void ScopeTree::addMethodsFromMetaObject(LanguageUtils::FakeMetaObject::ConstPtr metaObject)
{
    if (metaObject) {
        auto methodCount = metaObject->methodCount();
        for (auto i = 0; i < methodCount; ++i) {
            auto method = metaObject->method(i);
            this->addMethod(method);
        }
    }
}
