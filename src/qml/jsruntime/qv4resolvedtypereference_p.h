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

#ifndef QV4RESOLVEDTYPEREFERNCE_P_H
#define QV4RESOLVEDTYPEREFERNCE_P_H

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

#include <QtQml/private/qtqmlglobal_p.h>
#include <QtQml/private/qqmlrefcount_p.h>
#include <QtQml/private/qqmlpropertycache_p.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtQml/private/qv4executablecompilationunit_p.h>

QT_BEGIN_NAMESPACE

class QCryptographicHash;
namespace QV4 {

class ResolvedTypeReference
{
    Q_DISABLE_COPY_MOVE(ResolvedTypeReference)
public:
    ResolvedTypeReference() = default;
    ~ResolvedTypeReference()
    {
        if (m_stronglyReferencesCompilationUnit && m_compilationUnit)
            m_compilationUnit->release();
    }

    QQmlRefPointer<QQmlPropertyCache> propertyCache() const;
    QQmlRefPointer<QQmlPropertyCache> createPropertyCache(QQmlEngine *);
    bool addToHash(QCryptographicHash *hash, QQmlEngine *engine);

    void doDynamicTypeCheck();

    QQmlType type() const { return m_type; }
    void setType(QQmlType type)  {  m_type = std::move(type); }

    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit() { return m_compilationUnit; }
    void setCompilationUnit(QQmlRefPointer<QV4::ExecutableCompilationUnit> unit)
    {
        if (m_compilationUnit == unit.data())
            return;
        if (m_stronglyReferencesCompilationUnit) {
            if (m_compilationUnit)
                m_compilationUnit->release();
            m_compilationUnit = unit.take();
        } else {
            m_compilationUnit = unit.data();
        }
    }

    bool referencesCompilationUnit() const { return m_stronglyReferencesCompilationUnit; }
    void setReferencesCompilationUnit(bool doReference)
    {
        if (doReference == m_stronglyReferencesCompilationUnit)
            return;
        m_stronglyReferencesCompilationUnit = doReference;
        if (!m_compilationUnit)
            return;
        if (doReference) {
            m_compilationUnit->addref();
        } else if (m_compilationUnit->count() == 1) {
            m_compilationUnit->release();
            m_compilationUnit = nullptr;
        } else {
            m_compilationUnit->release();
        }
    }

    QQmlRefPointer<QQmlPropertyCache> typePropertyCache() const { return m_typePropertyCache; }
    void setTypePropertyCache(QQmlRefPointer<QQmlPropertyCache> cache)
    {
        m_typePropertyCache = std::move(cache);
    }

    QTypeRevision version() const { return m_version; }
    void setVersion(QTypeRevision version) { m_version = version; }

    bool isFullyDynamicType() const { return m_isFullyDynamicType; }
    void setFullyDynamicType(bool fullyDynamic) { m_isFullyDynamicType = fullyDynamic; }

private:
    QQmlType m_type;
    QQmlRefPointer<QQmlPropertyCache> m_typePropertyCache;
    QV4::ExecutableCompilationUnit *m_compilationUnit = nullptr;

    QTypeRevision m_version = QTypeRevision::zero();
    // Types such as QQmlPropertyMap can add properties dynamically at run-time and
    // therefore cannot have a property cache installed when instantiated.
    bool m_isFullyDynamicType = false;
    bool m_stronglyReferencesCompilationUnit = true;
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4RESOLVEDTYPEREFERNCE_P_H
