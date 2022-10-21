/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
QQmlRefPointer<QQmlPropertyCache> ResolvedTypeReference::propertyCache() const
{
    if (m_type.isValid())
        return m_typePropertyCache;
    else
        return m_compilationUnit->rootPropertyCache();
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QQmlRefPointer<QQmlPropertyCache> ResolvedTypeReference::createPropertyCache(QQmlEngine *engine)
{
    if (m_typePropertyCache) {
        return m_typePropertyCache;
    } else if (m_type.isValid()) {
        m_typePropertyCache = QQmlEnginePrivate::get(engine)->cache(m_type.metaObject(), m_version);
        return m_typePropertyCache;
    } else {
        Q_ASSERT(m_compilationUnit);
        return m_compilationUnit->rootPropertyCache();
    }
}

bool ResolvedTypeReference::addToHash(QCryptographicHash *hash, QQmlEngine *engine)
{
    if (m_type.isValid() && !m_type.isInlineComponentType()) {
        bool ok = false;
        hash->addData(createPropertyCache(engine)->checksum(&ok));
        return ok;
    }
    if (!m_compilationUnit)
        return false;
    hash->addData(m_compilationUnit->data->md5Checksum,
                  sizeof(m_compilationUnit->data->md5Checksum));
    return true;
}

} // namespace QV4

QT_END_NAMESPACE
