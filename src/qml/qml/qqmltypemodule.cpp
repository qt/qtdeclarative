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

#include "qqmltypemodule_p.h"

#include <private/qqmltype_p_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

void QQmlTypeModule::addMinorVersion(quint8 version)
{
    for (int oldVersion = m_minMinorVersion.loadRelaxed();
         oldVersion > version && !m_minMinorVersion.testAndSetOrdered(oldVersion, version);
         oldVersion = m_minMinorVersion.loadRelaxed()) {
    }

    for (int oldVersion = m_maxMinorVersion.loadRelaxed();
         oldVersion < version && !m_maxMinorVersion.testAndSetOrdered(oldVersion, version);
         oldVersion = m_maxMinorVersion.loadRelaxed()) {
    }
}

void QQmlTypeModule::add(QQmlTypePrivate *type)
{
    QMutexLocker lock(&m_mutex);

    if (type->version.hasMinorVersion())
        addMinorVersion(type->version.minorVersion());

    QList<QQmlTypePrivate *> &list = m_typeHash[type->elementName];
    for (int ii = 0; ii < list.count(); ++ii) {
        QQmlTypePrivate *in_list = list.at(ii);
        Q_ASSERT(in_list);
        if (in_list->version.minorVersion() < type->version.minorVersion()) {
            list.insert(ii, type);
            return;
        } else if (in_list->version.minorVersion() == type->version.minorVersion()) {
            list[ii] = type;
            return;
        }
    }
    list.append(type);
}

void QQmlTypeModule::remove(const QQmlTypePrivate *type)
{
    QMutexLocker lock(&m_mutex);
    for (auto elementIt = m_typeHash.begin(); elementIt != m_typeHash.end(); ++elementIt)
        QQmlMetaType::removeQQmlTypePrivate(elementIt.value(), type);
}

QQmlType QQmlTypeModule::findType(const QList<QQmlTypePrivate *> *types, QTypeRevision version)
{
    if (types) {
        for (int ii = 0; ii < types->count(); ++ii)
            if (types->at(ii)->version.minorVersion() <= version.minorVersion())
                return QQmlType(types->at(ii));
    }

    return QQmlType();
}

void QQmlTypeModule::walkCompositeSingletons(const std::function<void(const QQmlType &)> &callback) const
{
    QMutexLocker lock(&m_mutex);
    for (auto typeCandidates = m_typeHash.begin(), end = m_typeHash.end();
         typeCandidates != end; ++typeCandidates) {
        for (auto type: typeCandidates.value()) {
            if (type->regType == QQmlType::CompositeSingletonType)
                callback(QQmlType(type));
        }
    }
}

QT_END_NAMESPACE
