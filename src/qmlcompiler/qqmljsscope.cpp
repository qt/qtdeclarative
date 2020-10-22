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

#include "qqmljsscope_p.h"
#include "qqmljstypereader_p.h"
#include "qqmljsimporter_p.h"

#include <QtCore/qqueue.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qfileinfo.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

QQmlJSScope::QQmlJSScope(ScopeType type, const QQmlJSScope::Ptr &parentScope)
    : m_parentScope(parentScope), m_scopeType(type) {}

QQmlJSScope::Ptr QQmlJSScope::create(ScopeType type, const QQmlJSScope::Ptr &parentScope)
{
    QSharedPointer<QQmlJSScope> childScope(new QQmlJSScope{type, parentScope});
    if (parentScope)
        parentScope->m_childScopes.push_back(childScope);
    return childScope;
}

void QQmlJSScope::insertJSIdentifier(const QString &name, const JavaScriptIdentifier &identifier)
{
    Q_ASSERT(m_scopeType != QQmlJSScope::QMLScope);
    if (identifier.kind == JavaScriptIdentifier::LexicalScoped
            || identifier.kind == JavaScriptIdentifier::Injected
            || m_scopeType == QQmlJSScope::JSFunctionScope) {
        m_jsIdentifiers.insert(name, identifier);
    } else {
        auto targetScope = parentScope();
        while (targetScope->m_scopeType != QQmlJSScope::JSFunctionScope)
            targetScope = targetScope->parentScope();
        targetScope->m_jsIdentifiers.insert(name, identifier);
    }
}

void QQmlJSScope::insertPropertyIdentifier(const QQmlJSMetaProperty &property)
{
    addOwnProperty(property);
    QQmlJSMetaMethod method(property.propertyName() + QLatin1String("Changed"), QLatin1String("void"));
    addOwnMethod(method);
}

bool QQmlJSScope::isIdInCurrentScope(const QString &id) const
{
    return isIdInCurrentQMlScopes(id) || isIdInCurrentJSScopes(id);
}

bool QQmlJSScope::hasMethod(const QString &name) const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().data()) {
        if (scope->m_methods.contains(name))
            return true;
    }
    return false;
}

QQmlJSMetaMethod QQmlJSScope::method(const QString &name) const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().data()) {
        const auto it = scope->m_methods.find(name);
        if (it != scope->m_methods.end())
            return *it;
    }
    return {};
}

bool QQmlJSScope::isIdInCurrentQMlScopes(const QString &id) const
{
    if (m_scopeType == QQmlJSScope::QMLScope)
        return m_properties.contains(id) || m_methods.contains(id) || m_enums.contains(id);

    const auto qmlScope = findCurrentQMLScope(parentScope());
    return qmlScope->m_properties.contains(id)
            || qmlScope->m_methods.contains(id)
            || qmlScope->m_enums.contains(id);
}

bool QQmlJSScope::isIdInCurrentJSScopes(const QString &id) const
{
    if (m_scopeType != QQmlJSScope::QMLScope && m_jsIdentifiers.contains(id))
        return true;

    for (auto jsScope = parentScope(); jsScope; jsScope = jsScope->parentScope()) {
        if (jsScope->m_scopeType != QQmlJSScope::QMLScope && jsScope->m_jsIdentifiers.contains(id))
            return true;
    }

    return false;
}

bool QQmlJSScope::isIdInjectedFromSignal(const QString &id) const
{
    const auto found = findJSIdentifier(id);
    return found.has_value() && found->kind == JavaScriptIdentifier::Injected;
}

std::optional<QQmlJSScope::JavaScriptIdentifier>
QQmlJSScope::findJSIdentifier(const QString &id) const
{
    for (const auto *scope = this; scope; scope = scope->parentScope().data()) {
        if (scope->m_scopeType == QQmlJSScope::JSFunctionScope
                || scope->m_scopeType == QQmlJSScope::JSLexicalScope) {
            auto it = scope->m_jsIdentifiers.find(id);
            if (it != scope->m_jsIdentifiers.end())
                return *it;
        }
    }

    return std::optional<JavaScriptIdentifier>{};
}

void QQmlJSScope::resolveTypes(const QHash<QString, QQmlJSScope::ConstPtr> &contextualTypes)
{
    auto findType = [&](const QString &name) {
        auto type = contextualTypes.constFind(name);
        if (type != contextualTypes.constEnd())
            return *type;

        return QQmlJSScope::ConstPtr();
    };

    m_baseType = findType(m_baseTypeName);
    m_attachedType = findType(m_attachedTypeName);

    for (auto it = m_properties.begin(), end = m_properties.end(); it != end; ++it)
        it->setType(findType(it->typeName()));

    for (auto it = m_methods.begin(), end = m_methods.end(); it != end; ++it) {
        it->setReturnType(findType(it->returnTypeName()));
        const auto paramNames = it->parameterTypeNames();
        QList<QSharedPointer<const QQmlJSScope>> paramTypes;

        for (const QString &paramName: paramNames)
            paramTypes.append(findType(paramName));

        it->setParameterTypes(paramTypes);
    }
}

QQmlJSScope::ConstPtr QQmlJSScope::findCurrentQMLScope(const QQmlJSScope::ConstPtr &scope)
{
    auto qmlScope = scope;
    while (qmlScope && qmlScope->m_scopeType != QQmlJSScope::QMLScope)
        qmlScope = qmlScope->parentScope();
    return qmlScope;
}

void QQmlJSScope::addExport(const QString &name, const QString &package,
                            const QTypeRevision &version)
{
    m_exports.append(Export(package, name, version));
}

bool QQmlJSScope::hasProperty(const QString &name) const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().data()) {
        if (scope->m_properties.contains(name))
            return true;
    }
    return false;
}

QQmlJSMetaProperty QQmlJSScope::property(const QString &name) const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().data()) {
        const auto it = scope->m_properties.find(name);
        if (it != scope->m_properties.end())
            return *it;
    }
    return {};
}

QQmlJSScope::Export::Export(QString package, QString type, const QTypeRevision &version) :
    m_package(std::move(package)),
    m_type(std::move(type)),
    m_version(version)
{
}

bool QQmlJSScope::Export::isValid() const
{
    return m_version.isValid() || !m_package.isEmpty() || !m_type.isEmpty();
}

QQmlJSScope QDeferredFactory<QQmlJSScope>::create() const
{
    QQmlJSTypeReader typeReader(m_importer, m_filePath);
    QQmlJSScope::Ptr result = typeReader();
    m_importer->m_warnings.append(typeReader.errors());
    result->setInternalName(QFileInfo(m_filePath).baseName());
    return std::move(*result);
}

QT_END_NAMESPACE
