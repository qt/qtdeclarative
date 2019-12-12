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
#include "qcoloroutput.h"

#include <QtCore/qqueue.h>

#include <algorithm>

ScopeTree::ScopeTree(ScopeType type, QString name, ScopeTree *parentScope)
    : m_parentScope(parentScope), m_name(std::move(name)), m_scopeType(type) {}

ScopeTree *ScopeTree::createNewChildScope(ScopeType type, QString name)
{
    Q_ASSERT(type != ScopeType::QMLScope
            || !m_parentScope
            || m_parentScope->m_scopeType == ScopeType::QMLScope
            || m_parentScope->m_name == "global");
    auto childScope = new ScopeTree{type, std::move(name), this};
    m_childScopes.push_back(childScope);
    return childScope;
}

void ScopeTree::insertJSIdentifier(const QString &id, QQmlJS::AST::VariableScope scope)
{
    Q_ASSERT(m_scopeType != ScopeType::QMLScope);
    if (scope == QQmlJS::AST::VariableScope::Var) {
        auto targetScope = this;
        while (targetScope->scopeType() != ScopeType::JSFunctionScope) {
            targetScope = targetScope->m_parentScope;
        }
        targetScope->m_jsIdentifiers.insert(id);
    } else {
        m_jsIdentifiers.insert(id);
    }
}

void ScopeTree::insertSignalIdentifier(const QString &id, const MetaMethod &method,
                                       const QQmlJS::AST::SourceLocation &loc,
                                       bool hasMultilineHandlerBody)
{
    Q_ASSERT(m_scopeType == ScopeType::QMLScope);
    m_injectedSignalIdentifiers.insert(id, {method, loc, hasMultilineHandlerBody});
}

void ScopeTree::insertPropertyIdentifier(const MetaProperty &property)
{
    addProperty(property);
    MetaMethod method(property.name() + QLatin1String("Changed"), "void");
    addMethod(method);
}

void ScopeTree::addUnmatchedSignalHandler(const QString &handler,
                                          const QQmlJS::AST::SourceLocation &location)
{
    m_unmatchedSignalHandlers.append(qMakePair(handler, location));
}

bool ScopeTree::isIdInCurrentScope(const QString &id) const
{
    return isIdInCurrentQMlScopes(id) || isIdInCurrentJSScopes(id);
}

void ScopeTree::addIdToAccssedIfNotInParentScopes(
        const QPair<QString, QQmlJS::AST::SourceLocation> &idLocationPair,
        const QSet<QString> &unknownImports)
{
    // also do not add id if it is parent
    // parent is almost always defined valid in QML, and if we could not find a definition for the current QML component
    // not skipping "parent" will lead to many false positives
    // Moreover, if the top level item is Item or inherits from it, it will have a parent property to which we would point the user
    // which makes for a very nonsensical warning
    const auto *qmlScope = currentQMLScope();
    if (!isIdInCurrentScope(idLocationPair.first)
            && !(idLocationPair.first == QLatin1String("parent")
                 && qmlScope && unknownImports.contains(qmlScope->name()))) {
        m_accessedIdentifiers.push_back(idLocationPair);
    }
}

bool ScopeTree::isVisualRootScope() const
{
    return m_parentScope && m_parentScope->m_parentScope
            && m_parentScope->m_parentScope->m_parentScope == nullptr;
}

class IssueLocationWithContext
{
public:
    IssueLocationWithContext(const QString &code, const QQmlJS::AST::SourceLocation &location) {
        int before = std::max(0,code.lastIndexOf('\n', location.offset));
        m_beforeText = code.midRef(before + 1, int(location.offset - (before + 1)));
        m_issueText = code.midRef(location.offset, location.length);
        int after = code.indexOf('\n', int(location.offset + location.length));
        m_afterText = code.midRef(int(location.offset + location.length),
                                  int(after - (location.offset+location.length)));
    }

    QStringRef beforeText() const { return m_beforeText; }
    QStringRef issueText() const { return m_issueText; }
    QStringRef afterText() const { return m_afterText; }

private:
    QStringRef m_beforeText;
    QStringRef m_issueText;
    QStringRef m_afterText;
};

bool ScopeTree::recheckIdentifiers(
        const QString &code, const QHash<QString, ScopeTree::ConstPtr> &qmlIDs,
        const ScopeTree *root, const QString &rootId, ColorOutput &colorOut) const
{
    bool noUnqualifiedIdentifier = true;

    // revisit all scopes
    QQueue<const ScopeTree *> workQueue;
    workQueue.enqueue(this);
    while (!workQueue.empty()) {
        const ScopeTree *currentScope = workQueue.dequeue();
        for (const auto &handler : currentScope->m_unmatchedSignalHandlers) {
            colorOut.write("Warning: ", Warning);
            colorOut.write(QString::fromLatin1(
                                   "no matching signal found for handler \"%1\" at %2:%3\n")
                                   .arg(handler.first).arg(handler.second.startLine)
                                   .arg(handler.second.startColumn), Normal);
            printContext(colorOut, code, handler.second);
        }

        for (const auto &idLocationPair : qAsConst(currentScope->m_accessedIdentifiers)) {
            if (qmlIDs.contains(idLocationPair.first))
                continue;
            if (currentScope->isIdInCurrentScope(idLocationPair.first)) {
                continue;
            }
            noUnqualifiedIdentifier = false;
            colorOut.write("Warning: ", Warning);
            auto location = idLocationPair.second;
            colorOut.write(QString::fromLatin1("unqualified access at %1:%2\n")
                           .arg(location.startLine).arg(location.startColumn),
                           Normal);

            printContext(colorOut, code, location);

            // root(JS) --> program(qml) --> (first element)
            const auto firstElement = root->m_childScopes[0]->m_childScopes[0];
            if (firstElement->m_properties.contains(idLocationPair.first)
                    || firstElement->m_methods.contains(idLocationPair.first)
                    || firstElement->m_enums.contains(idLocationPair.first)) {
                colorOut.write("Note: ", Info);
                colorOut.write( idLocationPair.first + QLatin1String(" is a meber of the root element\n"), Normal );
                colorOut.write(QLatin1String("      You can qualify the access with its id to avoid this warning:\n"), Normal);
                if (rootId == QLatin1String("<id>")) {
                    colorOut.write("Note: ", Warning);
                    colorOut.write(("You first have to give the root element an id\n"));
                }
                IssueLocationWithContext issueLocationWithContext {code, location};
                colorOut.write(issueLocationWithContext.beforeText().toString(), Normal);
                colorOut.write(rootId + QLatin1Char('.'), Hint);
                colorOut.write(issueLocationWithContext.issueText().toString(), Normal);
                colorOut.write(issueLocationWithContext.afterText() + QLatin1Char('\n'), Normal);
            } else if (currentScope->isIdInjectedFromSignal(idLocationPair.first)) {
                auto methodUsages = currentScope->currentQMLScope()->m_injectedSignalIdentifiers
                        .values(idLocationPair.first);
                auto location = idLocationPair.second;
                // sort the list of signal handlers by their occurrence in the source code
                // then, we select the first one whose location is after the unqualified id
                // and go one step backwards to get the one which we actually need
                std::sort(methodUsages.begin(), methodUsages.end(),
                          [](const MethodUsage &m1, const MethodUsage &m2) {
                    return m1.loc.startLine < m2.loc.startLine
                            || (m1.loc.startLine == m2.loc.startLine
                                && m1.loc.startColumn < m2.loc.startColumn);
                });
                auto oneBehindIt = std::find_if(methodUsages.begin(), methodUsages.end(),
                                                [&location](const MethodUsage &methodUsage) {
                    return location.startLine < methodUsage.loc.startLine
                            || (location.startLine == methodUsage.loc.startLine
                                && location.startColumn < methodUsage.loc.startColumn);
                });
                auto methodUsage = *(--oneBehindIt);
                colorOut.write("Note:", Info);
                colorOut.write(
                            idLocationPair.first + QString::fromLatin1(
                                " is accessible in this scope because "
                                "you are handling a signal at %1:%2\n")
                            .arg(methodUsage.loc.startLine).arg(methodUsage.loc.startColumn),
                            Normal);
                colorOut.write("Consider using a function instead\n", Normal);
                IssueLocationWithContext context {code, methodUsage.loc};
                colorOut.write(context.beforeText() + QLatin1Char(' '));
                colorOut.write(methodUsage.hasMultilineHandlerBody ? "function(" : "(", Hint);
                const auto parameters = methodUsage.method.parameterNames();
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    colorOut.write(parameters.at(parameters.size() - numParams), Hint);
                    if (numParams > 1)
                        colorOut.write(", ", Hint);
                }
                colorOut.write(methodUsage.hasMultilineHandlerBody ? ")" : ") => ", Hint);
                colorOut.write(" {...", Normal);
            }
            colorOut.write("\n\n\n", Normal);
        }
        for (auto const& childScope: currentScope->m_childScopes)
            workQueue.enqueue(childScope);
    }
    return noUnqualifiedIdentifier;
}

bool ScopeTree::isIdInCurrentQMlScopes(const QString &id) const
{
    const auto *qmlScope = currentQMLScope();
    return qmlScope->m_properties.contains(id)
            || qmlScope->m_methods.contains(id)
            || qmlScope->m_enums.contains(id);
}

bool ScopeTree::isIdInCurrentJSScopes(const QString &id) const
{
    auto jsScope = this;
    while (jsScope) {
        if (jsScope->m_scopeType != ScopeType::QMLScope && jsScope->m_jsIdentifiers.contains(id))
            return true;
        jsScope = jsScope->m_parentScope;
    }
    return false;
}

bool ScopeTree::isIdInjectedFromSignal(const QString &id) const
{
    return currentQMLScope()->m_injectedSignalIdentifiers.contains(id);
}

const ScopeTree *ScopeTree::currentQMLScope() const
{
    auto qmlScope = this;
    while (qmlScope && qmlScope->m_scopeType != ScopeType::QMLScope)
        qmlScope = qmlScope->m_parentScope;
    return qmlScope;
}

void ScopeTree::printContext(ColorOutput &colorOut, const QString &code,
                             const QQmlJS::AST::SourceLocation &location) const
{
    IssueLocationWithContext issueLocationWithContext {code, location};
    colorOut.write(issueLocationWithContext.beforeText().toString(), Normal);
    colorOut.write(issueLocationWithContext.issueText().toString(), Error);
    colorOut.write(issueLocationWithContext.afterText().toString() + QLatin1Char('\n'), Normal);
    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    colorOut.write(QString(" ").repeated(issueLocationWithContext.beforeText().length() - tabCount)
                           + QString("\t").repeated(tabCount)
                           + QString("^").repeated(location.length)
                           + QLatin1Char('\n'), Normal);
}

void ScopeTree::addExport(const QString &name, const QString &package,
                          const ComponentVersion &version)
{
    m_exports.append(Export(package, name, version, 0));
}

void ScopeTree::setExportMetaObjectRevision(int exportIndex, int metaObjectRevision)
{
    m_exports[exportIndex].setMetaObjectRevision(metaObjectRevision);
}

ScopeTree::Export::Export(QString package, QString type, const ComponentVersion &version,
                          int metaObjectRevision) :
    m_package(std::move(package)),
    m_type(std::move(type)),
    m_version(version),
    m_metaObjectRevision(metaObjectRevision)
{
}

bool ScopeTree::Export::isValid() const
{
    return m_version.isValid() || !m_package.isEmpty() || !m_type.isEmpty();
}
