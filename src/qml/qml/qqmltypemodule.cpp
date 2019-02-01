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

#include "qqmltypemodule_p_p.h"

#include <private/qqmltype_p_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

QQmlTypeModule::QQmlTypeModule()
    : d(new QQmlTypeModulePrivate)
{
}

QQmlTypeModule::~QQmlTypeModule()
{
    delete d; d = nullptr;
}

QString QQmlTypeModule::module() const
{
    return d->uri.uri;
}

int QQmlTypeModule::majorVersion() const
{
    return d->uri.majorVersion;
}

int QQmlTypeModule::minimumMinorVersion() const
{
    return d->minMinorVersion;
}

int QQmlTypeModule::maximumMinorVersion() const
{
    return d->maxMinorVersion;
}

void QQmlTypeModulePrivate::add(QQmlTypePrivate *type)
{
    int minVersion = type->version_min;
    minMinorVersion = qMin(minMinorVersion, minVersion);
    maxMinorVersion = qMax(maxMinorVersion, minVersion);

    QList<QQmlTypePrivate *> &list = typeHash[type->elementName];
    for (int ii = 0; ii < list.count(); ++ii) {
        Q_ASSERT(list.at(ii));
        if (list.at(ii)->version_min < minVersion) {
            list.insert(ii, type);
            return;
        }
    }
    list.append(type);
}

void QQmlTypeModulePrivate::remove(const QQmlTypePrivate *type)
{
    for (TypeHash::ConstIterator elementIt = typeHash.begin(); elementIt != typeHash.end();) {
        QList<QQmlTypePrivate *> &list = const_cast<QList<QQmlTypePrivate *> &>(elementIt.value());

        QQmlMetaType::removeQQmlTypePrivate(list, type);

#if 0
        if (list.isEmpty())
            elementIt = typeHash.erase(elementIt);
        else
            ++elementIt;
#else
        ++elementIt;
#endif
    }
}

QQmlType QQmlTypeModule::type(const QHashedStringRef &name, int minor) const
{
    QMutexLocker lock(QQmlMetaType::typeRegistrationLock());

    QList<QQmlTypePrivate *> *types = d->typeHash.value(name);
    if (types) {
        for (int ii = 0; ii < types->count(); ++ii)
            if (types->at(ii)->version_min <= minor)
                return QQmlType(types->at(ii));
    }

    return QQmlType();
}

QQmlType QQmlTypeModule::type(const QV4::String *name, int minor) const
{
    QMutexLocker lock(QQmlMetaType::typeRegistrationLock());

    QList<QQmlTypePrivate *> *types = d->typeHash.value(name);
    if (types) {
        for (int ii = 0; ii < types->count(); ++ii)
            if (types->at(ii)->version_min <= minor)
                return QQmlType(types->at(ii));
    }

    return QQmlType();
}

void QQmlTypeModule::walkCompositeSingletons(const std::function<void(const QQmlType &)> &callback) const
{
    QMutexLocker lock(QQmlMetaType::typeRegistrationLock());
    for (auto typeCandidates = d->typeHash.begin(), end = d->typeHash.end();
         typeCandidates != end; ++typeCandidates) {
        for (auto type: typeCandidates.value()) {
            if (type->regType == QQmlType::CompositeSingletonType)
                callback(QQmlType(type));
        }
    }
}

QT_END_NAMESPACE
