// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCOMPONENTANDALIASRESOLVER_P_H
#define QQMLCOMPONENTANDALIASRESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlerror.h>

#include <QtCore/qglobal.h>
#include <QtCore/qhash.h>

#include <private/qqmltypeloader_p.h>
#include <private/qqmlpropertyresolver_p.h>
#include <private/qqmlpropertycachecreator_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQmlTypeCompiler);

// This class primarily resolves component boundaries in a document.
// With the information about boundaries, it then goes on to resolve aliases and generalized
// grouped properties. Both rely on IDs as first part of their expressions and the IDs have
// to be located in surrounding components. That's why we have to do this with the component
// boundaries in mind.

class QQmlComponentAndAliasResolverBase
{
    Q_DECLARE_TR_FUNCTIONS(QQmlComponentAndAliasResolverBase)
};

template<typename ObjectContainer>
class QQmlComponentAndAliasResolver : public QQmlComponentAndAliasResolverBase
{
public:
    using CompiledObject = typename ObjectContainer::CompiledObject;
    using CompiledBinding = typename ObjectContainer::CompiledBinding;

    QQmlComponentAndAliasResolver(
            ObjectContainer *compiler,
            QQmlPropertyCacheVector *propertyCaches);

    [[nodiscard]] QQmlError resolve(int root = 0);

private:
    enum AliasResolutionResult {
        NoAliasResolved,
        SomeAliasesResolved,
        AllAliasesResolved
    };

    enum AliasTargetResult {
        TargetResolved,
        TargetInvalid,
        TargetInOtherObject,
        TargetIsResolvedAlias,
        TargetIsPendingAlias
    };

    static bool findSubProperty(
            const QQmlPropertyCache::ConstPtr &cache, QStringView subProperty,
            QQmlPropertyIndex *propIdx);
    [[nodiscard]] bool resolveDeepAlias(
            const CompiledObject *targetObject, QStringView property, QStringView subProperty,
            QMetaType targetPropertyType, QQmlPropertyIndex *propIdx) const;
    [[nodiscard]] QQmlError resolveSubProperty(
            const QV4::CompiledData::Alias *alias, const CompiledObject *targetObject,
            QStringView property, QStringView subProperty, QMetaType targetPropertyType,
            QQmlPropertyIndex *propIdx) const;
    [[nodiscard]] AliasTargetResult resolveAliasTarget(
            const QV4::CompiledData::Alias *alias, int objectIndex, int targetObjectIndex,
            QQmlPropertyIndex *propIdx, QQmlError *error) const;
    [[nodiscard]] AliasResolutionResult resolveAliasesInObject(
            int objectIndex, QQmlPropertyCacheAliasCreator<ObjectContainer> *aliasCacheCreator,
            QQmlError *error);

    // To be specialized for each container
    void allocateNamedObjects(int componentRoot) const;
    void setObjectId(int index) const;
    [[nodiscard]] bool markAsComponent(int index) const;
    [[nodiscard]] bool isComponentBoundary(int index) const;
    [[nodiscard]] bool appendAliasToPropertyCache(
            const QV4::CompiledData::Alias *alias, int objectIndex, int aliasIndex,
            int encodedPropertyIndex, int resolvedTargetObjectId, int targetObjectIndex,
            QQmlPropertyCacheAliasCreator<ObjectContainer> *aliasCacheCreator, QQmlError *error)
    {
        *error = aliasCacheCreator->appendAliasToPropertyCache(
                *alias, objectIndex, aliasIndex, encodedPropertyIndex, resolvedTargetObjectId,
                targetObjectIndex);
        resolvedAliases.insert(alias);
        return !error->isValid();
    }


    void resolveGeneralizedGroupedProperty(CompiledBinding *binding);
    [[nodiscard]] bool wrapImplicitComponent(CompiledBinding *binding)
    {
        const int childObjectIndex = binding->value.objectIndex;

        // Create an implicit component wrapper as an additional property cache entry.
        // The CU binary is read-only so we cannot patch the binding.
        QQmlPropertyCache::ConstPtr baseComponentCache
                = QQmlMetaType::propertyCache(&QQmlComponent::staticMetaObject);
        QQmlPropertyCache::Ptr wrapperCache = baseComponentCache->copyAndReserve(1, 0, 0, 0);
        wrapperCache->appendComponentWrapper(
                wrapperCache->propertyCount(), childObjectIndex);

        const uint wrapperIndex = m_propertyCaches->count();
        m_propertyCaches->append(std::move(wrapperCache));

        m_componentRoots.append(wrapperIndex);
        return true;
    }

    [[nodiscard]] QQmlError findAndRegisterImplicitComponents(
            const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &propertyCache);
    [[nodiscard]] QQmlError collectIdsAndAliases(int objectIndex);
    [[nodiscard]] QQmlError resolveAliases();
    void resolveGeneralizedGroupedProperties();
    [[nodiscard]] QQmlError resolveComponentsInInlineComponentRoot(int root);

    QString stringAt(int idx) const { return m_compiler->stringAt(idx); }
    QV4::ResolvedTypeReference *resolvedType(int id) const { return m_compiler->resolvedType(id); }

    [[nodiscard]] QQmlError error(
            const QV4::CompiledData::Location &location,
            const QString &description)
    {
        QQmlError error;
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(location.column()));
        error.setDescription(description);
        error.setUrl(m_compiler->url());
        return error;
    }

    template<typename Token>
    [[nodiscard]] QQmlError error(Token token, const QString &description)
    {
        return error(token->location, description);
    }

    static bool isUsableComponent(const QMetaObject *metaObject)
    {
        // The metaObject is a component we're interested in if it either is a QQmlComponent itself
        // or if any of its parents is a QQmlAbstractDelegateComponent. We don't want to include
        // qqmldelegatecomponent_p.h because it belongs to QtQmlModels.

        if (metaObject == &QQmlComponent::staticMetaObject)
            return true;

        for (; metaObject; metaObject = metaObject->superClass()) {
            if (qstrcmp(metaObject->className(), "QQmlAbstractDelegateComponent") == 0)
                return true;
        }

        return false;
    }

    ObjectContainer *m_compiler = nullptr;

    // Implicit component insertion may have added objects and thus we also need
    // to extend the symmetric propertyCaches. Therefore, non-const propertyCaches.
    QQmlPropertyCacheVector *m_propertyCaches = nullptr;

    // indices of the objects that are actually Component {}
    QList<quint32> m_componentRoots;
    QList<int> m_objectsWithAliases;
    QList<CompiledBinding *> m_generalizedGroupedProperties;
    QSet<const QV4::CompiledData::Alias *> resolvedAliases;
    typename ObjectContainer::IdToObjectMap m_idToObjectIndex;
};

template<typename ObjectContainer>
QQmlComponentAndAliasResolver<ObjectContainer>::QQmlComponentAndAliasResolver(
        ObjectContainer *compiler,
        QQmlPropertyCacheVector *propertyCaches)
    : m_compiler(compiler)
    , m_propertyCaches(propertyCaches)
{
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::findAndRegisterImplicitComponents(
        const CompiledObject *obj, const QQmlPropertyCache::ConstPtr &propertyCache)
{
    QQmlPropertyResolver propertyResolver(propertyCache);

    const QQmlPropertyData *defaultProperty = obj->indexOfDefaultPropertyOrAlias != -1
            ? propertyCache->parent()->defaultProperty()
            : propertyCache->defaultProperty();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd(); binding != end; ++binding) {
        if (binding->type() != QV4::CompiledData::Binding::Type_Object)
            continue;

        auto targetObject = m_compiler->objectAt(binding->value.objectIndex);
        auto typeReference = resolvedType(targetObject->inheritedTypeNameIndex);
        Q_ASSERT(typeReference);

        const QMetaObject *firstMetaObject = nullptr;
        if (const auto propCache = typeReference->typePropertyCache())
            firstMetaObject = propCache->firstCppMetaObject();
        if (isUsableComponent(firstMetaObject))
            continue;

        // if here, not a QQmlComponent, so needs wrapping
        const QQmlPropertyData *pd = nullptr;
        if (binding->propertyNameIndex != quint32(0)) {
            bool notInRevision = false;
            pd = propertyResolver.property(stringAt(binding->propertyNameIndex), &notInRevision);
        } else {
            pd = defaultProperty;
        }
        if (!pd || !pd->isQObject())
            continue;

        // If the version is given, use it and look up by QQmlType.
        // Otherwise, make sure we look up by metaobject.
        // TODO: Is this correct?
        QQmlPropertyCache::ConstPtr pc = pd->typeVersion().hasMinorVersion()
                ? QQmlMetaType::rawPropertyCacheForType(pd->propType(), pd->typeVersion())
                : QQmlMetaType::rawPropertyCacheForType(pd->propType());
        const QMetaObject *mo = pc ? pc->firstCppMetaObject() : nullptr;
        while (mo) {
            if (mo == &QQmlComponent::staticMetaObject)
                break;
            mo = mo->superClass();
        }

        if (!mo)
            continue;

        if (!wrapImplicitComponent(binding))
            return error(binding, QQmlComponentAndAliasResolverBase::tr("Cannot wrap implicit component"));
    }

    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolveComponentsInInlineComponentRoot(
        int root)
{
    // Find implicit components in the inline component itself. Also warn about inline
    // components being explicit components.

    const auto rootObj = m_compiler->objectAt(root);
    Q_ASSERT(rootObj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot));

    if (const int typeName = rootObj->inheritedTypeNameIndex) {
        const auto *tref = resolvedType(typeName);
        Q_ASSERT(tref);
        if (tref->type().metaObject() == &QQmlComponent::staticMetaObject) {
            qCWarning(lcQmlTypeCompiler).nospace().noquote()
                    << m_compiler->url().toString() << ":" << rootObj->location.line() << ":"
                    << rootObj->location.column()
                    << ": Using a Component as the root of an inline component is deprecated: "
                       "inline components are "
                       "automatically wrapped into Components when needed.";
            return QQmlError();
        }
    }

    const QQmlPropertyCache::ConstPtr rootCache = m_propertyCaches->at(root);
    Q_ASSERT(rootCache);

    return findAndRegisterImplicitComponents(rootObj, rootCache);
}

// Resolve ignores everything relating to inline components, except for implicit components.
template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolve(int root)
{
    // Detect real Component {} objects as well as implicitly defined components, such as
    //     someItemDelegate: Item {}
    // In the implicit case Item is surrounded by a synthetic Component {} because the property
    // on the left hand side is of QQmlComponent type.
    const int objCountWithoutSynthesizedComponents = m_compiler->objectCount();

    if (root != 0) {
        const QQmlError error = resolveComponentsInInlineComponentRoot(root);
        if (error.isValid())
            return error;
    }

    // root+1, as ic root is handled at the end
    const int startObjectIndex = root == 0 ? root : root+1;

    for (int i = startObjectIndex; i < objCountWithoutSynthesizedComponents; ++i) {
        auto obj = m_compiler->objectAt(i);
        const bool isInlineComponentRoot
                = obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot);
        const bool isPartOfInlineComponent
                = obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent);
        QQmlPropertyCache::ConstPtr cache = m_propertyCaches->at(i);

        if (root == 0) {
            // normal component root, skip over anything inline component related
            if (isInlineComponentRoot || isPartOfInlineComponent)
                continue;
        } else if (!isPartOfInlineComponent || isInlineComponentRoot) {
            // When handling an inline component, stop where the inline component ends
            // Note: We do not support nested inline components. Therefore, isInlineComponentRoot
            //       tells us that the element after the current inline component is again an
            //       inline component
            break;
        }

        if (obj->inheritedTypeNameIndex == 0 && !cache)
            continue;

        bool isExplicitComponent = false;
        if (obj->inheritedTypeNameIndex) {
            auto *tref = resolvedType(obj->inheritedTypeNameIndex);
            Q_ASSERT(tref);
            if (tref->type().metaObject() == &QQmlComponent::staticMetaObject)
                isExplicitComponent = true;
        }

        if (!isExplicitComponent) {
            if (cache) {
                const QQmlError error = findAndRegisterImplicitComponents(obj, cache);
                if (error.isValid())
                    return error;
            }
            continue;
        }

        if (!markAsComponent(i))
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Cannot mark object as component"));

        // check if this object is the root
        if (i == 0) {
            if (isExplicitComponent)
                qCWarning(lcQmlTypeCompiler).nospace().noquote()
                        << m_compiler->url().toString() << ":" << obj->location.line() << ":"
                        << obj->location.column()
                        << ": Using a Component as the root of a QML document is deprecated: types "
                           "defined in qml documents are "
                           "automatically wrapped into Components when needed.";
        }

        if (obj->functionCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new functions."));
        if (obj->propertyCount() > 0 || obj->aliasCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new properties."));
        if (obj->signalCount() > 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Component objects cannot declare new signals."));

        if (obj->bindingCount() == 0)
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Cannot create empty component specification"));

        const auto rootBinding = obj->bindingsBegin();
        const auto bindingsEnd = obj->bindingsEnd();

        // Produce the more specific "no properties" error rather than the "invalid body" error
        // where possible.
        for (auto b = rootBinding; b != bindingsEnd; ++b) {
            if (b->propertyNameIndex == 0)
                continue;

            return error(b, QQmlComponentAndAliasResolverBase::tr("Component elements may not contain properties other than id"));
        }

        if (auto b = rootBinding;
                b->type() != QV4::CompiledData::Binding::Type_Object || ++b != bindingsEnd) {
            return error(obj, QQmlComponentAndAliasResolverBase::tr("Invalid component body specification"));
        }

        // For the root object, we are going to collect ids/aliases and resolve them for as a
        // separate last pass.
        if (i != 0)
            m_componentRoots.append(i);
    }

    for (int i = 0; i < m_componentRoots.size(); ++i) {
        const int componentRoot = m_componentRoots.at(i);
        const int childObjectIndex = componentRoot < m_compiler->objectCount()
                ? m_compiler->objectAt(componentRoot)->bindingsBegin()->value.objectIndex
                : m_compiler->resolvedIndex(componentRoot);

        m_idToObjectIndex.clear();
        m_objectsWithAliases.clear();
        m_generalizedGroupedProperties.clear();

        if (const QQmlError error = collectIdsAndAliases(childObjectIndex);
                error.isValid()) {
            return error;
        }

        allocateNamedObjects(componentRoot);

        if (const QQmlError error = resolveAliases(); error.isValid())
            return error;

        resolveGeneralizedGroupedProperties();
    }

    // Collect ids and aliases for root
    m_idToObjectIndex.clear();
    m_objectsWithAliases.clear();
    m_generalizedGroupedProperties.clear();

    if (const QQmlError error = collectIdsAndAliases(root); error.isValid())
        return error;

    allocateNamedObjects(root);
    if (const QQmlError error = resolveAliases(); error.isValid())
        return error;

    resolveGeneralizedGroupedProperties();
    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::collectIdsAndAliases(int objectIndex)
{
    auto obj = m_compiler->objectAt(objectIndex);

    if (obj->idNameIndex != 0) {
        if (m_idToObjectIndex.contains(obj->idNameIndex))
            return error(obj->locationOfIdProperty, QQmlComponentAndAliasResolverBase::tr("id is not unique"));
        setObjectId(objectIndex);
        m_idToObjectIndex.insert(obj->idNameIndex, objectIndex);
    }

    if (obj->aliasCount() > 0)
        m_objectsWithAliases.append(objectIndex);

    // Stop at Component boundary
    if (isComponentBoundary(objectIndex) && objectIndex != /*root object*/0)
        return QQmlError();

    for (auto binding = obj->bindingsBegin(), end = obj->bindingsEnd();
         binding != end; ++binding) {
        switch (binding->type()) {
        case QV4::CompiledData::Binding::Type_GroupedProperty: {
            const auto *inner = m_compiler->objectAt(binding->value.objectIndex);
            if (m_compiler->stringAt(inner->inheritedTypeNameIndex).isEmpty()) {
                const auto cache = m_propertyCaches->at(objectIndex);
                if (!cache || !cache->property(
                            m_compiler->stringAt(binding->propertyNameIndex), nullptr, nullptr)) {
                    m_generalizedGroupedProperties.append(binding);
                }
            }
        }
        Q_FALLTHROUGH();
        case QV4::CompiledData::Binding::Type_Object:
        case QV4::CompiledData::Binding::Type_AttachedProperty:
            // Implicit component wrappers are component boundaries.
            // Their children are collected separately via the componentRoots loop.
            if (m_compiler->implicitComponentForObject(binding->value.objectIndex) != -1)
                break;
            if (const QQmlError error = collectIdsAndAliases(binding->value.objectIndex);
                    error.isValid()) {
                return error;
            }
            break;
        default:
            break;
        }
    }

    return QQmlError();
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolveAliases()
{
    if (m_objectsWithAliases.isEmpty())
        return QQmlError();

    QQmlPropertyCacheAliasCreator<ObjectContainer> aliasCacheCreator(m_propertyCaches, m_compiler);

    bool atLeastOneAliasResolved;
    do {
        atLeastOneAliasResolved = false;
        QList<int> pendingObjects;

        for (int objectIndex: std::as_const(m_objectsWithAliases)) {

            QQmlError error;
            const auto result = resolveAliasesInObject(objectIndex, &aliasCacheCreator, &error);
            if (error.isValid())
                return error;

            if (result == AllAliasesResolved) {
                atLeastOneAliasResolved = true;
            } else if (result == SomeAliasesResolved) {
                atLeastOneAliasResolved = true;
                pendingObjects.append(objectIndex);
            } else {
                pendingObjects.append(objectIndex);
            }
        }
        qSwap(m_objectsWithAliases, pendingObjects);
    } while (!m_objectsWithAliases.isEmpty() && atLeastOneAliasResolved);

    if (!atLeastOneAliasResolved && !m_objectsWithAliases.isEmpty()) {
        const CompiledObject *obj = m_compiler->objectAt(m_objectsWithAliases.first());
        for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end; ++alias) {
            if (!resolvedAliases.contains(alias)) {
                return error(
                        alias->location(), QQmlComponentAndAliasResolverBase::tr("Cyclic alias"));
            }
        }
    }

    return QQmlError();
}

template<typename ObjectContainer>
bool QQmlComponentAndAliasResolver<ObjectContainer>::findSubProperty(
        const QQmlPropertyCache::ConstPtr &cache, QStringView subProperty,
        QQmlPropertyIndex *propIdx)
{
    if (!cache)
        return false;

    const QQmlPropertyResolver resolver(cache);
    const QQmlPropertyData *pd = resolver.property(subProperty.toString());
    if (!pd)
        return false;

    *propIdx = QQmlPropertyIndex(propIdx->coreIndex(), pd->coreIndex());
    return true;
}

// Attempts to resolve a "deep alias" - an alias whose sub-property path
// goes through a QObject property, an inline component binding, or another
// alias. For example: \c{alias foo: target.groupedProp.innerProp}
//
// First searches the target object's bindings and aliases for \a property,
// then looks up \a subProperty on the bound/aliased object's property cache.
// If no binding or alias matches, falls back to looking up \a subProperty
// on the declared type's property cache.
//
// On success, updates \a propIdx with the resolved sub-property index and
// returns \c true.
template<typename ObjectContainer>
bool QQmlComponentAndAliasResolver<ObjectContainer>::resolveDeepAlias(
        const CompiledObject *targetObject, QStringView property, QStringView subProperty,
        QMetaType targetPropertyType, QQmlPropertyIndex *propIdx) const
{
    for (auto it = targetObject->bindingsBegin(), end = targetObject->bindingsEnd();
         it != end; ++it) {
        if (stringAt(it->propertyNameIndex) != property)
            continue;
        if (findSubProperty(m_propertyCaches->at(it->value.objectIndex), subProperty, propIdx))
            return true;
    }

    for (auto it = targetObject->aliasesBegin(), end = targetObject->aliasesEnd();
         it != end; ++it) {
        if (stringAt(it->nameIndex()) != property)
            continue;
        const int innerObjectIndex = m_idToObjectIndex.value(it->idIndex(), -1);
        if (innerObjectIndex == -1)
            continue;
        if (findSubProperty(m_propertyCaches->at(innerObjectIndex), subProperty, propIdx))
            return true;
    }

    return findSubProperty(
            QQmlMetaType::propertyCacheForType(targetPropertyType), subProperty, propIdx);
}

template<typename ObjectContainer>
QQmlError QQmlComponentAndAliasResolver<ObjectContainer>::resolveSubProperty(
        const QV4::CompiledData::Alias *alias, const CompiledObject *targetObject,
        QStringView property, QStringView subProperty, QMetaType targetPropertyType,
        QQmlPropertyIndex *propIdx) const
{
    const auto invalidTarget = [&]() {
        return qQmlCompileError(
                alias->referenceLocation(),
                QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1")
                        .arg(subProperty.toString()));
    };

    const QMetaObject *valueTypeMetaObject
            = QQmlMetaType::metaObjectForValueType(targetPropertyType);
    if (!valueTypeMetaObject) {
        // Could be a deep alias.
        if (subProperty.at(0).isLower()
                && resolveDeepAlias(targetObject, property, subProperty, targetPropertyType,
                                    propIdx)) {
            return QQmlError();
        }
        return invalidTarget();
    }

    const int valueTypeIndex = valueTypeMetaObject->indexOfProperty(
            subProperty.toString().toUtf8().constData());
    if (valueTypeIndex == -1)
        return invalidTarget();

    Q_ASSERT(valueTypeIndex <= 0x0000FFFF);
    *propIdx = QQmlPropertyIndex(propIdx->coreIndex(), valueTypeIndex);
    return QQmlError();
}

template<typename ObjectContainer>
typename QQmlComponentAndAliasResolver<ObjectContainer>::AliasTargetResult
QQmlComponentAndAliasResolver<ObjectContainer>::resolveAliasTarget(
        const QV4::CompiledData::Alias *alias, int objectIndex, int targetObjectIndex,
        QQmlPropertyIndex *propIdx, QQmlError *error) const
{
    const QString aliasPropertyValue = stringAt(alias->propertyNameIndex());
    const QStringView aliasProperty(aliasPropertyValue);

    QStringView property = aliasProperty;
    QStringView subProperty;
    if (const int propertySeparator = aliasProperty.indexOf(QLatin1Char('.'));
            propertySeparator != -1) {
        property = aliasProperty.left(propertySeparator);
        subProperty = aliasProperty.mid(propertySeparator + 1);
    }

    // Alias points to object. No need to resolve properties.
    if (property.isEmpty())
        return TargetResolved;

    const auto invalidTarget = [&]() {
        *error = qQmlCompileError(
                alias->referenceLocation(),
                QQmlComponentAndAliasResolverBase::tr("Invalid alias target location: %1")
                        .arg(property.toString()));
        return TargetInvalid;
    };

    const QQmlPropertyCache::ConstPtr targetCache = m_propertyCaches->at(targetObjectIndex);
    if (!targetCache)
        return invalidTarget();

    const QQmlPropertyResolver resolver(targetCache);
    const QQmlPropertyData *targetProperty = resolver.property(
            property.toString(), nullptr, QQmlPropertyResolver::IgnoreRevision);

    const CompiledObject *targetObject = m_compiler->objectAt(targetObjectIndex);

    // The target may be an alias on the target object that has not been resolved yet.
    // Resolve that one first and try this one again later.
    if (!targetProperty) {
        const auto targetAlias = std::find_if(
                targetObject->aliasesBegin(), targetObject->aliasesEnd(),
                [&](const auto &t) { return stringAt(t.nameIndex()) == property; });
        if (targetAlias != targetObject->aliasesEnd()) {
            // Don't continue yet; respect the order of objects.
            if (targetObjectIndex != objectIndex)
                return TargetInOtherObject;
            return resolvedAliases.contains(targetAlias) ? TargetIsResolvedAlias
                                                         : TargetIsPendingAlias;
        }
    }

    if (!targetProperty || targetProperty->coreIndex() > 0x0000FFFF)
        return invalidTarget();

    *propIdx = QQmlPropertyIndex(targetProperty->coreIndex());

    if (!subProperty.isEmpty()) {
        *error = resolveSubProperty(
                alias, targetObject, property, subProperty, targetProperty->propType(), propIdx);
        if (error->isValid())
            return TargetInvalid;
    }

    return TargetResolved;
}

template<typename ObjectContainer>
typename QQmlComponentAndAliasResolver<ObjectContainer>::AliasResolutionResult
QQmlComponentAndAliasResolver<ObjectContainer>::resolveAliasesInObject(
        int objectIndex, QQmlPropertyCacheAliasCreator<ObjectContainer> *aliasCacheCreator,
        QQmlError *error)
{
    const CompiledObject *obj = m_compiler->objectAt(objectIndex);
    if (!obj->aliasCount())
        return AllAliasesResolved;

    int aliasIndex = 0;
    int numSkippedAliases = 0;
    bool hasUnresolvedLocalAliases = false;

    for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end;
         ++alias, ++aliasIndex) {
        if (resolvedAliases.contains(alias)) {
            ++numSkippedAliases;
            continue;
        }

        const int targetObjectIndex = m_idToObjectIndex.value(alias->idIndex(), -1);
        if (targetObjectIndex == -1) {
            *error = qQmlCompileError(
                    alias->referenceLocation(),
                    QQmlComponentAndAliasResolverBase::tr(
                            "Invalid alias reference. Unable to find id \"%1\"")
                            .arg(stringAt(alias->idIndex())));
            break;
        }

        QQmlPropertyIndex propIdx;
        const AliasTargetResult target
                = resolveAliasTarget(alias, objectIndex, targetObjectIndex, &propIdx, error);
        if (target == TargetInvalid)
            break;

        if (target == TargetInOtherObject)
            return aliasIndex == numSkippedAliases ? NoAliasResolved : SomeAliasesResolved;

        if (target == TargetIsPendingAlias) {
            // Target is in the same object but not resolved yet. Try again later.
            ++numSkippedAliases;
            hasUnresolvedLocalAliases = true;
            continue;
        }

        // If the target is an already resolved alias, we can set this alias right away.
        const int encodedPropertyIndex
                = target == TargetIsResolvedAlias ? -1 : propIdx.toEncoded();
        if (!appendAliasToPropertyCache(
                    alias, objectIndex, aliasIndex, encodedPropertyIndex,
                    m_compiler->objectId(targetObjectIndex), targetObjectIndex, aliasCacheCreator,
                    error)) {
            break;
        }
    }

    if (numSkippedAliases == aliasIndex)
        return NoAliasResolved;

    if (aliasIndex == obj->aliasCount() && !hasUnresolvedLocalAliases)
        return AllAliasesResolved;

    return SomeAliasesResolved;
}

template<typename ObjectContainer>
void QQmlComponentAndAliasResolver<ObjectContainer>::resolveGeneralizedGroupedProperties()
{
    for (CompiledBinding *binding : std::as_const(m_generalizedGroupedProperties))
        resolveGeneralizedGroupedProperty(binding);
}

QT_END_NAMESPACE

#endif // QQMLCOMPONENTANDALIASRESOLVER_P_H
