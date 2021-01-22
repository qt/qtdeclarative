/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqmltypemodule_p_p.h"

#include <private/qqmltype_p_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

QQmlTypeModule::QQmlTypeModule(const QString &module, int majorVersion)
    : d(new QQmlTypeModulePrivate(module, majorVersion))
{
}

QQmlTypeModule::~QQmlTypeModule()
{
    delete d;
}

QString QQmlTypeModule::module() const
{
    // No need to lock. d->module is const
    return d->module;
}

int QQmlTypeModule::majorVersion() const
{
    // No need to lock. d->majorVersion is const
    return d->majorVersion;
}

int QQmlTypeModule::minimumMinorVersion() const
{
    return d->minMinorVersion.loadRelaxed();
}

int QQmlTypeModule::maximumMinorVersion() const
{
    return d->maxMinorVersion.loadRelaxed();
}

void QQmlTypeModule::addMinorVersion(int version)
{
    for (int oldVersion = d->minMinorVersion.loadRelaxed();
         oldVersion > version && !d->minMinorVersion.testAndSetOrdered(oldVersion, version);
         oldVersion = d->minMinorVersion.loadRelaxed()) {
    }

    for (int oldVersion = d->maxMinorVersion.loadRelaxed();
         oldVersion < version && !d->maxMinorVersion.testAndSetOrdered(oldVersion, version);
         oldVersion = d->maxMinorVersion.loadRelaxed()) {
    }
}

void QQmlTypeModule::add(QQmlTypePrivate *type)
{
    QMutexLocker lock(&d->mutex);
    addMinorVersion(type->version_min);

    QList<QQmlTypePrivate *> &list = d->typeHash[type->elementName];
    for (int ii = 0; ii < list.count(); ++ii) {
        QQmlTypePrivate *in_list = list.at(ii);
        Q_ASSERT(in_list);
        if (in_list->version_min < type->version_min) {
            list.insert(ii, type);
            return;
        } else if (in_list->version_min == type->version_min) {
            list[ii] = type;
            return;
        }
    }
    list.append(type);
}

void QQmlTypeModule::remove(const QQmlTypePrivate *type)
{
    QMutexLocker lock(&d->mutex);
    for (auto elementIt = d->typeHash.begin(); elementIt != d->typeHash.end();) {
        QQmlMetaType::removeQQmlTypePrivate(elementIt.value(), type);

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

bool QQmlTypeModule::isLocked() const
{
    return d->locked.loadRelaxed() != 0;
}

void QQmlTypeModule::lock()
{
    d->locked.storeRelaxed(1);
}

QQmlType QQmlTypeModule::type(const QHashedStringRef &name, int minor) const
{
    QMutexLocker lock(&d->mutex);
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
    QMutexLocker lock(&d->mutex);
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
    QMutexLocker lock(&d->mutex);
    for (auto typeCandidates = d->typeHash.begin(), end = d->typeHash.end();
         typeCandidates != end; ++typeCandidates) {
        for (auto type: typeCandidates.value()) {
            if (type->regType == QQmlType::CompositeSingletonType)
                callback(QQmlType(type));
        }
    }
}

QT_END_NAMESPACE
