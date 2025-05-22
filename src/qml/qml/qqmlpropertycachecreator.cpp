// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertycachecreator_p.h"

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(invalidOverride, "qt.qml.invalidOverride");

QAtomicInt QQmlPropertyCacheCreatorBase::classIndexCounter(0);

template<typename BaseNameHandler, typename FailHandler>
auto processUrlForClassName(
    const QUrl &url, BaseNameHandler &&baseNameHandler, FailHandler &&failHandler)
{
    const QString path = url.path();

    // Not a reusable type if we don't have an absolute Url
    const qsizetype lastSlash = path.lastIndexOf(QLatin1Char('/'));
    if (lastSlash <= -1)
        return failHandler();

    // ### this might not be correct for .ui.qml files
    const QStringView baseName = QStringView{path}.mid(lastSlash + 1, path.size() - lastSlash - 5);

    // Not a reusable type if it doesn't start with a upper case letter.
    return (!baseName.isEmpty() && baseName.at(0).isUpper())
        ? baseNameHandler(baseName)
        : failHandler();
}

bool QQmlPropertyCacheCreatorBase::canCreateClassNameTypeByUrl(const QUrl &url)
{
    return processUrlForClassName(url, [](QStringView) {
        return true;
    }, []() {
        return false;
    });
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameTypeByUrl(const QUrl &url)
{
    return processUrlForClassName(url, [](QStringView nameBase) {
        return nameBase.toUtf8() + QByteArray("_QMLTYPE_");
    }, []() {
        return QByteArray("ANON_QML_TYPE_");
    }) + QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
}

QByteArray QQmlPropertyCacheCreatorBase::createClassNameForInlineComponent(const QUrl &url)
{
    QByteArray baseName = processUrlForClassName(url, [](QStringView nameBase) {
        return QByteArray(nameBase.toUtf8() + "_QMLTYPE_");
    }, []() {
        return QByteArray("ANON_QML_IC_");
    });
    return baseName + url.fragment().toUtf8() + '_'
            + QByteArray::number(classIndexCounter.fetchAndAddRelaxed(1));
}

QQmlBindingInstantiationContext::QQmlBindingInstantiationContext(
        int referencingObjectIndex,
        const QV4::CompiledData::Binding *instantiatingBinding,
        const QString &instantiatingPropertyName,
        const QQmlPropertyCache::ConstPtr &referencingObjectPropertyCache)
    : referencingObjectIndex(referencingObjectIndex)
    , instantiatingBinding(instantiatingBinding)
    , instantiatingPropertyName(instantiatingPropertyName)
    , referencingObjectPropertyCache(referencingObjectPropertyCache)
{
}

bool QQmlBindingInstantiationContext::resolveInstantiatingProperty()
{
    if (!instantiatingBinding
           || instantiatingBinding->type() != QV4::CompiledData::Binding::Type_GroupProperty) {
        return true;
    }

    if (!referencingObjectPropertyCache)
        return false;

    Q_ASSERT(referencingObjectIndex >= 0);
    Q_ASSERT(instantiatingBinding->propertyNameIndex != 0);

    bool notInRevision = false;
    instantiatingProperty = QQmlPropertyResolver(referencingObjectPropertyCache)
                                    .property(instantiatingPropertyName, &notInRevision,
                                              QQmlPropertyResolver::IgnoreRevision);
    return instantiatingProperty != nullptr;
}

QQmlPropertyCache::ConstPtr QQmlBindingInstantiationContext::instantiatingPropertyCache() const
{
    if (instantiatingProperty) {
        if (instantiatingProperty->isQObject()) {
            // rawPropertyCacheForType assumes a given unspecified version means "any version".
            // There is another overload that takes no version, which we shall not use here.
            auto result = QQmlMetaType::rawPropertyCacheForType(instantiatingProperty->propType(),
                                                         instantiatingProperty->typeVersion());
            if (result)
                return result;
            /* We might end up here if there's a grouped property, and the type hasn't been registered.
               Still try to get a property cache, as long as the type of the property is well-behaved
               (i.e., not dynamic)*/
            if (auto metaObject = instantiatingProperty->propType().metaObject(); metaObject) {
                // we'll warn about dynamic meta-object later in the property validator
                if (!(QMetaObjectPrivate::get(metaObject)->flags & DynamicMetaObject))
                    return QQmlMetaType::propertyCache(metaObject);
            }
            // fall through intentional
        } else if (const QMetaObject *vtmo = QQmlMetaType::metaObjectForValueType(instantiatingProperty->propType())) {
            return QQmlMetaType::propertyCache(vtmo, instantiatingProperty->typeVersion());
        }
    }
    return QQmlPropertyCache::ConstPtr();
}

void QQmlPendingGroupPropertyBindings::resolveMissingPropertyCaches(
        QQmlPropertyCacheVector *propertyCaches) const
{
    for (QQmlBindingInstantiationContext pendingBinding: *this) {
        const int groupPropertyObjectIndex = pendingBinding.instantiatingBinding->value.objectIndex;

        if (propertyCaches->at(groupPropertyObjectIndex))
            continue;

        Q_ASSERT(!pendingBinding.instantiatingPropertyName.isEmpty());

        if (!pendingBinding.referencingObjectPropertyCache) {
            pendingBinding.referencingObjectPropertyCache
                    = propertyCaches->at(pendingBinding.referencingObjectIndex);
        }

        if (!pendingBinding.resolveInstantiatingProperty())
            continue;
        if (auto cache = pendingBinding.instantiatingPropertyCache())
            propertyCaches->set(groupPropertyObjectIndex, cache);
    }
}

QT_END_NAMESPACE
