// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlmetatypedata_p.h"

#include <private/qqmltype_p_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmlpropertycache_p.h>

QT_BEGIN_NAMESPACE

QQmlMetaTypeData::QQmlMetaTypeData()
{
}

QQmlMetaTypeData::~QQmlMetaTypeData()
{
    {
        // Unregister all remaining composite types.
        // Avoid deletion recursion (via QQmlTypePrivate dtor) by moving them out of the way first.
        CompositeTypes emptyComposites;
        emptyComposites.swap(compositeTypes);
    }

    propertyCaches.clear();
    // Do this before the attached properties disappear.
    types.clear();
    undeletableTypes.clear();
    qDeleteAll(metaTypeToValueType);
    clearCompositeMetaTypes();
}

// This expects a "fresh" QQmlTypePrivate and adopts its reference.
void QQmlMetaTypeData::registerType(QQmlTypePrivate *priv)
{
    for (int i = 0; i < types.size(); ++i) {
        if (!types.at(i).isValid()) {
            types[i] = QQmlType(priv);
            priv->index = i;
            priv->release();
            return;
        }
    }
    types.append(QQmlType(priv));
    priv->index = types.size() - 1;
    priv->release();
}

QQmlMetaTypeData::VersionedUri::VersionedUri(const std::unique_ptr<QQmlTypeModule> &module)
    : uri(module->module()), majorVersion(module->majorVersion())
{
}

QQmlTypeModule *QQmlMetaTypeData::findTypeModule(const QString &module, QTypeRevision version)
{
    const auto qqtm = std::lower_bound(
                uriToModule.begin(), uriToModule.end(), VersionedUri(module, version),
                std::less<QQmlMetaTypeData::VersionedUri>());
    if (qqtm == uriToModule.end())
        return nullptr;

    QQmlTypeModule *candidate = qqtm->get();
    return (candidate->module() == module && candidate->majorVersion() == version.majorVersion())
            ? candidate
            : nullptr;
}

QQmlTypeModule *QQmlMetaTypeData::addTypeModule(std::unique_ptr<QQmlTypeModule> module)
{
    QQmlTypeModule *ret = module.get();
    uriToModule.emplace_back(std::move(module));
    std::sort(uriToModule.begin(), uriToModule.end(),
              [](const std::unique_ptr<QQmlTypeModule> &a,
                 const std::unique_ptr<QQmlTypeModule> &b) {
        const int diff = a->module().compare(b->module());
        return diff < 0 || (diff == 0 && a->majorVersion() < b->majorVersion());
    });
    return ret;
}

bool QQmlMetaTypeData::registerModuleTypes(const QString &uri)
{
    auto function = moduleTypeRegistrationFunctions.constFind(uri);
    if (function != moduleTypeRegistrationFunctions.constEnd()) {
        (*function)();
        return true;
    }
    return false;
}

QQmlPropertyCache::ConstPtr QQmlMetaTypeData::propertyCacheForVersion(
        int index, QTypeRevision version) const
{
    return (index < typePropertyCaches.size())
            ? typePropertyCaches.at(index).value(version)
            : QQmlPropertyCache::ConstPtr();
}

void QQmlMetaTypeData::setPropertyCacheForVersion(int index, QTypeRevision version,
                                                  const QQmlPropertyCache::ConstPtr &cache)
{
    if (index >= typePropertyCaches.size())
        typePropertyCaches.resize(index + 1);
    typePropertyCaches[index][version] = cache;
}

void QQmlMetaTypeData::clearPropertyCachesForVersion(int index)
{
    if (index < typePropertyCaches.size())
        typePropertyCaches[index].clear();
}

QQmlPropertyCache::ConstPtr QQmlMetaTypeData::propertyCache(
        const QMetaObject *metaObject, QTypeRevision version)
{
    if (QQmlPropertyCache::ConstPtr rv = propertyCaches.value(metaObject))
        return rv;

    QQmlPropertyCache::ConstPtr rv;
    if (const QMetaObject *superMeta = metaObject->superClass())
        rv = propertyCache(superMeta, version)->copyAndAppend(metaObject, version);
    else
        rv = QQmlPropertyCache::createStandalone(metaObject);

    const auto *mop = reinterpret_cast<const QMetaObjectPrivate *>(metaObject->d.data);
    if (!(mop->flags & DynamicMetaObject))
        propertyCaches.insert(metaObject, rv);

    return rv;
}

QQmlPropertyCache::ConstPtr QQmlMetaTypeData::propertyCache(
        const QQmlType &type, QTypeRevision version)
{
    Q_ASSERT(type.isValid());

    if (auto pc = propertyCacheForVersion(type.index(), version))
        return pc;

    QVector<QQmlType> types;

    quint8 maxMinorVersion = 0;

    const QMetaObject *metaObject = type.metaObject();
    Q_ASSERT(metaObject);

    const QTypeRevision combinedVersion = version.hasMajorVersion()
            ? version
            : (version.hasMinorVersion()
               ? QTypeRevision::fromVersion(type.version().majorVersion(),
                                            version.minorVersion())
               : QTypeRevision::fromMajorVersion(type.version().majorVersion()));

    while (metaObject) {
        QQmlType t = QQmlMetaType::qmlType(metaObject, type.module(), combinedVersion);
        if (t.isValid()) {
            maxMinorVersion = qMax(maxMinorVersion, t.version().minorVersion());
            types << t;
        } else {
            types << QQmlType();
        }

        metaObject = metaObject->superClass();
    }

    const QTypeRevision maxVersion = QTypeRevision::fromVersion(combinedVersion.majorVersion(),
                                                                maxMinorVersion);
    if (auto pc = propertyCacheForVersion(type.index(), maxVersion)) {
        setPropertyCacheForVersion(type.index(), maxVersion, pc);
        return pc;
    }

    QQmlPropertyCache::ConstPtr raw = propertyCache(type.metaObject(), combinedVersion);
    QQmlPropertyCache::Ptr copied;

    for (int ii = 0; ii < types.size(); ++ii) {
        const QQmlType &currentType = types.at(ii);
        if (!currentType.isValid())
            continue;

        QTypeRevision rev = currentType.metaObjectRevision();
        int moIndex = types.size() - 1 - ii;

        if (raw->allowedRevision(moIndex) != rev) {
            if (copied.isNull()) {
                copied = raw->copy();
                raw = copied;
            }
            copied->setAllowedRevision(moIndex, rev);
        }
    }

    // Test revision compatibility - the basic rule is:
    //    * Anything that is excluded, cannot overload something that is not excluded *

    // Signals override:
    //    * other signals and methods of the same name.
    //    * properties named on<Signal Name>
    //    * automatic <property name>Changed notify signals

    // Methods override:
    //    * other methods of the same name

    // Properties override:
    //    * other elements of the same name

#if 0
    bool overloadError = false;
    QString overloadName;

    for (QQmlPropertyCache::StringCache::ConstIterator iter = raw->stringCache.begin();
         !overloadError && iter != raw->stringCache.end();
         ++iter) {

        const QQmlPropertyData *d = *iter;
        if (raw->isAllowedInRevision(d))
            continue; // Not excluded - no problems

        // check that a regular "name" overload isn't happening
        const QQmlPropertyData *current = d;
        while (!overloadError && current) {
            current = d->overrideData(current);
            if (current && raw->isAllowedInRevision(current))
                overloadError = true;
        }
    }

    if (overloadError) {
        if (hasCopied) raw->release();

        error.setDescription(QLatin1String("Type ") + type.qmlTypeName() + QLatin1Char(' ') + QString::number(type.majorVersion()) + QLatin1Char('.') + QString::number(minorVersion) + QLatin1String(" contains an illegal property \"") + overloadName + QLatin1String("\".  This is an error in the type's implementation."));
        return 0;
    }
#endif

    setPropertyCacheForVersion(type.index(), version, raw);

    if (version != maxVersion)
        setPropertyCacheForVersion(type.index(), maxVersion, raw);

    return raw;
}

static QQmlPropertyCache::ConstPtr propertyCacheForPotentialInlineComponentType(
        QMetaType t, const QQmlMetaTypeData::CompositeTypes::const_iterator &iter) {
    if (t != (*iter)->metaType()) {
        // this is an inline component, and what we have in the iterator is currently the parent compilation unit
        for (auto &&icDatum: (*iter)->inlineComponentData)
            if (icDatum.qmlType.typeId() == t)
                return (*iter)->propertyCaches.at(icDatum.objectIndex);
    }
    return (*iter)->rootPropertyCache();
}

QQmlPropertyCache::ConstPtr QQmlMetaTypeData::findPropertyCacheInCompositeTypes(QMetaType t) const
{
    auto iter = compositeTypes.constFind(t.iface());
    return (iter == compositeTypes.constEnd())
            ? QQmlPropertyCache::ConstPtr()
            : propertyCacheForPotentialInlineComponentType(t, iter);
}

void QQmlMetaTypeData::clearCompositeMetaTypes()
{
    for (const auto &compositeMetaType : std::as_const(compositeMetaTypes)) {
        Q_ASSERT(compositeMetaType.type);
        QMetaType::unregisterMetaType(QMetaType(compositeMetaType.type));
        delete compositeMetaType.type;

        Q_ASSERT(compositeMetaType.listType);
        QMetaType::unregisterMetaType(QMetaType(compositeMetaType.listType));
        delete compositeMetaType.listType;
    }
    compositeMetaTypes.clear();
}

QT_END_NAMESPACE
