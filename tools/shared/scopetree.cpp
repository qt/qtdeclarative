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

#include <QtCore/qqueue.h>
#include <QtCore/qsharedpointer.h>

#include <algorithm>

ScopeTree::ScopeTree(ScopeType type, const ScopeTree::Ptr &parentScope)
    : m_parentScope(parentScope), m_scopeType(type) {}

ScopeTree::Ptr ScopeTree::create(ScopeType type, const ScopeTree::Ptr &parentScope)
{
    ScopeTree::Ptr childScope(new ScopeTree{type, parentScope});
    if (parentScope) {
        Q_ASSERT(type != ScopeType::QMLScope
                || !parentScope->m_parentScope
                || parentScope->parentScope()->m_scopeType == ScopeType::QMLScope
                || parentScope->parentScope()->m_internalName == QLatin1String("global"));
        parentScope->m_childScopes.push_back(childScope);
    }
    return childScope;
}

void ScopeTree::insertJSIdentifier(const QString &id, ScopeType scope)
{
    Q_ASSERT(m_scopeType != ScopeType::QMLScope);
    Q_ASSERT(scope != ScopeType::QMLScope);
    if (scope != ScopeType::JSFunctionScope || m_scopeType == ScopeType::JSFunctionScope) {
        m_jsIdentifiers.insert(id);
    } else {
        auto targetScope = parentScope();
        while (targetScope->m_scopeType != ScopeType::JSFunctionScope)
            targetScope = targetScope->parentScope();
        targetScope->m_jsIdentifiers.insert(id);
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
    MetaMethod method(property.propertyName() + QLatin1String("Changed"), QLatin1String("void"));
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
    m_memberAccessChains.append(QVector<FieldMember>());
    m_memberAccessChains.last().append(FieldMember {id, QString(), location});
}

void ScopeTree::accessMember(const QString &name, const QString &parentType,
                             const QQmlJS::SourceLocation &location)
{
    Q_ASSERT(!m_memberAccessChains.last().isEmpty());
    m_memberAccessChains.last().append(FieldMember {name, parentType, location });
}

bool ScopeTree::isVisualRootScope() const
{
    if (!m_parentScope)
        return false;

    const auto grandParent = parentScope()->m_parentScope.toStrongRef();
    if (!grandParent)
        return false;

    return grandParent->m_parentScope == nullptr;
}

bool ScopeTree::isIdInCurrentQMlScopes(const QString &id) const
{
    if (m_scopeType == ScopeType::QMLScope)
        return m_properties.contains(id) || m_methods.contains(id) || m_enums.contains(id);

    const auto qmlScope = findCurrentQMLScope(parentScope());
    return qmlScope->m_properties.contains(id)
            || qmlScope->m_methods.contains(id)
            || qmlScope->m_enums.contains(id);
}

bool ScopeTree::isIdInCurrentJSScopes(const QString &id) const
{
    if (m_scopeType != ScopeType::QMLScope && m_jsIdentifiers.contains(id))
        return true;

    for (auto jsScope = parentScope(); jsScope; jsScope = jsScope->parentScope()) {
        if (jsScope->m_scopeType != ScopeType::QMLScope && jsScope->m_jsIdentifiers.contains(id))
            return true;
    }

    return false;
}

bool ScopeTree::isIdInjectedFromSignal(const QString &id) const
{
    if (m_scopeType == ScopeType::QMLScope)
        return m_injectedSignalIdentifiers.contains(id);
    return findCurrentQMLScope(parentScope())->m_injectedSignalIdentifiers.contains(id);
}

ScopeTree::ConstPtr ScopeTree::findCurrentQMLScope(const ScopeTree::ConstPtr &scope)
{
    auto qmlScope = scope;
    while (qmlScope && qmlScope->m_scopeType != ScopeType::QMLScope)
        qmlScope = qmlScope->parentScope();
    return qmlScope;
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

void ScopeTree::updateParentProperty(const ScopeTree::ConstPtr &scope)
{
    auto it = m_properties.find(QLatin1String("parent"));
    if (it != m_properties.end()
            && scope->internalName() != QLatin1String("Component")
            && scope->internalName() != QLatin1String("program"))
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
