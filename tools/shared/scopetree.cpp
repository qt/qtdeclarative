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

ScopeTree::ScopeTree(ScopeType type, QString name, ScopeTree *parentScope)
    : m_parentScope(parentScope), m_name(std::move(name)), m_scopeType(type) {}

ScopeTree::Ptr ScopeTree::createNewChildScope(ScopeType type, const QString &name)
{
    Q_ASSERT(type != ScopeType::QMLScope
            || !m_parentScope
            || m_parentScope->m_scopeType == ScopeType::QMLScope
            || m_parentScope->m_name == QLatin1String("global"));
    auto childScope = ScopeTree::Ptr(new ScopeTree{type, name, this});
    m_childScopes.push_back(childScope);
    return childScope;
}

void ScopeTree::insertJSIdentifier(const QString &id, ScopeType scope)
{
    Q_ASSERT(m_scopeType != ScopeType::QMLScope);
    Q_ASSERT(scope != ScopeType::QMLScope);
    if (scope == ScopeType::JSFunctionScope) {
        auto targetScope = this;
        while (targetScope->scopeType() != ScopeType::JSFunctionScope)
            targetScope = targetScope->m_parentScope;
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
    return m_parentScope && m_parentScope->m_parentScope
            && m_parentScope->m_parentScope->m_parentScope == nullptr;
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
