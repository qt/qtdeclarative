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

QQmlJSScope::Ptr QQmlJSScope::clone(const ConstPtr &origin)
{
    if (origin.isNull())
        return QQmlJSScope::Ptr();
    QQmlJSScope::Ptr cloned = create(origin->m_scopeType, origin->m_parentScope);
    *cloned = *origin;
    return cloned;
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
    QQmlJSMetaMethod method(property.propertyName() + u"Changed"_qs, u"void"_qs);
    method.setMethodType(QQmlJSMetaMethod::Signal);
    method.setIsImplicitQmlPropertyChangeSignal(true);
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

/*!
    Returns all methods visible from this scope including those of
    base types and extensions.

    \note Methods that get shadowed are not included and only the
    version visible from this scope is contained. Additionally method
    overrides are not included either, only the first visible version
    of any method is included.
*/
QHash<QString, QQmlJSMetaMethod> QQmlJSScope::methods() const
{
    QHash<QString, QQmlJSMetaMethod> results;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        for (auto it = scope->m_methods.constBegin(); it != scope->m_methods.constEnd(); it++) {
            if (!results.contains(it.key()))
                results.insert(it.key(), it.value());
        }
        return false;
    });

    return results;
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

QList<QQmlJSMetaMethod> QQmlJSScope::methods(const QString &name, QQmlJSMetaMethod::Type type) const
{
    QList<QQmlJSMetaMethod> results;

    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const auto ownMethods = scope->ownMethods(name);
        for (const auto &method : ownMethods) {
            if (method.methodType() == type)
                results.append(method);
        }
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

QHash<QString, QQmlJSMetaEnum> QQmlJSScope::enumerations() const
{
    QHash<QString, QQmlJSMetaEnum> results;

    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        for (auto it = scope->m_enumerations.constBegin(); it != scope->m_enumerations.constEnd();
             it++) {
            if (!results.contains(it.key()))
                results.insert(it.key(), it.value());
        }
        return false;
    });

    return results;
}

/*!
    Returns if assigning \a assignedType to \a property would require an
    implicit component wrapping.
 */
bool QQmlJSScope::causesImplicitComponentWrapping(const QQmlJSMetaProperty &property,
                                                  const QQmlJSScope::ConstPtr &assignedType)
{
    // See QQmlComponentAndAliasResolver::findAndRegisterImplicitComponents()
    // for the logic in qqmltypecompiler

    // Note: unlike findAndRegisterImplicitComponents() we do not check whether
    // the property type is *derived* from QQmlComponent at some point because
    // this is actually meaningless (and in the case of QQmlComponent::create()
    // gets rejected in QQmlPropertyValidator): if the type is not a
    // QQmlComponent, we have a type mismatch because of assigning a Component
    // object to a non-Component property
    const bool propertyVerdict = property.type()->internalName() == u"QQmlComponent";

    const bool assignedTypeVerdict = [&assignedType]() {
        // Note: nonCompositeBaseType covers the case when assignedType itself
        // is non-composite
        auto cppBase = nonCompositeBaseType(assignedType);
        Q_ASSERT(cppBase); // any QML type has (or must have) a C++ base type

        // See isUsableComponent() in qqmltypecompiler.cpp: along with checking
        // whether a type has a QQmlComponent static meta object (which we
        // substitute here with checking the first non-composite base for being
        // a QQmlComponent), it also excludes QQmlAbstractDelegateComponent
        // subclasses from implicit wrapping
        if (cppBase->internalName() == u"QQmlComponent")
            return false;
        for (; cppBase; cppBase = cppBase->baseType()) {
            if (cppBase->internalName() == u"QQmlAbstractDelegateComponent")
                return false;
        }
        return true;
    }();

    return propertyVerdict && assignedTypeVerdict;
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

    const auto colonColon = name.lastIndexOf(QStringLiteral("::"));
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
    for (auto it = self->m_enumerations.begin(), end = self->m_enumerations.end(); it != end;
         ++it) {
        if (it->type())
            continue;
        Q_ASSERT(intType); // We need an "int" type to resolve enums
        auto enumScope = QQmlJSScope::create(EnumScope, self);
        enumScope->m_baseTypeName = QStringLiteral("int");
        enumScope->m_baseType.scope = intType;
        enumScope->m_semantics = AccessSemantics::Value;
        enumScope->m_internalName = self->internalName() + QStringLiteral("::") + it->name();
        it->setType(QQmlJSScope::ConstPtr(enumScope));
    }
}

void QQmlJSScope::resolveGeneralizedGroup(
        const Ptr &self, const ConstPtr &baseType,
        const QQmlJSScope::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
{
    // Generalized group properties are always composite,
    // which means we expect contextualTypes to be QML names.
    Q_ASSERT(self->isComposite());

    self->m_baseType.scope = baseType;
    resolveNonEnumTypes(self, contextualTypes, usedTypes);
}

QQmlJSScope::ConstPtr QQmlJSScope::findCurrentQMLScope(const QQmlJSScope::ConstPtr &scope)
{
    auto qmlScope = scope;
    while (qmlScope && qmlScope->m_scopeType != QQmlJSScope::QMLScope)
        qmlScope = qmlScope->parentScope();
    return qmlScope;
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

/*!
    Returns all properties visible from this scope including those of
    base types and extensions.

    \note Properties that get shadowed are not included and only the
    version visible from this scope is contained.
*/
QHash<QString, QQmlJSMetaProperty> QQmlJSScope::properties() const
{
    QHash<QString, QQmlJSMetaProperty> results;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        for (auto it = scope->m_properties.constBegin(); it != scope->m_properties.constEnd();
             it++) {
            if (!results.contains(it.key()))
                results.insert(it.key(), it.value());
        }
        return false;
    });
    return results;
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

static_assert(QTypeInfo<QQmlJSScope::QmlIRCompatibilityBindingData>::isRelocatable,
              "We really want T to be relocatable as it improves QList<T> performance");

void QQmlJSScope::addOwnPropertyBindingInQmlIROrder(const QQmlJSMetaPropertyBinding &binding,
                                                    BindingTargetSpecifier specifier)
{
    // the order:
    // * ordinary bindings are prepended to the binding array
    // * list bindings are properly ordered within each other, so basically
    //   prepended "in bulk"
    // * bindings to default properties (which are not explicitly mentioned in
    //   binding expression) are inserted by source location's offset

    switch (specifier) {
    case BindingTargetSpecifier::SimplePropertyTarget: {
        m_propertyBindingsArray.emplaceFront(binding.propertyName(),
                                             binding.sourceLocation().offset);
        break;
    }
    case BindingTargetSpecifier::ListPropertyTarget: {
        const auto bindingOnTheSameProperty =
                [&](const QQmlJSScope::QmlIRCompatibilityBindingData &x) {
                    return x.propertyName == binding.propertyName();
                };
        // fake "prepend in bulk" by appending a list binding to the sequence of
        // bindings to the same property. there's an implicit QML language
        // guarantee that such sequence does not contain arbitrary in-between
        // bindings that do not belong to the same list property
        auto pos = std::find_if_not(m_propertyBindingsArray.begin(), m_propertyBindingsArray.end(),
                                    bindingOnTheSameProperty);
        Q_ASSERT(pos == m_propertyBindingsArray.begin()
                 || std::prev(pos)->propertyName == binding.propertyName());
        m_propertyBindingsArray.emplace(pos, binding.propertyName(),
                                        binding.sourceLocation().offset);
        break;
    }
    case BindingTargetSpecifier::UnnamedPropertyTarget: {
        // see QmlIR::PoolList<>::findSortedInsertionPoint()
        const auto findInsertionPoint = [this](const QQmlJSMetaPropertyBinding &x) {
            qsizetype pos = -1;
            for (auto it = m_propertyBindingsArray.cbegin(); it != m_propertyBindingsArray.cend();
                 ++it) {
                if (!(it->sourceLocationOffset <= x.sourceLocation().offset))
                    break;
                ++pos;
            }
            return pos;
        };

        // see QmlIR::PoolList<>::insertAfter()
        const auto insertAfter = [this](qsizetype pos, const QQmlJSMetaPropertyBinding &x) {
            if (pos == -1) {
                m_propertyBindingsArray.emplaceFront(x.propertyName(), x.sourceLocation().offset);
            } else if (pos == m_propertyBindingsArray.size()) {
                m_propertyBindingsArray.emplaceBack(x.propertyName(), x.sourceLocation().offset);
            } else {
                // since we insert *after*, use (pos + 1) as insertion point
                m_propertyBindingsArray.emplace(pos + 1, x.propertyName(),
                                                x.sourceLocation().offset);
            }
        };

        const qsizetype insertionPos = findInsertionPoint(binding);
        insertAfter(insertionPos, binding);
        break;
    }
    default: {
        Q_UNREACHABLE();
        break;
    }
    }
}

QList<QQmlJSMetaPropertyBinding> QQmlJSScope::ownPropertyBindingsInQmlIROrder() const
{
    QList<QQmlJSMetaPropertyBinding> qmlIrOrdered;
    qmlIrOrdered.reserve(m_propertyBindingsArray.size());

    for (const auto &data : m_propertyBindingsArray) {
        const auto [first, last] = m_propertyBindings.equal_range(data.propertyName);
        Q_ASSERT(first != last);
        auto binding = std::find_if(first, last, [&](const QQmlJSMetaPropertyBinding &x) {
            return x.sourceLocation().offset == data.sourceLocationOffset;
        });
        Q_ASSERT(binding != last);
        qmlIrOrdered.append(*binding);
    }

    return qmlIrOrdered;
}

bool QQmlJSScope::hasPropertyBindings(const QString &name) const
{
    return searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        return scope->hasOwnPropertyBindings(name);
    });
}

QList<QQmlJSMetaPropertyBinding> QQmlJSScope::propertyBindings(const QString &name) const
{
    QList<QQmlJSMetaPropertyBinding> bindings;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const auto range = scope->ownPropertyBindings(name);
        for (auto it = range.first; it != range.second; ++it)
            bindings.append(*it);
        return false;
    });
    return bindings;
}

bool QQmlJSScope::hasInterface(const QString &name) const
{
    return searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope) { return scope->m_interfaceNames.contains(name); });
}

bool QQmlJSScope::isNameDeferred(const QString &name) const
{
    bool isDeferred = false;

    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        const QStringList immediate = scope->ownImmediateNames();
        if (!immediate.isEmpty()) {
            isDeferred = !immediate.contains(name);
            return true;
        }

        const QStringList deferred = scope->ownDeferredNames();
        if (!deferred.isEmpty()) {
            isDeferred = deferred.contains(name);
            return true;
        }

        return false;
    });

    return isDeferred;
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
    const bool nameIsEmpty = (m_scopeType == ScopeType::AttachedPropertyScope
                              || m_scopeType == ScopeType::GroupedPropertyScope)
            ? m_internalName.isEmpty()
            : m_baseTypeName.isEmpty();
    if (nameIsEmpty)
        return true;
    if (m_baseType.scope.isNull())
        return false;
    if (isComposite() && !nonCompositeBaseType(baseType()))
        return false;
    return true;
}

QString QQmlJSScope::defaultPropertyName() const
{
    QString name;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        name = scope->ownDefaultPropertyName();
        return !name.isEmpty();
    });
    return name;
}

QString QQmlJSScope::parentPropertyName() const
{
    QString name;
    searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        name = scope->ownParentPropertyName();
        return !name.isEmpty();
    });
    return name;
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

QQmlJSScope::Import::Import(QString prefix, QString name, QTypeRevision version, bool isFile,
                            bool isDependency) :
    m_prefix(std::move(prefix)),
    m_name(std::move(name)),
    m_version(version),
    m_isFile(isFile),
    m_isDependency(isDependency)
{
}

bool QQmlJSScope::Import::isValid() const
{
    return !m_name.isEmpty();
}

QQmlJSScope::Export::Export(
        QString package, QString type, QTypeRevision version, QTypeRevision revision)
    : m_package(std::move(package))
    , m_type(std::move(type))
    , m_version(std::move(version))
    , m_revision(std::move(revision))
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
    m_importer->m_globalWarnings.append(typeReader.errors());
    result->setInternalName(internalName());
    QQmlJSScope::resolveEnums(result, m_importer->builtinInternalNames().value(u"int"_qs).scope);

    if (m_isSingleton && !result->isSingleton()) {
        m_importer->m_globalWarnings.append(
                { QStringLiteral(
                          "Type %1 declared as singleton in qmldir but missing pragma Singleton")
                          .arg(result->internalName()),
                  QtCriticalMsg, QQmlJS::SourceLocation() });
        result->setIsSingleton(true);
    } else if (!m_isSingleton && result->isSingleton()) {
        m_importer->m_globalWarnings.append(
                { QStringLiteral("Type %1 not declared as singleton in qmldir "
                                 "but using pragma Singleton")
                          .arg(result->internalName()),
                  QtCriticalMsg, QQmlJS::SourceLocation() });
        result->setIsSingleton(false);
    }

    return std::move(*result);
}

bool QQmlJSScope::canAssign(const QQmlJSScope::ConstPtr &derived) const
{
    if (!derived)
        return false;

    // expect this and derived types to have non-composite bases
    Q_ASSERT(!isComposite() || nonCompositeBaseType(baseType()));
    Q_ASSERT(nonCompositeBaseType(derived));

    // the logic with isBaseComponent (as well as the way we set this flag)
    // feels wrong - QTBUG-101940
    const bool isBaseComponent = [this]() {
        if (internalName() == u"QQmlComponent")
            return true;
        else if (isComposite())
            return false;
        for (auto cppBase = nonCompositeBaseType(baseType()); cppBase;
             cppBase = cppBase->baseType()) {
            if (cppBase->internalName() == u"QQmlAbstractDelegateComponent")
                return true;
        }
        return false;
    }();

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
