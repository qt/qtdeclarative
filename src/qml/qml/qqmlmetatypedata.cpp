/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    for (TypeModules::const_iterator i = uriToModule.constBegin(), cend = uriToModule.constEnd(); i != cend; ++i)
        delete *i;
    for (QHash<const QMetaObject *, QQmlPropertyCache *>::Iterator it = propertyCaches.begin(), end = propertyCaches.end();
         it != end; ++it)
        (*it)->release();

    // Do this before the attached properties disappear.
    types.clear();
    undeletableTypes.clear();
}

// This expects a "fresh" QQmlTypePrivate and adopts its reference.
void QQmlMetaTypeData::registerType(QQmlTypePrivate *priv)
{
    for (int i = 0; i < types.count(); ++i) {
        if (!types.at(i).isValid()) {
            types[i] = QQmlType(priv);
            priv->index = i;
            priv->release();
            return;
        }
    }
    types.append(QQmlType(priv));
    priv->index = types.count() - 1;
    priv->release();
}

bool QQmlMetaTypeData::registerModuleTypes(const QQmlMetaTypeData::VersionedUri &versionedUri)
{
    auto function = moduleTypeRegistrationFunctions.constFind(versionedUri);
    if (function != moduleTypeRegistrationFunctions.constEnd()) {
        (*function)();
        return true;
    }
    return false;
}

QQmlPropertyCache *QQmlMetaTypeData::propertyCacheForMinorVersion(int index, int minorVersion) const
{
    return (index < typePropertyCaches.length())
            ? typePropertyCaches.at(index).value(minorVersion).data()
            : nullptr;
}

void QQmlMetaTypeData::setPropertyCacheForMinorVersion(int index, int minorVersion,
                                                       QQmlPropertyCache *cache)
{
    if (index >= typePropertyCaches.length())
        typePropertyCaches.resize(index + 1);
    typePropertyCaches[index][minorVersion] = cache;
}

void QQmlMetaTypeData::clearPropertyCachesForMinorVersion(int index)
{
    if (index < typePropertyCaches.length())
        typePropertyCaches[index].clear();
}

QQmlRefPointer<QQmlPropertyCache> QQmlMetaTypeData::propertyCache(const QMetaObject *metaObject, int minorVersion)
{
    if (QQmlPropertyCache *rv = propertyCaches.value(metaObject))
        return rv;

    if (!metaObject->superClass()) {
        QQmlPropertyCache *rv = new QQmlPropertyCache(metaObject);
        propertyCaches.insert(metaObject, rv);
        return rv;
    }
    auto super = propertyCache(metaObject->superClass(), minorVersion);
    QQmlPropertyCache *rv = super->copyAndAppend(metaObject, minorVersion);
    propertyCaches.insert(metaObject, rv);
    return rv;
}

QQmlPropertyCache *QQmlMetaTypeData::propertyCache(const QQmlType &type, int minorVersion)
{
    Q_ASSERT(type.isValid());

    if (QQmlPropertyCache *pc = propertyCacheForMinorVersion(type.index(), minorVersion))
        return pc;

    QVector<QQmlType> types;

    int maxMinorVersion = 0;

    const QMetaObject *metaObject = type.metaObject();

    while (metaObject) {
        QQmlType t = QQmlMetaType::qmlType(metaObject, type.module(), type.majorVersion(), minorVersion);
        if (t.isValid()) {
            maxMinorVersion = qMax(maxMinorVersion, t.minorVersion());
            types << t;
        } else {
            types << QQmlType();
        }

        metaObject = metaObject->superClass();
    }

    if (QQmlPropertyCache *pc = propertyCacheForMinorVersion(type.index(), maxMinorVersion)) {
        setPropertyCacheForMinorVersion(type.index(), minorVersion, pc);
        return pc;
    }

    QQmlPropertyCache *raw = propertyCache(type.metaObject(), minorVersion).data();

    bool hasCopied = false;

    for (int ii = 0; ii < types.count(); ++ii) {
        const QQmlType &currentType = types.at(ii);
        if (!currentType.isValid())
            continue;

        int rev = currentType.metaObjectRevision();
        int moIndex = types.count() - 1 - ii;

        if (raw->allowedRevision(moIndex) != rev) {
            if (!hasCopied) {
                // TODO: The copy should be mutable, and the original should be const
                //       Considering this, the setAllowedRevision() below does not violate
                //       the immutability of already published property caches.
                raw = raw->copy();
                hasCopied = true;
            }
            raw->setAllowedRevision(moIndex, rev);
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

        QQmlPropertyData *d = *iter;
        if (raw->isAllowedInRevision(d))
            continue; // Not excluded - no problems

        // check that a regular "name" overload isn't happening
        QQmlPropertyData *current = d;
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

    setPropertyCacheForMinorVersion(type.index(), minorVersion, raw);

    if (hasCopied)
        raw->release();

    if (minorVersion != maxMinorVersion)
        setPropertyCacheForMinorVersion(type.index(), maxMinorVersion, raw);

    return raw;
}

QT_END_NAMESPACE
