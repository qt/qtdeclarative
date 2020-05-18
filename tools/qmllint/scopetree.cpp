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

ScopeTree::Ptr ScopeTree::createNewChildScope(ScopeType type, const QString &name)
{
    Q_ASSERT(type != ScopeType::QMLScope
            || !m_parentScope
            || m_parentScope->m_scopeType == ScopeType::QMLScope
            || m_parentScope->m_name == "global");
    auto childScope = ScopeTree::Ptr(new ScopeTree{type, name, this});
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
                                       const QQmlJS::SourceLocation &loc,
                                       bool hasMultilineHandlerBody)
{
    Q_ASSERT(m_scopeType == ScopeType::QMLScope);
    m_injectedSignalIdentifiers.insert(id, {method, loc, hasMultilineHandlerBody});
}

void ScopeTree::insertPropertyIdentifier(const MetaProperty &property)
{
    addProperty(property);
    MetaMethod method(property.propertyName() + QLatin1String("Changed"), "void");
    addMethod(method);
}

void ScopeTree::addUnmatchedSignalHandler(const QString &handler,
                                          const QQmlJS::SourceLocation &location)
{
    m_unmatchedSignalHandlers.append(qMakePair(handler, location));
}

bool ScopeTree::isIdInCurrentScope(const QString &id) const
{
    return isIdInCurrentQMlScopes(id) || isIdInCurrentJSScopes(id);
}

void ScopeTree::addIdToAccessed(const QString &id, const QQmlJS::SourceLocation &location) {
    m_currentFieldMember = new FieldMemberList {id, QString(), location, {}};
    m_accessedIdentifiers.push_back(std::unique_ptr<FieldMemberList>(m_currentFieldMember));
}

void ScopeTree::accessMember(const QString &name, const QString &parentType,
                             const QQmlJS::SourceLocation &location)
{
    Q_ASSERT(m_currentFieldMember);
    auto *fieldMember = new FieldMemberList {name, parentType, location, {}};
    m_currentFieldMember->m_child.reset(fieldMember);
    m_currentFieldMember = fieldMember;
}

void ScopeTree::resetMemberScope()
{
    m_currentFieldMember = nullptr;
}

bool ScopeTree::isVisualRootScope() const
{
    return m_parentScope && m_parentScope->m_parentScope
            && m_parentScope->m_parentScope->m_parentScope == nullptr;
}

class IssueLocationWithContext
{
public:
    IssueLocationWithContext(const QString &code, const QQmlJS::SourceLocation &location) {
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

static const QStringList unknownBuiltins = {
    // TODO: "string" should be added to builtins.qmltypes, and the special handling below removed
    QStringLiteral("alias"),    // TODO: we cannot properly resolve aliases, yet
    QStringLiteral("QRectF"),   // TODO: should be added to builtins.qmltypes
    QStringLiteral("QFont"),    // TODO: should be added to builtins.qmltypes
    QStringLiteral("QJSValue"), // We cannot say anything intelligent about untyped JS values.
    QStringLiteral("variant"),  // Same for generic variants
};

bool ScopeTree::checkMemberAccess(
        const QString &code,
        FieldMemberList *members,
        const ScopeTree *scope,
        const QHash<QString, ScopeTree::ConstPtr> &types,
        ColorOutput& colorOut) const
{
    if (!members->m_child)
        return true;

    Q_ASSERT(scope != nullptr);

    const QString scopeName = scope->name().isEmpty() ? scope->className() : scope->name();
    const auto &access = members->m_child;

    const auto scopeIt = scope->m_properties.find(access->m_name);
    if (scopeIt != scope->m_properties.end()) {
        const QString typeName = access->m_parentType.isEmpty() ? scopeIt->typeName()
                                                                : access->m_parentType;
        if (scopeIt->isList() || typeName == QLatin1String("string")) {
            if (access->m_child && access->m_child->m_name != QLatin1String("length")) {
                colorOut.write("Warning: ", Warning);
                colorOut.write(
                            QString::fromLatin1(
                                "\"%1\" is a %2. You cannot access \"%3\" on it at %4:%5\n")
                            .arg(access->m_name)
                            .arg(QLatin1String(scopeIt->isList() ? "list" : "string"))
                            .arg(access->m_child->m_name)
                            .arg(access->m_child->m_location.startLine)
                            .arg(access->m_child->m_location.startColumn), Normal);
                printContext(colorOut, code, access->m_child->m_location);
                return false;
            }
            return true;
        }

        if (!access->m_child)
            return true;

        if (const ScopeTree *type = scopeIt->type()) {
            if (access->m_parentType.isEmpty())
                return checkMemberAccess(code, access.get(), type, types, colorOut);
        }

        if (unknownBuiltins.contains(typeName))
            return true;

        const auto it = types.find(typeName);
        if (it != types.end())
            return checkMemberAccess(code, access.get(), it->get(), types, colorOut);

        colorOut.write("Warning: ", Warning);
        colorOut.write(
                    QString::fromLatin1("Type \"%1\" of member \"%2\" not found at %3:%4.\n")
                    .arg(typeName)
                    .arg(access->m_name)
                    .arg(access->m_location.startLine)
                    .arg(access->m_location.startColumn), Normal);
        printContext(colorOut, code, access->m_location);
        return false;
    }

    const auto scopeMethodIt = scope->m_methods.find(access->m_name);
    if (scopeMethodIt != scope->m_methods.end())
        return true; // Access to property of JS function

    for (const auto &enumerator : scope->m_enums) {
        for (const QString &key : enumerator.keys()) {
            if (access->m_name != key)
                continue;

            if (!access->m_child)
                return true;

            colorOut.write("Warning: ", Warning);
            colorOut.write(QString::fromLatin1(
                               "\"%1\" is an enum value. You cannot access \"%2\" on it at %3:%4\n")
                           .arg(access->m_name)
                           .arg(access->m_child->m_name)
                           .arg(access->m_child->m_location.startLine)
                           .arg(access->m_child->m_location.startColumn), Normal);
            printContext(colorOut, code, access->m_child->m_location);
            return false;
        }
    }

    auto type = types.value(access->m_parentType.isEmpty() ? scopeName : access->m_parentType);
    while (type) {
        const auto typeIt = type->m_properties.find(access->m_name);
        if (typeIt != type->m_properties.end()) {
            const ScopeTree *propType = typeIt->type();
            return checkMemberAccess(code, access.get(),
                                     propType ? propType : types.value(typeIt->typeName()).get(),
                                     types, colorOut);
        }

        const auto typeMethodIt = type->m_methods.find(access->m_name);
        if (typeMethodIt != type->m_methods.end()) {
            if (access->m_child == nullptr)
                return true;

            colorOut.write("Warning: ", Warning);
            colorOut.write(QString::fromLatin1(
                                   "\"%1\" is a method. You cannot access \"%2\" on it at %3:%4\n")
                                   .arg(access->m_name)
                                   .arg(access->m_child->m_name)
                                   .arg(access->m_child->m_location.startLine)
                                   .arg(access->m_child->m_location.startColumn), Normal);
            printContext(colorOut, code, access->m_child->m_location);
            return false;
        }

        type = types.value(type->superclassName());
    }

    if (access->m_name.front().isUpper() && scope->scopeType() == ScopeType::QMLScope) {
        // may be an attached type
        const auto it = types.find(access->m_name);
        if (it != types.end() && !(*it)->attachedTypeName().isEmpty()) {
            const auto attached = types.find((*it)->attachedTypeName());
            if (attached != types.end())
                return checkMemberAccess(code, access.get(), attached->get(), types, colorOut);
        }
    }

    colorOut.write("Warning: ", Warning);
    colorOut.write(QString::fromLatin1(
                           "Property \"%1\" not found on type \"%2\" at %3:%4\n")
                           .arg(access->m_name)
                           .arg(scopeName)
                           .arg(access->m_location.startLine)
                           .arg(access->m_location.startColumn), Normal);
    printContext(colorOut, code, access->m_location);
    return false;
}

bool ScopeTree::recheckIdentifiers(
        const QString &code,
        const QHash<QString, const ScopeTree *> &qmlIDs,
        const QHash<QString, ScopeTree::ConstPtr> &types,
        const ScopeTree *root, const QString &rootId,
        ColorOutput& colorOut) const
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

        for (const auto &memberAccessTree : qAsConst(currentScope->m_accessedIdentifiers)) {
            if (currentScope->isIdInCurrentJSScopes(memberAccessTree->m_name))
                continue;

            auto it = qmlIDs.find(memberAccessTree->m_name);
            if (it != qmlIDs.end()) {
                if (*it != nullptr) {
                    if (!checkMemberAccess(code, memberAccessTree.get(), *it, types, colorOut))
                        noUnqualifiedIdentifier = false;
                    continue;
                } else if (memberAccessTree->m_child
                           && memberAccessTree->m_child->m_name.front().isUpper()) {
                    // It could be a qualified type name
                    const QString qualified = memberAccessTree->m_name + QLatin1Char('.')
                            + memberAccessTree->m_child->m_name;
                    const auto typeIt = types.find(qualified);
                    if (typeIt != types.end()) {
                        if (!checkMemberAccess(code, memberAccessTree->m_child.get(), typeIt->get(),
                                               types, colorOut)) {
                            noUnqualifiedIdentifier = false;
                        }
                        continue;
                    }
                }
            }

            auto qmlScope = currentScope->currentQMLScope();
            if (qmlScope->methods().contains(memberAccessTree->m_name)) {
                // a property of a JavaScript function
                continue;
            }

            const auto qmlIt = qmlScope->m_properties.find(memberAccessTree->m_name);
            if (qmlIt != qmlScope->m_properties.end()) {
                if (!memberAccessTree->m_child || unknownBuiltins.contains(qmlIt->typeName()))
                    continue;

                if (!qmlIt->type()) {
                    colorOut.write("Warning: ", Warning);
                    colorOut.write(QString::fromLatin1(
                                           "Type of property \"%2\" not found at %3:%4\n")
                                           .arg(memberAccessTree->m_name)
                                           .arg(memberAccessTree->m_location.startLine)
                                           .arg(memberAccessTree->m_location.startColumn), Normal);
                    printContext(colorOut, code, memberAccessTree->m_location);
                    noUnqualifiedIdentifier = false;
                } else if (!checkMemberAccess(code, memberAccessTree.get(), qmlIt->type(), types,
                                              colorOut)) {
                    noUnqualifiedIdentifier = false;
                }

                continue;
            }

            // TODO: Lots of builtins are missing
            if (memberAccessTree->m_name == "Qt")
                continue;

            const auto typeIt = types.find(memberAccessTree->m_name);
            if (typeIt != types.end()) {
                if (!checkMemberAccess(code, memberAccessTree.get(), typeIt->get(), types,
                                       colorOut)) {
                    noUnqualifiedIdentifier = false;
                }
                continue;
            }

            noUnqualifiedIdentifier = false;
            colorOut.write("Warning: ", Warning);
            auto location = memberAccessTree->m_location;
            colorOut.write(QString::fromLatin1("unqualified access at %1:%2\n")
                           .arg(location.startLine).arg(location.startColumn),
                           Normal);

            printContext(colorOut, code, location);

            // root(JS) --> program(qml) --> (first element)
            const auto firstElement = root->m_childScopes[0]->m_childScopes[0];
            if (firstElement->m_properties.contains(memberAccessTree->m_name)
                    || firstElement->m_methods.contains(memberAccessTree->m_name)
                    || firstElement->m_enums.contains(memberAccessTree->m_name)) {
                colorOut.write("Note: ", Info);
                colorOut.write(memberAccessTree->m_name + QLatin1String(" is a member of the root element\n"), Normal );
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
            } else if (currentScope->isIdInjectedFromSignal(memberAccessTree->m_name)) {
                auto methodUsages = currentScope->currentQMLScope()->m_injectedSignalIdentifiers
                        .values(memberAccessTree->m_name);
                auto location = memberAccessTree->m_location;
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
                            memberAccessTree->m_name + QString::fromLatin1(
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
        for (auto const &childScope: currentScope->m_childScopes)
            workQueue.enqueue(childScope.get());
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
                             const QQmlJS::SourceLocation &location) const
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

void ScopeTree::updateParentProperty(const ScopeTree *scope)
{
    auto it = m_properties.find(QLatin1String("parent"));
    if (it != m_properties.end()
            && scope->name() != QLatin1String("Component")
            && scope->name() != QLatin1String("program"))
        it->setType(scope);
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
