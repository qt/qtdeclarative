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

template<typename Action>
static bool searchBaseAndExtensionTypes(const QQmlJSScope *type, const Action &check)
{
    for (const QQmlJSScope *scope = type; scope; scope = scope->baseType().data()) {
        // Extensions override their base types
        for (const QQmlJSScope *extension = scope->extensionType().data(); extension;
             extension = extension->baseType().data()) {
            if (check(extension))
                return true;
        }

        if (check(scope))
            return true;
    }

    return false;
}

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
    return isIdInCurrentQmlScopes(id) || isIdInCurrentJSScopes(id);
}

bool QQmlJSScope::hasMethod(const QString &name) const
{
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        return scope->m_methods.contains(name);
    });
}

QList<QQmlJSMetaMethod> QQmlJSScope::methods(const QString &name) const
{
    QList<QQmlJSMetaMethod> results;

    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        results.append(scope->ownMethods(name));
        return false;
    });
    return results;
}

bool QQmlJSScope::hasEnumeration(const QString &name) const
{
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        return scope->m_enumerations.contains(name);
    });
}

bool QQmlJSScope::hasEnumerationKey(const QString &name) const
{
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        for (const auto &e : scope->m_enumerations) {
            if (e.keys().contains(name))
                return true;
        }
        return false;
    });
}

QQmlJSMetaEnum QQmlJSScope::enumeration(const QString &name) const
{
    QQmlJSMetaEnum result;

    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const auto it = scope->m_enumerations.find(name);
        if (it == scope->m_enumerations.end())
            return false;
        result = *it;
        return true;
    });

    return result;
}

bool QQmlJSScope::isIdInCurrentQmlScopes(const QString &id) const
{
    if (m_scopeType == QQmlJSScope::QMLScope)
        return m_properties.contains(id) || m_methods.contains(id) || m_enumerations.contains(id);

    const auto qmlScope = findCurrentQMLScope(parentScope());
    return qmlScope->m_properties.contains(id)
            || qmlScope->m_methods.contains(id)
            || qmlScope->m_enumerations.contains(id);
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

void QQmlJSScope::resolveTypes(const QQmlJSScope::Ptr &self,
                               const QHash<QString, QQmlJSScope::ConstPtr> &contextualTypes)
{
    auto findType = [&](const QString &name) -> QQmlJSScope::ConstPtr {
        auto type = contextualTypes.constFind(name);
        if (type != contextualTypes.constEnd())
            return *type;

        const auto colonColon = name.indexOf(QStringLiteral("::"));
        if (colonColon > 0) {
            const auto outerType = contextualTypes.constFind(name.left(colonColon));
            if (outerType != contextualTypes.constEnd()) {
                for (const auto &innerType : qAsConst((*outerType)->m_childScopes)) {
                    if (innerType->m_internalName == name)
                        return innerType;
                }
            }
        }

        return QQmlJSScope::ConstPtr();
    };

    if (!self->m_baseType && !self->m_baseTypeName.isEmpty())
        self->m_baseType = findType(self->m_baseTypeName);

    if (!self->m_attachedType && !self->m_attachedTypeName.isEmpty())
        self->m_attachedType = findType(self->m_attachedTypeName);

    if (!self->m_valueType && !self->m_valueTypeName.isEmpty())
        self->m_valueType = findType(self->m_valueTypeName);

    if (!self->m_extensionType && !self->m_extensionTypeName.isEmpty())
        self->m_extensionType = findType(self->m_extensionTypeName);

    const auto intType = findType(QStringLiteral("int"));
    Q_ASSERT(intType); // There always has to be a builtin "int" type
    for (auto it = self->m_enumerations.begin(), end = self->m_enumerations.end();
         it != end; ++it) {
        if (it->type())
            continue;
        auto enumScope = QQmlJSScope::create(EnumScope, self);
        enumScope->m_baseTypeName = QStringLiteral("int");
        enumScope->m_baseType = intType;
        enumScope->m_semantics = AccessSemantics::Value;
        enumScope->m_internalName = self->internalName() + QStringLiteral("::") + it->name();
        it->setType(ConstPtr(enumScope));
    }

    for (auto it = self->m_properties.begin(), end = self->m_properties.end(); it != end; ++it) {
        const QString typeName = it->typeName();
        if (it->type() || typeName.isEmpty())
            continue;

        if (const auto type = findType(typeName)) {
            it->setType(type);
            continue;
        }

        const auto enumeration = self->m_enumerations.find(typeName);
        if (enumeration != self->m_enumerations.end())
            it->setType(enumeration->type());
    }

    for (auto it = self->m_methods.begin(), end = self->m_methods.end(); it != end; ++it) {
        const QString returnTypeName = it->returnTypeName();
        if (!it->returnType() && !returnTypeName.isEmpty())
            it->setReturnType(findType(returnTypeName));

        const auto paramTypeNames = it->parameterTypeNames();
        QList<QSharedPointer<const QQmlJSScope>> paramTypes = it->parameterTypes();
        if (paramTypes.length() < paramTypeNames.length())
            paramTypes.resize(paramTypeNames.length());

        for (int i = 0, length = paramTypes.length(); i < length; ++i) {
            auto &paramType = paramTypes[i];
            const auto paramTypeName = paramTypeNames[i];
            if (!paramType && !paramTypeName.isEmpty())
                paramType = findType(paramTypeName);
        }

        it->setParameterTypes(paramTypes);
    }

    for (auto it = self->m_childScopes.begin(), end = self->m_childScopes.end(); it != end; ++it) {
        QQmlJSScope::Ptr childScope = *it;
        switch (childScope->scopeType()) {
        case QQmlJSScope::GroupedPropertyScope:
            searchBaseAndExtensionTypes(self.data(), [&](const QQmlJSScope *type) {
                const auto propertyIt = type->m_properties.find(childScope->internalName());
                if (propertyIt != type->m_properties.end()) {
                    childScope->m_baseType = QQmlJSScope::ConstPtr(propertyIt->type());
                    childScope->m_baseTypeName = propertyIt->typeName();
                    return true;
                }
                return false;
            });
            break;
        case QQmlJSScope::AttachedPropertyScope:
            if (const auto attachedBase = findType(childScope->internalName())) {
                childScope->m_baseType = attachedBase->attachedType();
                childScope->m_baseTypeName = attachedBase->attachedTypeName();
            }
            break;
        default:
            break;
        }
        resolveTypes(childScope, contextualTypes);
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
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        return scope->m_properties.contains(name);
    });
}

QQmlJSMetaProperty QQmlJSScope::property(const QString &name) const
{
    QQmlJSMetaProperty prop;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const auto it = scope->m_properties.find(name);
        if (it == scope->m_properties.end())
            return false;
        prop = *it;
        return true;
    });
    return prop;
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
