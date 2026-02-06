// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlpropertyresolver_p.h"
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlsignalnames_p.h>

QT_BEGIN_NAMESPACE

const QQmlPropertyData *QQmlPropertyResolver::property(
        const QString &name, bool *notInRevision, RevisionCheck check) const
{
    if (notInRevision)
        *notInRevision = false;

    const QQmlPropertyData *d = cache->property(name, nullptr, nullptr);

    // Find the first property
    while (d && d->isFunction())
        d = cache->overrideData(d);

    if (check != IgnoreRevision && d && !cache->isAllowedInRevision(d)) {
        if (notInRevision)
            *notInRevision = true;
        return nullptr;
    }

    return d;
}

const QQmlPropertyData *QQmlPropertyResolver::signal(
        const QString &name, bool *notInRevision, RevisionCheck check) const
{
    if (notInRevision)
        *notInRevision = false;

    const QQmlPropertyData *d = cache->property(name, nullptr, nullptr);

    while (d && !d->isFunction())
        d = cache->overrideData(d);

    if (check != IgnoreRevision && d && !cache->isAllowedInRevision(d)) {
        if (notInRevision)
            *notInRevision = true;
        return nullptr;
    }

    if (d && d->isSignal())
        return d;

    if (const auto propName = QQmlSignalNames::changedSignalNameToPropertyName(name)) {
        if (d = property(*propName, notInRevision, check); d)
            return cache->signal(d->notifyIndex());
    }

    return nullptr;
}

QT_END_NAMESPACE
