// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscontextualtypes_p.h"
#include "qqmljsscope_p.h"
#include "qqmljstypereader_p.h"
#include "qqmljsimporter_p.h"
#include "qqmljsutils_p.h"
#include "qqmlsa.h"
#include "qqmlsa_p.h"

#include <QtCore/qqueue.h>
#include <QtCore/qsharedpointer.h>

#include <private/qduplicatetracker_p.h>

#include <algorithm>
#include <type_traits>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlJSScope
    \internal
    \brief Tracks the types for the QmlCompiler

    QQmlJSScope tracks the types used in qml for the QmlCompiler.

    Multiple QQmlJSScope objects might be created for the same conceptual type, except when reused
    due to extensive caching. Two QQmlJSScope objects are considered equal when they are backed
    by the same implementation, that is, they have the same internalName.
*/

using namespace Qt::StringLiterals;

QQmlJSScope::QQmlJSScope(const QString &internalName) : QQmlJSScope{}
{
    m_internalName = internalName;
}

QQmlJSScope::Ptr QQmlJSScope::create(const QString &internalName)
{
    return QSharedPointer<QQmlJSScope>(new QQmlJSScope(internalName));
}

void QQmlJSScope::reparent(const QQmlJSScope::Ptr &parentScope, const QQmlJSScope::Ptr &childScope)
{
    if (const QQmlJSScope::Ptr parent = childScope->m_parentScope.toStrongRef())
        parent->m_childScopes.removeOne(childScope);
    if (parentScope)
        parentScope->m_childScopes.append(childScope);
    childScope->m_parentScope = parentScope;
}

QQmlJSScope::Ptr QQmlJSScope::clone(const ConstPtr &origin)
{
    if (origin.isNull())
        return QQmlJSScope::Ptr();
    QQmlJSScope::Ptr cloned = create();
    *cloned = *origin;
    if (QQmlJSScope::Ptr parent = cloned->parentScope())
        parent->m_childScopes.append(cloned);
    return cloned;
}

/*!
\internal
Return all the JavaScript identifiers defined in the current scope.
*/
QHash<QString, QQmlJSScope::JavaScriptIdentifier> QQmlJSScope::ownJSIdentifiers() const
{
    return m_jsIdentifiers;
}

void QQmlJSScope::insertJSIdentifier(const QString &name, const JavaScriptIdentifier &identifier)
{
    Q_ASSERT(m_scopeType != QQmlSA::ScopeType::QMLScope);
    if (identifier.kind == JavaScriptIdentifier::LexicalScoped
        || identifier.kind == JavaScriptIdentifier::Injected
        || m_scopeType == QQmlSA::ScopeType::JSFunctionScope) {
        m_jsIdentifiers.insert(name, identifier);
    } else {
        auto targetScope = parentScope();
        while (targetScope->m_scopeType != QQmlSA::ScopeType::JSFunctionScope)
            targetScope = targetScope->parentScope();
        targetScope->m_jsIdentifiers.insert(name, identifier);
    }
}

void QQmlJSScope::insertPropertyIdentifier(const QQmlJSMetaProperty &property)
{
    addOwnProperty(property);
    QQmlJSMetaMethod method(
            QQmlSignalNames::propertyNameToChangedSignalName(property.propertyName()), u"void"_s);
    method.setMethodType(QQmlJSMetaMethodType::Signal);
    method.setIsImplicitQmlPropertyChangeSignal(true);
    addOwnMethod(method);
}

bool QQmlJSScope::hasMethod(const QString &name) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
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
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                for (auto it = scope->m_methods.constBegin(); it != scope->m_methods.constEnd();
                     it++) {
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

    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                results.append(scope->ownMethods(name));
                return false;
            });
    return results;
}

QList<QQmlJSMetaMethod> QQmlJSScope::methods(const QString &name, QQmlJSMetaMethodType type) const
{
    QList<QQmlJSMetaMethod> results;

    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
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
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope) { return scope->m_enumerations.contains(name); });
}

bool QQmlJSScope::hasOwnEnumerationKey(const QString &name) const
{
    for (const auto &e : m_enumerations) {
        if (e.keys().contains(name))
            return true;
    }
    return false;
}

bool QQmlJSScope::hasEnumerationKey(const QString &name) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope) { return scope->hasOwnEnumerationKey(name); });
}

QQmlJSMetaEnum QQmlJSScope::enumeration(const QString &name) const
{
    QQmlJSMetaEnum result;

    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
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

    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        for (auto it = scope->m_enumerations.constBegin(); it != scope->m_enumerations.constEnd();
             it++) {
            if (!results.contains(it.key()))
                results.insert(it.key(), it.value());
        }
        return false;
    });

    return results;
}

QString QQmlJSScope::augmentedInternalName() const
{
    using namespace Qt::StringLiterals;
    Q_ASSERT(!m_internalName.isEmpty());

    switch (m_semantics) {
    case AccessSemantics::Reference:
        return m_internalName + " *"_L1;
    case AccessSemantics::Value:
    case AccessSemantics::Sequence:
        break;
    case AccessSemantics::None:
        // If we got a namespace, it might still be a regular type, exposed as namespace.
        // We may need to travel the inheritance chain all the way up to QObject to
        // figure this out, since all other types may be exposed the same way.
        for (QQmlJSScope::ConstPtr base = baseType(); base; base = base->baseType()) {
            switch (base->accessSemantics()) {
            case AccessSemantics::Reference:
                return m_internalName + " *"_L1;
            case AccessSemantics::Value:
            case AccessSemantics::Sequence:
                return m_internalName;
            case AccessSemantics::None:
                break;
            }
        }
        break;
    }
    return m_internalName;
}

QString QQmlJSScope::prettyName(QAnyStringView name)
{
    const auto internal = "$internal$."_L1;
    const QString anonymous = "$anonymous$."_L1;

    QString pretty = name.toString();

    if (pretty.startsWith(internal))
        pretty = pretty.mid(internal.size());
    else if (pretty.startsWith(anonymous))
        pretty = pretty.mid(anonymous.size());

    if (pretty == u"std::nullptr_t")
        return u"null"_s;

    if (pretty == u"void")
        return u"undefined"_s;

    return pretty;
}

/*!
    \internal
    Returns \c Yes if the scope is the outermost element of a separate Component
    Either because it has been implicitly wrapped, e.g. due to an assignment to
    a Component property, or because it is the first (and only) child of a
    Component.
    Returns \c No if we can clearly determine that this is not the case.
    Returns \c Maybe if the scope is assigned to an unknown property. This may
    or may not be a Component.
    For visitors: This method should only be called after implicit components
    are detected, that is, after QQmlJSImportVisitor::endVisit(UiProgram *)
    was called.
 */
QQmlJSScope::IsComponentRoot QQmlJSScope::componentRootStatus() const {
    if (m_flags.testFlag(WrappedInImplicitComponent))
        return IsComponentRoot::Yes;

    // If the object is assigned to an unknown property, assume it's Component.
    if (m_flags.testFlag(AssignedToUnknownProperty))
        return IsComponentRoot::Maybe;

    auto base = nonCompositeBaseType(parentScope()); // handles null parentScope()
    if (!base)
        return IsComponentRoot::No;
    return base->internalName() == u"QQmlComponent"
            ? IsComponentRoot::Yes
            : IsComponentRoot::No;
}

std::optional<QQmlJSScope::JavaScriptIdentifier>
QQmlJSScope::jsIdentifier(const QString &id) const
{
    for (const auto *scope = this; scope; scope = scope->parentScope().data()) {
        if (scope->m_scopeType == QQmlSA::ScopeType::JSFunctionScope
            || scope->m_scopeType == QQmlSA::ScopeType::JSLexicalScope) {
            auto it = scope->m_jsIdentifiers.find(id);
            if (it != scope->m_jsIdentifiers.end())
                return *it;
        }
    }

    return std::optional<JavaScriptIdentifier>{};
}

std::optional<QQmlJSScope::JavaScriptIdentifier> QQmlJSScope::ownJSIdentifier(const QString &id) const
{
    auto it = m_jsIdentifiers.find(id);
    if (it != m_jsIdentifiers.end())
        return *it;

    return std::optional<JavaScriptIdentifier>{};
}

static QQmlJSScope::ImportedScope<QQmlJSScope::ConstPtr>
qFindInlineComponents(QStringView typeName, const QQmlJS::ContextualTypes &contextualTypes)
{
    const int separatorIndex = typeName.lastIndexOf(u'.');
    // do not crash in typeName.sliced() when it starts or ends with an '.'.
    if (separatorIndex < 1 || separatorIndex >= typeName.size() - 1)
        return {};

    const auto parentIt = contextualTypes.types().constFind(typeName.first(separatorIndex).toString());
    if (parentIt == contextualTypes.types().constEnd())
        return {};

    auto inlineComponentParent = *parentIt;

    // find the inline components using BFS, as inline components defined in childrens are also
    // accessible from other qml documents. Same for inline components defined in a base class of
    // the parent. Use BFS over DFS as the inline components are probably not deeply-nested.

    QStringView inlineComponentName = typeName.sliced(separatorIndex + 1);
    QQueue<QQmlJSScope::ConstPtr> candidatesForInlineComponents;
    candidatesForInlineComponents.enqueue(inlineComponentParent.scope);
    while (candidatesForInlineComponents.size()) {
        QQmlJSScope::ConstPtr current = candidatesForInlineComponents.dequeue();
        if (!current) // if some type was not resolved, ignore it instead of crashing
            continue;
        if (current->isInlineComponent() && current->inlineComponentName() == inlineComponentName) {
            return { current, inlineComponentParent.revision };
        }
        // check alternatively the inline components at layer 1 in current and basetype, then at
        // layer 2, etc...
        candidatesForInlineComponents.append(current->childScopes());
        if (const auto base = current->baseType())
            candidatesForInlineComponents.enqueue(base);
    }
    return {};
}

/*! \internal
 *  Finds a type in contextualTypes with given name.
 *  If a type is found, then its name is inserted into usedTypes (when provided).
 *  If contextualTypes has mode INTERNAl, then namespace resolution for enums is
 *  done (eg for Qt::Alignment).
 *  If contextualTypes has mode QML, then inline component resolution is done
 *  ("qmlFileName.IC" is correctly resolved from qmlFileName).
 */
QQmlJSScope::ImportedScope<QQmlJSScope::ConstPtr> QQmlJSScope::findType(
        const QString &name, const QQmlJS::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    const auto useType = [&]() {
        if (usedTypes != nullptr)
            usedTypes->insert(name);
    };

    auto type = contextualTypes.types().constFind(name);

    if (type != contextualTypes.types().constEnd()) {
        useType();
        return *type;
    }

    const auto findListType = [&](const QString &prefix, const QString &postfix)
            -> ImportedScope<ConstPtr> {
        if (name.startsWith(prefix) && name.endsWith(postfix)) {
            const qsizetype prefixLength = prefix.length();
            const QString &elementName
                    = name.mid(prefixLength, name.length() - prefixLength - postfix.length());
            const ImportedScope<ConstPtr> element
                    = findType(elementName, contextualTypes, usedTypes);
            if (element.scope) {
                useType();
                return { element.scope->listType(), element.revision };
            }
        }

        return {};
    };

    switch (contextualTypes.context()) {
    case QQmlJS::ContextualTypes::INTERNAL: {
        if (const auto listType = findListType(u"QList<"_s, u">"_s);
                listType.scope && !listType.scope->isReferenceType()) {
            return listType;
        }

        if (const auto listType = findListType(u"QQmlListProperty<"_s, u">"_s);
                listType.scope && listType.scope->isReferenceType())  {
            return listType;
        }

        // look for c++ namescoped enums!
        const auto colonColon = name.lastIndexOf(QStringLiteral("::"));
        if (colonColon == -1)
            break;

        const QString outerTypeName = name.left(colonColon);
        const auto outerType = contextualTypes.types().constFind(outerTypeName);
        if (outerType == contextualTypes.types().constEnd())
            break;

        for (const auto &innerType : std::as_const(outerType->scope->m_childScopes)) {
            if (innerType->m_internalName == name) {
                useType();
                return { innerType, outerType->revision };
            }
        }

        break;
    }
    case QQmlJS::ContextualTypes::QML: {
        // look after inline components
        const auto inlineComponent = qFindInlineComponents(name, contextualTypes);
        if (inlineComponent.scope) {
            useType();
            return inlineComponent;
        }

        if (const auto listType = findListType(u"list<"_s, u">"_s); listType.scope)
            return listType;

        break;
    }
    }
    return {};
}

QTypeRevision QQmlJSScope::resolveType(
        const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &context,
        QSet<QString> *usedTypes)
{
    if (self->accessSemantics() == AccessSemantics::Sequence
            && self->internalName().startsWith(u"QQmlListProperty<"_s)) {
        self->setIsListProperty(true);
    }

    const QString baseTypeName = self->baseTypeName();
    const auto baseType = findType(baseTypeName, context, usedTypes);
    if (!self->m_baseType.scope && !baseTypeName.isEmpty())
        self->m_baseType = { baseType.scope, baseType.revision };

    if (!self->m_attachedType && !self->m_attachedTypeName.isEmpty())
        self->m_attachedType = findType(self->m_attachedTypeName, context, usedTypes).scope;

    if (!self->m_valueType && !self->m_valueTypeName.isEmpty())
        self->m_valueType = findType(self->m_valueTypeName, context, usedTypes).scope;

    if (!self->m_extensionType) {
        if (self->m_extensionTypeName.isEmpty()) {
            if (self->accessSemantics() == AccessSemantics::Sequence) {
                // All sequence types are implicitly extended by JS Array.
                self->setExtensionTypeName(u"Array"_s);
                self->setExtensionIsJavaScript(true);
                self->m_extensionType = context.arrayType();
            }
        } else {
            self->m_extensionType = findType(self->m_extensionTypeName, context, usedTypes).scope;
        }
    }


    for (auto it = self->m_properties.begin(), end = self->m_properties.end(); it != end; ++it) {
        const QString typeName = it->typeName();
        if (it->type() || typeName.isEmpty())
            continue;

        if (const auto type = findType(typeName, context, usedTypes); type.scope) {
            it->setType(it->isList() ? type.scope->listType() : type.scope);
            continue;
        }

        const auto enumeration = self->m_enumerations.find(typeName);
        if (enumeration != self->m_enumerations.end()) {
            it->setType(it->isList()
                        ? enumeration->type()->listType()
                        : QQmlJSScope::ConstPtr(enumeration->type()));
        }
    }

    const auto resolveParameter = [&](QQmlJSMetaParameter &parameter) {
        if (const QString typeName = parameter.typeName();
            !parameter.type() && !typeName.isEmpty()) {
            auto type = findType(typeName, context, usedTypes);
            if (type.scope && parameter.isList()) {
                type.scope = type.scope->listType();
                parameter.setIsList(false);
                parameter.setIsPointer(false);
                parameter.setTypeName(type.scope ? type.scope->internalName() : QString());
            } else if (type.scope && type.scope->isReferenceType()) {
                parameter.setIsPointer(true);
            }
            parameter.setType({ type.scope });
        }
    };

    for (auto it = self->m_methods.begin(), end = self->m_methods.end(); it != end; ++it) {
        auto returnValue = it->returnValue();
        resolveParameter(returnValue);
        it->setReturnValue(returnValue);

        auto parameters = it->parameters();
        for (int i = 0, length = parameters.size(); i < length; ++i)
            resolveParameter(parameters[i]);
        it->setParameters(parameters);
    }

    for (auto it = self->m_jsIdentifiers.begin(); it != self->m_jsIdentifiers.end(); ++it) {
        if (it->typeName)
            it->scope = findType(it->typeName.value(), context, usedTypes).scope;
    }

    return baseType.revision;
}

void QQmlJSScope::updateChildScope(
        const QQmlJSScope::Ptr &childScope, const QQmlJSScope::Ptr &self,
        const QQmlJS::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
{
    switch (childScope->scopeType()) {
    case QQmlSA::ScopeType::GroupedPropertyScope:
        QQmlJSUtils::searchBaseAndExtensionTypes(
                self.data(), [&](const QQmlJSScope *type, QQmlJSScope::ExtensionKind mode) {
                    if (mode == QQmlJSScope::ExtensionNamespace)
                        return false;
                    const auto propertyIt = type->m_properties.find(childScope->internalName());
                    if (propertyIt != type->m_properties.end()) {
                        childScope->m_baseType.scope = QQmlJSScope::ConstPtr(propertyIt->type());
                        if (propertyIt->type())
                            childScope->m_semantics = propertyIt->type()->accessSemantics();
                        childScope->setBaseTypeName(propertyIt->typeName());
                        return true;
                    }
                    return false;
                });
        break;
    case QQmlSA::ScopeType::AttachedPropertyScope:
        if (const auto attachedBase = findType(
                    childScope->internalName(), contextualTypes, usedTypes).scope) {
            childScope->m_baseType.scope = attachedBase->attachedType();
            childScope->setBaseTypeName(attachedBase->attachedTypeName());
        }
        break;
    default:
        break;
    }
}

template<typename Resolver, typename ChildScopeUpdater>
static QTypeRevision resolveTypesInternal(
        Resolver resolve, ChildScopeUpdater update, const QQmlJSScope::Ptr &self,
        const QQmlJS::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
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
        const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    const auto resolveAll = [](const QQmlJSScope::Ptr &self,
                               const QQmlJS::ContextualTypes &contextualTypes,
                               QSet<QString> *usedTypes) {
        resolveEnums(self, contextualTypes, usedTypes);
        resolveList(self, contextualTypes.arrayType());
        return resolveType(self, contextualTypes, usedTypes);
    };
    return resolveTypesInternal(resolveAll, updateChildScope, self, contextualTypes, usedTypes);
}

void QQmlJSScope::resolveNonEnumTypes(
        const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    resolveTypesInternal(resolveType, updateChildScope, self, contextualTypes, usedTypes);
}

static QString flagStorage(const QString &underlyingType)
{
    // All numeric types are builtins. Therefore we can exhaustively check the internal names.

    if (underlyingType == u"uint"
        || underlyingType == u"quint8"
        || underlyingType == u"ushort"
        || underlyingType == u"ulonglong") {
        return u"uint"_s;
    }

    if (underlyingType == u"int"
        || underlyingType == u"qint8"
        || underlyingType == u"short"
        || underlyingType == u"longlong") {
        return u"int"_s;
    }

    // Will fail to resolve and produce an error on usage.
    // It's harmless if you never use the enum.
    return QString();
}

/*!
    \internal
    Resolves all enums of self.

    Some enums happen to have an alias, e.g. when an enum is used as a flag, the enum will exist in
   two versions, once as enum (e.g. Qt::MouseButton) and once as a flag (e.g. Qt::MouseButtons). In
   this case, normally only the flag is exposed to the qt metatype system and tools like qmltc will
   have troubles when encountering the enum in signal parameters etc. To solve this problem,
   resolveEnums() will create a QQmlJSMetaEnum copy for the alias in case the 'self'-scope already
   does not have an enum called like the alias.
 */
void QQmlJSScope::resolveEnums(
        const QQmlJSScope::Ptr &self, const QQmlJS::ContextualTypes &contextualTypes,
        QSet<QString> *usedTypes)
{
    // temporary hash to avoid messing up m_enumerations while iterators are active on it
    QHash<QString, QQmlJSMetaEnum> toBeAppended;
    for (auto it = self->m_enumerations.begin(), end = self->m_enumerations.end(); it != end; ++it) {
        if (it->type())
            continue;
        QQmlJSScope::Ptr enumScope = QQmlJSScope::create();
        reparent(self, enumScope);
        enumScope->m_scopeType = QQmlSA::ScopeType::EnumScope;

        QString typeName = it->typeName();
        if (typeName.isEmpty())
            typeName = QStringLiteral("int");
        else if (it->isFlag())
            typeName = flagStorage(typeName);
        enumScope->setBaseTypeName(typeName);
        const auto type = findType(typeName, contextualTypes, usedTypes);
        enumScope->m_baseType = { type.scope, type.revision };

        enumScope->m_semantics = AccessSemantics::Value;
        enumScope->m_internalName = self->internalName() + QStringLiteral("::") + it->name();
        if (QString alias = it->alias(); !alias.isEmpty()
            && self->m_enumerations.constFind(alias) == self->m_enumerations.constEnd()) {
            auto aliasScope = QQmlJSScope::clone(enumScope);
            aliasScope->m_internalName = self->internalName() + QStringLiteral("::") + alias;
            QQmlJSMetaEnum cpy(*it);
            cpy.setType(QQmlJSScope::ConstPtr(aliasScope));
            toBeAppended.insert(alias, cpy);
        }
        it->setType(QQmlJSScope::ConstPtr(enumScope));
    }
    // no more iterators active on m_enumerations, so it can be changed safely now
    self->m_enumerations.insert(toBeAppended);
}

void QQmlJSScope::resolveList(const QQmlJSScope::Ptr &self, const QQmlJSScope::ConstPtr &arrayType)
{
    if (self->listType() || self->accessSemantics() == AccessSemantics::Sequence)
        return;

    Q_ASSERT(!arrayType.isNull());
    QQmlJSScope::Ptr listType = QQmlJSScope::create();
    listType->setAccessSemantics(AccessSemantics::Sequence);
    listType->setValueTypeName(self->internalName());

    if (self->isComposite()) {
        // There is no internalName for this thing. Just set the value type right away
        listType->setInternalName(u"QQmlListProperty<>"_s);
        listType->m_valueType = QQmlJSScope::ConstPtr(self);
    } else if (self->isReferenceType()) {
        listType->setInternalName(u"QQmlListProperty<%2>"_s.arg(self->internalName()));
        // Do not set a filePath on the list type, so that we have to generalize it
        // even in direct mode.
    } else {
        listType->setInternalName(u"QList<%2>"_s.arg(self->internalName()));
        listType->setFilePath(self->filePath());
    }

    const QQmlJSImportedScope element = {self, QTypeRevision()};
    const QQmlJSImportedScope array = {arrayType, QTypeRevision()};
    QQmlJS::ContextualTypes contextualTypes(
            QQmlJS::ContextualTypes::INTERNAL,
            { { self->internalName(), element }, },
            { { self, self->internalName() }, },
            arrayType);
    QQmlJSScope::resolveTypes(listType, contextualTypes);

    Q_ASSERT(listType->valueType() == self);
    self->m_listType = listType;
}

void QQmlJSScope::resolveGroup(
        const Ptr &self, const ConstPtr &baseType,
        const QQmlJS::ContextualTypes &contextualTypes, QSet<QString> *usedTypes)
{
    Q_ASSERT(baseType);
    // Generalized group properties are always composite,
    // which means we expect contextualTypes to be QML names.
    Q_ASSERT(self->isComposite());

    self->m_baseType.scope = baseType;
    self->m_semantics = baseType->accessSemantics();
    resolveNonEnumTypes(self, contextualTypes, usedTypes);
}

QQmlJSScope::ConstPtr QQmlJSScope::findCurrentQMLScope(const QQmlJSScope::ConstPtr &scope)
{
    auto qmlScope = scope;
    while (qmlScope && qmlScope->m_scopeType != QQmlSA::ScopeType::QMLScope)
        qmlScope = qmlScope->parentScope();
    return qmlScope;
}

bool QQmlJSScope::hasProperty(const QString &name) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                return scope->m_properties.contains(name);
            });
}

QQmlJSMetaProperty QQmlJSScope::property(const QString &name) const
{
    QQmlJSMetaProperty prop;
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
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
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                for (auto it = scope->m_properties.constBegin();
                     it != scope->m_properties.constEnd(); it++) {
                    if (!results.contains(it.key()))
                        results.insert(it.key(), it.value());
                }
                return false;
            });
    return results;
}

QQmlJSScope::AnnotatedScope QQmlJSScope::ownerOfProperty(const QQmlJSScope::ConstPtr &self,
                                                         const QString &name)
{
    QQmlJSScope::AnnotatedScope owner;
    QQmlJSUtils::searchBaseAndExtensionTypes(
            self, [&](const QQmlJSScope::ConstPtr &scope, QQmlJSScope::ExtensionKind mode) {
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;
                if (scope->hasOwnProperty(name)) {
                    owner = { scope, mode };
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
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (scope->isPropertyLocallyRequired(name)) {
                    isRequired = true;
                    return true;
                }

                // the hasOwnProperty() below only makes sense if our scope is
                // not an extension namespace
                if (mode == QQmlJSScope::ExtensionNamespace)
                    return false;

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

void QQmlJSScope::addOwnPropertyBinding(const QQmlJSMetaPropertyBinding &binding, BindingTargetSpecifier specifier)
{
    Q_ASSERT(binding.sourceLocation().isValid());
    m_propertyBindings.insert(binding.propertyName(), binding);

    // NB: insert() prepends \a binding to the list of bindings, but we need
    // append, so rotate
    using iter = typename QMultiHash<QString, QQmlJSMetaPropertyBinding>::iterator;
    QPair<iter, iter> r = m_propertyBindings.equal_range(binding.propertyName());
    std::rotate(r.first, std::next(r.first), r.second);

    // additionally store bindings in the QmlIR compatible order
    addOwnPropertyBindingInQmlIROrder(binding, specifier);
    Q_ASSERT(m_propertyBindings.size() == m_propertyBindingsArray.size());
}

void QQmlJSScope::addOwnPropertyBindingInQmlIROrder(const QQmlJSMetaPropertyBinding &binding,
                                                    BindingTargetSpecifier specifier)
{
    // the order:
    // * ordinary bindings are prepended to the binding array
    // * list bindings are properly ordered within each other, so basically
    //   prepended "in bulk"
    // * bindings to default properties (which are not explicitly mentioned in
    //   binding expression) are inserted by source location's offset

    static_assert(QTypeInfo<QQmlJSScope::QmlIRCompatibilityBindingData>::isRelocatable,
                  "We really want T to be relocatable as it improves QList<T> performance");

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
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode != QQmlJSScope::NotExtension) {
                    Q_ASSERT(!scope->hasOwnPropertyBindings(name));
                    return false;
                }
                return scope->hasOwnPropertyBindings(name);
            });
}

QList<QQmlJSMetaPropertyBinding> QQmlJSScope::propertyBindings(const QString &name) const
{
    QList<QQmlJSMetaPropertyBinding> bindings;
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode != QQmlJSScope::NotExtension) {
                    Q_ASSERT(!scope->hasOwnPropertyBindings(name));
                    return false;
                }
                const auto range = scope->ownPropertyBindings(name);
                for (auto it = range.first; it != range.second; ++it)
                    bindings.append(*it);
                return false;
            });
    return bindings;
}

bool QQmlJSScope::hasInterface(const QString &name) const
{
    return QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode != QQmlJSScope::NotExtension)
                    return false;
                return scope->m_interfaceNames.contains(name);
            });
}

bool QQmlJSScope::isNameDeferred(const QString &name) const
{
    bool isDeferred = false;

    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
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

void QQmlJSScope::setBaseTypeName(const QString &baseTypeName)
{
    m_flags.setFlag(HasBaseTypeError, false);
    m_baseTypeNameOrError = baseTypeName;
}

QString QQmlJSScope::baseTypeName() const
{
    return m_flags.testFlag(HasBaseTypeError) ? QString() : m_baseTypeNameOrError;
}

void QQmlJSScope::setBaseTypeError(const QString &baseTypeError)
{
    m_flags.setFlag(HasBaseTypeError);
    m_baseTypeNameOrError = baseTypeError;
}

/*!
\internal
The name of the module is only saved in the QmlComponent. Iterate through the parent scopes until
the QmlComponent or the root is reached to find out the module name of the component in which `this`
resides.
*/
QString QQmlJSScope::moduleName() const
{
    for (const QQmlJSScope *it = this; it; it = it->parentScope().get()) {
        const QString name = it->ownModuleName();
        if (!name.isEmpty())
            return name;
    }
    return {};
}

QString QQmlJSScope::baseTypeError() const
{
    return m_flags.testFlag(HasBaseTypeError) ? m_baseTypeNameOrError : QString();
}

QString QQmlJSScope::attachedTypeName() const
{
    QString name;
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode != QQmlJSScope::NotExtension)
                    return false;
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
    QQmlJSUtils::searchBaseAndExtensionTypes(
            this, [&](const QQmlJSScope *scope, QQmlJSScope::ExtensionKind mode) {
                if (mode != QQmlJSScope::NotExtension)
                    return false;
                if (scope->ownAttachedType().isNull())
                    return false;
                ptr = scope->ownAttachedType();
                return true;
            });

    return ptr;
}

QQmlJSScope::AnnotatedScope QQmlJSScope::extensionType() const
{
    if (!m_extensionType)
        return { m_extensionType, NotExtension };
    if (m_flags & ExtensionIsJavaScript)
        return { m_extensionType, ExtensionJavaScript };
    if (m_flags & ExtensionIsNamespace)
        return { m_extensionType, ExtensionNamespace };
    return { m_extensionType, ExtensionType };
}

void QQmlJSScope::addOwnRuntimeFunctionIndex(QQmlJSMetaMethod::AbsoluteFunctionIndex index)
{
    m_runtimeFunctionIndices.emplaceBack(index);
}

bool QQmlJSScope::isResolved() const
{
    const bool nameIsEmpty = (m_scopeType == ScopeType::AttachedPropertyScope
                              || m_scopeType == ScopeType::GroupedPropertyScope)
            ? m_internalName.isEmpty()
            : m_baseTypeNameOrError.isEmpty();
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
    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        name = scope->ownDefaultPropertyName();
        return !name.isEmpty();
    });
    return name;
}

QString QQmlJSScope::parentPropertyName() const
{
    QString name;
    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        name = scope->ownParentPropertyName();
        return !name.isEmpty();
    });
    return name;
}

bool QQmlJSScope::isFullyResolved() const
{
    bool baseResolved = true;
    QQmlJSUtils::searchBaseAndExtensionTypes(this, [&](const QQmlJSScope *scope) {
        if (!scope->isResolved()) {
            baseResolved = false;
            return true;
        }
        return false;
    });

    return baseResolved;
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

QDeferredFactory<QQmlJSScope>::QDeferredFactory(QQmlJSImporter *importer, const QString &filePath,
                                                const TypeReader &typeReader)
    : m_filePath(filePath),
      m_importer(importer),
      m_typeReader(typeReader ? typeReader
                              : [](QQmlJSImporter *importer, const QString &filePath,
                                   const QSharedPointer<QQmlJSScope> &scopeToPopulate) {
                                    QQmlJSTypeReader defaultTypeReader(importer, filePath);
                                    defaultTypeReader(scopeToPopulate);
                                    return defaultTypeReader.errors();
                                })
{
}

void QDeferredFactory<QQmlJSScope>::populate(const QSharedPointer<QQmlJSScope> &scope) const
{
    scope->setOwnModuleName(m_moduleName);
    scope->setFilePath(m_filePath);

    QList<QQmlJS::DiagnosticMessage> errors = m_typeReader(m_importer, m_filePath, scope);
    m_importer->m_globalWarnings.append(errors);

    scope->setInternalName(internalName());
    QQmlJSScope::resolveEnums(
            scope, m_importer->builtinInternalNames().contextualTypes());
    QQmlJSScope::resolveList(
            scope, m_importer->builtinInternalNames().contextualTypes().arrayType());

    if (m_isSingleton && !scope->isSingleton()) {
        m_importer->m_globalWarnings.append(
                { QStringLiteral(
                          "Type %1 declared as singleton in qmldir but missing pragma Singleton")
                          .arg(scope->internalName()),
                  QtCriticalMsg, QQmlJS::SourceLocation() });
        scope->setIsSingleton(true);
    } else if (!m_isSingleton && scope->isSingleton()) {
        m_importer->m_globalWarnings.append(
                { QStringLiteral("Type %1 not declared as singleton in qmldir "
                                 "but using pragma Singleton")
                          .arg(scope->internalName()),
                  QtCriticalMsg, QQmlJS::SourceLocation() });
        scope->setIsSingleton(false);
    }
}

/*!
  \internal
  Checks whether \a derived type can be assigned to this type. Returns \c
  true if the type hierarchy of \a derived contains a type equal to this.

   \note Assigning \a derived to "QVariant" or "QJSValue" is always possible and
   the function returns \c true in this case. In addition any "QObject" based \a derived type
   can be assigned to a this type if that type is derived from "QQmlComponent".
 */
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
        if (isBaseComponent && scope->internalName() == u"QObject"_s)
            return true;
    }

    if (internalName() == u"QVariant"_s || internalName() == u"QJSValue"_s)
        return true;

    return isListProperty() && valueType()->canAssign(derived);
}

/*!
  \internal
  Checks whether this type or its parents have a custom parser.
*/
bool QQmlJSScope::isInCustomParserParent() const
{
    for (const auto *scope = this; scope; scope = scope->parentScope().get()) {
        if (!scope->baseType().isNull() && scope->baseType()->hasCustomParser())
            return true;
    }

    return false;
}

/*!
 * \internal
 * if this->isInlineComponent(), then this getter returns the name of the inline
 * component.
 */
std::optional<QString> QQmlJSScope::inlineComponentName() const
{
    Q_ASSERT(isInlineComponent() == m_inlineComponentName.has_value());
    return m_inlineComponentName;
}

/*!
 * \internal
 * If this type is part of an inline component, return its name. Otherwise, if this type
 * is part of the document root, return the document root name.
 */
QQmlJSScope::InlineComponentOrDocumentRootName QQmlJSScope::enclosingInlineComponentName() const
{
    for (auto *type = this; type; type = type->parentScope().get()) {
        if (type->isInlineComponent())
            return *type->inlineComponentName();
    }
    return RootDocumentNameType();
}

QVector<QQmlJSScope::ConstPtr> QQmlJSScope::childScopes() const
{
    QVector<QQmlJSScope::ConstPtr> result;
    result.reserve(m_childScopes.size());
    for (const auto &child : m_childScopes)
        result.append(child);
    return result;
}

/*!
    \internal

    Returns true if this type or any base type of it has the "EnforcesScopedEnums" flag.
    The rationale is that you can turn on enforcement of scoped enums, but you cannot turn
    it off explicitly.
 */
bool QQmlJSScope::enforcesScopedEnums() const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().get()) {
        if (scope->hasEnforcesScopedEnumsFlag())
            return true;
    }
    return false;
}

/*!
   \internal
   Returns true if the current type is creatable by checking all the required base classes.
   "Uncreatability" is only inherited from base types for composite types (in qml) and not for non-composite types (c++).

For the exact definition:
A type is uncreatable if and only if one of its composite base type or its first non-composite base type matches
   following criteria:
   \list
   \li the base type is a singleton, or
   \li the base type is an attached type, or
   \li the base type is a C++ type with the QML_UNCREATABLE or QML_ANONYMOUS macro, or
   \li the base type is a type without default constructor (in that case, it really needs QML_UNCREATABLE or QML_ANONYMOUS)
   \endlist
 */
bool QQmlJSScope::isCreatable() const
{
    auto isCreatableNonRecursive = [](const QQmlJSScope *scope) {
        return scope->hasCreatableFlag() && !scope->isSingleton()
                && scope->scopeType() == QQmlSA::ScopeType::QMLScope;
    };

    for (const QQmlJSScope* scope = this; scope; scope = scope->baseType().get()) {
        if (!scope->isComposite()) {
            // just check the first nonComposite (c++) base for isCreatableNonRecursive() and then stop
            return isCreatableNonRecursive(scope);
        } else {
            // check all composite (qml) bases for isCreatableNonRecursive().
            if (isCreatableNonRecursive(scope))
                return true;
        }
    }
    // no uncreatable bases found
    return false;
}

bool QQmlJSScope::isStructured() const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().get()) {
        if (!scope->isComposite())
            return scope->hasStructuredFlag();
    }
    return false;
}

QQmlSA::Element QQmlJSScope::createQQmlSAElement(const ConstPtr &ptr)
{
    QQmlSA::Element element;
    *reinterpret_cast<QQmlJSScope::ConstPtr *>(element.m_data) = ptr;
    return element;
}

QQmlSA::Element QQmlJSScope::createQQmlSAElement(ConstPtr &&ptr)
{
    QQmlSA::Element element;
    *reinterpret_cast<QQmlJSScope::ConstPtr *>(element.m_data) = std::move(ptr);
    return element;
}

const QQmlJSScope::ConstPtr &QQmlJSScope::scope(const QQmlSA::Element &element)
{
    return *reinterpret_cast<const QQmlJSScope::ConstPtr *>(element.m_data);
}

QTypeRevision
QQmlJSScope::nonCompositeBaseRevision(const ImportedScope<QQmlJSScope::ConstPtr> &scope)
{
    for (auto base = scope; base.scope;
         base = { base.scope->m_baseType.scope, base.scope->m_baseType.revision }) {
        if (!base.scope->isComposite())
            return base.revision;
    }
    return {};
}

/*!
  \internal
  Checks whether \a otherScope is the same type as this.

   In addition to checking whether the scopes are identical, we also cover duplicate scopes with
   the same internal name.
 */
bool QQmlJSScope::isSameType(const ConstPtr &otherScope) const
{
    return this == otherScope.get()
            || (!this->internalName().isEmpty()
                && this->internalName() == otherScope->internalName());
}

bool QQmlJSScope::inherits(const ConstPtr &base) const
{
    for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().get()) {
        if (scope->isSameType(base))
            return true;
    }
    return false;
}


QT_END_NAMESPACE
