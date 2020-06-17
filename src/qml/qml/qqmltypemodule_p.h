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

#ifndef QQMLTYPEMODULE_P_H
#define QQMLTYPEMODULE_P_H

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

#include <QtQml/qtqmlglobal.h>
#include <QtQml/private/qstringhash_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

#include <functional>

QT_BEGIN_NAMESPACE

class QQmlType;
class QQmlTypePrivate;
struct QQmlMetaTypeData;

namespace QV4 {
struct String;
}

class QQmlTypeModule
{
public:
    QQmlTypeModule() = default;
    QQmlTypeModule(const QString &uri, quint8 majorVersion)
        : m_module(uri), m_majorVersion(majorVersion)
    {}

    void add(QQmlTypePrivate *);
    void remove(const QQmlTypePrivate *type);

    bool isLocked() const { return m_locked.loadRelaxed() != 0; }
    void lock() { m_locked.storeRelaxed(1); }

    QString module() const
    {
        // No need to lock. m_module is const
        return m_module;
    }

    quint8 majorVersion() const
    {
        // No need to lock. d->majorVersion is const
        return m_majorVersion;
    }

    void addMinorVersion(quint8 minorVersion);
    quint8 minimumMinorVersion() const { return m_minMinorVersion.loadRelaxed(); }
    quint8 maximumMinorVersion() const { return m_maxMinorVersion.loadRelaxed(); }

    QQmlType type(const QHashedStringRef &, QTypeRevision version) const;
    QQmlType type(const QV4::String *, QTypeRevision version) const;

    void walkCompositeSingletons(const std::function<void(const QQmlType &)> &callback) const;

private:
    const QString m_module;
    const quint8 m_majorVersion = 0;

    // Can only ever decrease
    QAtomicInt m_minMinorVersion = std::numeric_limits<quint8>::max();

    // Can only ever increase
    QAtomicInt m_maxMinorVersion = 0;

    // Bool. Can only be set to 1 once.
    QAtomicInt m_locked = 0;

    using TypeHash = QStringHash<QList<QQmlTypePrivate *>>;
    TypeHash m_typeHash;

    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // QQMLTYPEMODULE_P_H
