// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4resolvedtypereference_p.h"

#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/qqmlpropertymap.h>
#include <QtCore/qcryptographichash.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

template <typename T>
bool qtTypeInherits(const QMetaObject *mo) {
    while (mo) {
        if (mo == &T::staticMetaObject)
            return true;
        mo = mo->superClass();
    }
    return false;
}

void ResolvedTypeReference::doDynamicTypeCheck()
{
    const QMetaObject *mo = nullptr;
    if (m_typePropertyCache)
        mo = m_typePropertyCache->firstCppMetaObject();
    else if (m_type.isValid())
        mo = m_type.metaObject();
    else if (m_compilationUnit)
        mo = m_compilationUnit->rootPropertyCache()->firstCppMetaObject();
    m_isFullyDynamicType = qtTypeInherits<QQmlPropertyMap>(mo);
}

/*!
Returns the property cache, if one alread exists.  The cache is not referenced.
*/
QQmlPropertyCache::ConstPtr ResolvedTypeReference::propertyCache() const
{
    if (m_type.isValid())
        return m_typePropertyCache;
    else
        return m_compilationUnit->rootPropertyCache();
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QQmlPropertyCache::ConstPtr ResolvedTypeReference::createPropertyCache()
{
    if (m_typePropertyCache) {
        return m_typePropertyCache;
    } else if (m_type.isValid()) {
        const QMetaObject *metaObject = m_type.metaObject();
        if (!metaObject) // value type of non-Q_GADGET base with extension
            metaObject = m_type.extensionMetaObject();
        Q_ASSERT(metaObject);
        m_typePropertyCache = QQmlMetaType::propertyCache(metaObject, m_version);
        return m_typePropertyCache;
    } else {
        Q_ASSERT(m_compilationUnit);
        return m_compilationUnit->rootPropertyCache();
    }
}

bool ResolvedTypeReference::addToHash(
        QCryptographicHash *hash, QHash<quintptr, QByteArray> *checksums)
{
    if (m_type.isValid() && !m_type.isInlineComponentType()) {
        bool ok = false;
        hash->addData(createPropertyCache()->checksum(checksums, &ok));
        return ok;
    }
    if (!m_compilationUnit)
        return false;
    hash->addData({m_compilationUnit->data->md5Checksum,
                  sizeof(m_compilationUnit->data->md5Checksum)});
    return true;
}

} // namespace QV4

QT_END_NAMESPACE
