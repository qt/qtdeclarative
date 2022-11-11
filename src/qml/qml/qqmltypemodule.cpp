// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    for (int ii = 0; ii < list.size(); ++ii) {
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
        for (int ii = 0; ii < types->size(); ++ii)
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
