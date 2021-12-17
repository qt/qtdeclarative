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

#include <private/qduplicatetracker_p.h>

#include <algorithm>
#include <type_traits>

QT_BEGIN_NAMESPACE

/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From.
*/
template<typename To, typename From, typename std::enable_if_t<!std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> From
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p;
}

/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From::get(), which is a raw pointer. The returned type is
    not necessary equal to To (e.g. To might be `QQmlJSScope *` while returned
    is `const QQmlJSScope *`).
*/
template<typename To, typename From, typename std::enable_if_t<std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> decltype(p.get())
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p.get();
}

template<typename QQmlJSScopePtr, typename Action>
static bool searchBaseAndExtensionTypes(QQmlJSScopePtr type, const Action &check)
{
    // NB: among other things, getQQmlJSScopeFromSmartPtr() also resolves const
    // vs non-const pointer issue, so use it's return value as the type
    using T = decltype(
            getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(std::declval<QQmlJSScope::ConstPtr>()));

    QDuplicateTracker<T> seen;
    for (T scope = type; scope && !seen.hasSeen(scope);
         scope = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(scope->baseType())) {
        // Extensions override their base types
        for (T extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(scope->extensionType());
             extension && !seen.hasSeen(extension);
             extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(extension->baseType())) {
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

/*!
    Returns if assigning to a property of this type would cause
    implicit component wrapping for non-Component types.

    \note This method can also be used to check whether a type needs
    to be implicitly wrapped: A type for which this function returns true
    doesn't need to be actually wrapped.
 */
bool QQmlJSScope::causesImplicitComponentWrapping() const {
    if (internalName() == u"QQmlComponent")
        return true;
    else if (isComposite()) // composite types are never treated as Component
        return false;
    // A class which is derived from component is not treated as a Component
    // However isUsableComponent considers also QQmlAbstractDelegateComponent
    // See isUsableComponent in qqmltypecompiler.cpp

    for (auto cppBase = nonCompositeBaseType(baseType()); cppBase; cppBase = cppBase->baseType())
        if (cppBase->internalName() == u"QQmlAbstractDelegateComponent")
            return true;
    return false;
}

/*!
    \internal
    Returns true if the scope is the outermost element of a separate Component
    Either because it has been implicitly wrapped, e.g. due to an assignment to
    a Component property, or because it is the first (and only) child of a
    Component.
 */
bool QQmlJSScope::isComponentRootElement() const {
    if (m_flags.testFlag(WrappedInImplicitComponent))
        return true;
    auto base = nonCompositeBaseType(parentScope()); // handles null parentScope()
    if (!base)
        return false;
    return base->internalName() == u"QQmlComponent";
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

QQmlJSScope::ImportedScope<QQmlJSScope::ConstPtr> QQmlJSScope::findType(
        const QString &name, const QQmlJSScope::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    auto type = contextualTypes.constFind(name);

    if (type != contextualTypes.constEnd()) {
        if (usedTypes != nullptr)
            usedTypes->insert(name);
        return *type;
    }

    const auto colonColon = name.indexOf(QStringLiteral("::"));
    if (colonColon > 0) {
        const QString outerTypeName = name.left(colonColon);
        const auto outerType = contextualTypes.constFind(outerTypeName);
        if (outerType != contextualTypes.constEnd()) {
            for (const auto &innerType : qAsConst(outerType->scope->m_childScopes)) {
                if (innerType->m_internalName == name) {
                    if (usedTypes != nullptr)
                        usedTypes->insert(name);
                    return { innerType, outerType->revision };
                }
            }
        }
    }

    return {};
}

QTypeRevision QQmlJSScope::resolveType(
        const QQmlJSScope::Ptr &self, const QQmlJSScope::ContextualTypes &context,
        QSet<QString> *usedTypes)
{
    const auto baseType = findType(self->m_baseTypeName, context, usedTypes);
    if (!self->m_baseType.scope && !self->m_baseTypeName.isEmpty())
        self->m_baseType = { baseType.scope, baseType.revision };

    if (!self->m_attachedType && !self->m_attachedTypeName.isEmpty())
        self->m_attachedType = findType(self->m_attachedTypeName, context, usedTypes).scope;

    if (!self->m_valueType && !self->m_valueTypeName.isEmpty())
        self->m_valueType = findType(self->m_valueTypeName, context, usedTypes).scope;

    if (!self->m_extensionType && !self->m_extensionTypeName.isEmpty())
        self->m_extensionType = findType(self->m_extensionTypeName, context, usedTypes).scope;

    for (auto it = self->m_properties.begin(), end = self->m_properties.end(); it != end; ++it) {
        const QString typeName = it->typeName();
        if (it->type() || typeName.isEmpty())
            continue;

        if (const auto type = findType(typeName, context, usedTypes); type.scope) {
            it->setType(type.scope);
            continue;
        }

        const auto enumeration = self->m_enumerations.find(typeName);
        if (enumeration != self->m_enumerations.end())
            it->setType(enumeration->type());
    }

    for (auto it = self->m_methods.begin(), end = self->m_methods.end(); it != end; ++it) {
        const QString returnTypeName = it->returnTypeName();
        if (!it->returnType() && !returnTypeName.isEmpty()) {
            const auto returnType = findType(returnTypeName, context, usedTypes);
            it->setReturnType(returnType.scope);
        }

        const auto paramTypeNames = it->parameterTypeNames();
        QList<QSharedPointer<const QQmlJSScope>> paramTypes = it->parameterTypes();
        if (paramTypes.length() < paramTypeNames.length())
            paramTypes.resize(paramTypeNames.length());

        for (int i = 0, length = paramTypes.length(); i < length; ++i) {
            auto &paramType = paramTypes[i];
            const auto paramTypeName = paramTypeNames[i];
            if (!paramType && !paramTypeName.isEmpty()) {
                const auto type = findType(paramTypeName, context, usedTypes);
                paramType = type.scope;
            }
        }

        it->setParameterTypes(paramTypes);
    }

    return baseType.revision;
}

void QQmlJSScope::updateChildScope(
        const QQmlJSScope::Ptr &childScope, const QQmlJSScope::Ptr &self,
        const QQmlJSScope::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
{
    switch (childScope->scopeType()) {
    case QQmlJSScope::GroupedPropertyScope:
        searchBaseAndExtensionTypes(self.data(), [&](const QQmlJSScope *type) {
            const auto propertyIt = type->m_properties.find(childScope->internalName());
            if (propertyIt != type->m_properties.end()) {
                childScope->m_baseType.scope = QQmlJSScope::ConstPtr(propertyIt->type());
                childScope->m_baseTypeName = propertyIt->typeName();
                return true;
            }
            return false;
        });
        break;
    case QQmlJSScope::AttachedPropertyScope:
        if (const auto attachedBase = findType(
                    childScope->internalName(), contextualTypes, usedTypes).scope) {
            childScope->m_baseType.scope = attachedBase->attachedType();
            childScope->m_baseTypeName = attachedBase->attachedTypeName();
        }
        break;
    default:
        break;
    }
}

template<typename Resolver, typename ChildScopeUpdater>
static QTypeRevision resolveTypesInternal(
        Resolver resolve, ChildScopeUpdater update, const QQmlJSScope::Ptr &self,
        const QQmlJSScope::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
{
    const QTypeRevision revision = resolve(self, contextualTypes, usedTypes);
    // NB: constness ensures no detach
    const auto childScopes = self->childScopes();
    for (auto it = childScopes.begin(), end = childScopes.end(); it != end; ++it) {
        const auto childScope = *it;
        update(childScope, self, contextualTypes, usedTypes);
        resolveTypesInternal(resolve, update, childScope, contextualTypes, usedTypes); // recursion
    }
    return revision;
}

QTypeRevision QQmlJSScope::resolveTypes(
        const QQmlJSScope::Ptr &self, const QQmlJSScope::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    const auto resolveAll = [](const QQmlJSScope::Ptr &self,
                               const QQmlJSScope::ContextualTypes &contextualTypes,
                               QSet<QString> *usedTypes) {
        resolveEnums(self, findType(u"int"_qs, contextualTypes, usedTypes).scope);
        return resolveType(self, contextualTypes, usedTypes);
    };
    return resolveTypesInternal(resolveAll, updateChildScope, self, contextualTypes, usedTypes);
}

void QQmlJSScope::resolveNonEnumTypes(
        const QQmlJSScope::Ptr &self, const QQmlJSScope::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    resolveTypesInternal(resolveType, updateChildScope, self, contextualTypes, usedTypes);
}

void QQmlJSScope::resolveEnums(const QQmlJSScope::Ptr &self, const QQmlJSScope::ConstPtr &intType)
{
    Q_ASSERT(intType); // There always has to be a builtin "int" type
    for (auto it = self->m_enumerations.begin(), end = self->m_enumerations.end(); it != end;
         ++it) {
        if (it->type())
            continue;
        auto enumScope = QQmlJSScope::create(EnumScope, self);
        enumScope->m_baseTypeName = QStringLiteral("int");
        enumScope->m_baseType.scope = intType;
        enumScope->m_semantics = AccessSemantics::Value;
        enumScope->m_internalName = self->internalName() + QStringLiteral("::") + it->name();
        it->setType(QQmlJSScope::ConstPtr(enumScope));
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
                            const QTypeRevision &version, const QTypeRevision &revision)
{
    m_exports.append(Export(package, name, version, revision));
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

QQmlJSScope::ConstPtr QQmlJSScope::ownerOfProperty(const QQmlJSScope::ConstPtr &self,
                                                   const QString &name)
{
    QQmlJSScope::ConstPtr owner;
    searchBaseAndExtensionTypes(self, [&](const QQmlJSScope::ConstPtr &scope) {
        if (scope->hasOwnProperty(name)) {
            owner = scope;
            return true;
        }
        return false;
    });
    return owner;
}

void QQmlJSScope::setPropertyLocallyRequired(const QString &name, bool isRequired)
{
    if (!isRequired)
        m_requiredPropertyNames.removeOne(name);
    else if (!m_requiredPropertyNames.contains(name))
        m_requiredPropertyNames.append(name);
}

bool QQmlJSScope::isPropertyRequired(const QString &name) const
{
    bool isRequired = false;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        if (scope->isPropertyLocallyRequired(name)) {
            isRequired = true;
            return true;
        }

        // If it has a property of that name, and that is not required, then none of the
        // base types matter. You cannot make a derived type's property required with
        // a "required" specification in a base type.
        return scope->hasOwnProperty(name);
    });
    return isRequired;
}

bool QQmlJSScope::isPropertyLocallyRequired(const QString &name) const
{
    return m_requiredPropertyNames.contains(name);
}

bool QQmlJSScope::hasPropertyBinding(const QString &name) const
{
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        return scope->m_propertyBindings.contains(name);
    });
}

QQmlJSMetaPropertyBinding QQmlJSScope::propertyBinding(const QString &name) const
{
    QQmlJSMetaPropertyBinding binding;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const auto it = scope->m_propertyBindings.find(name);
        if (it == scope->m_propertyBindings.end())
            return false;
        binding = *it;
        return true;
    });
    return binding;
}

bool QQmlJSScope::hasInterface(const QString &name) const
{
    return searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope) { return scope->m_interfaceNames.contains(name); });
}

QString QQmlJSScope::attachedTypeName() const
{
    QString name;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        if (scope->ownAttachedType().isNull())
            return false;
        name = scope->ownAttachedTypeName();
        return true;
    });

    return name;
}

QQmlJSScope::ConstPtr QQmlJSScope::attachedType() const
{
    QQmlJSScope::ConstPtr ptr;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        if (scope->ownAttachedType().isNull())
            return false;
        ptr = scope->ownAttachedType();
        return true;
    });

    return ptr;
}

bool QQmlJSScope::isResolved() const
{
    if (m_scopeType == ScopeType::AttachedPropertyScope
        || m_scopeType == ScopeType::GroupedPropertyScope) {
        return m_internalName.isEmpty() || !m_baseType.scope.isNull();
    }

    return m_baseTypeName.isEmpty() || !m_baseType.scope.isNull();
}

bool QQmlJSScope::isFullyResolved() const
{
    bool baseResolved = true;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        if (!scope->isResolved()) {
            baseResolved = false;
            return true;
        }
        return false;
    });

    return baseResolved;
}

QQmlJSScope::Export::Export(
        QString package, QString type, QTypeRevision version, QTypeRevision revision) :
    m_package(std::move(package)),
    m_type(std::move(type)),
    m_version(std::move(version)),
    m_revision(std::move(revision))
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

bool QQmlJSScope::canAssign(const QQmlJSScope::ConstPtr &derived) const
{
    if (!derived)
        return false;

    bool isBaseComponent = causesImplicitComponentWrapping();

    QDuplicateTracker<QQmlJSScope::ConstPtr> seen;
    for (auto scope = derived; !scope.isNull() && !seen.hasSeen(scope);
         scope = scope->baseType()) {
        if (isSameType(scope))
            return true;
        if (isBaseComponent && scope->internalName() == u"QObject"_qs)
            return true;
    }

    return internalName() == u"QVariant"_qs || internalName() == u"QJSValue"_qs;
}

bool QQmlJSScope::isInCustomParserParent() const
{
    for (const auto *scope = this; scope; scope = scope->parentScope().get()) {
        if (!scope->baseType().isNull() && scope->baseType()->hasCustomParser())
            return true;
    }

    return false;
}

QT_END_NAMESPACE
