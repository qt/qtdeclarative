// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlpropertyresolver_p.h"
#include <private/qqmlcontextdata_p.h>

QT_BEGIN_NAMESPACE

const QQmlPropertyData *QQmlPropertyResolver::property(const QString &name, bool *notInRevision,
                                                 RevisionCheck check) const
{
    if (notInRevision) *notInRevision = false;

    const QQmlPropertyData *d = cache->property(name, nullptr, nullptr);

    // Find the first property
    while (d && d->isFunction())
        d = cache->overrideData(d);

    if (check != IgnoreRevision && d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return nullptr;
    } else {
        return d;
    }
}


const QQmlPropertyData *QQmlPropertyResolver::signal(const QString &name, bool *notInRevision) const
{
    if (notInRevision) *notInRevision = false;

    const QQmlPropertyData *d = cache->property(name, nullptr, nullptr);
    if (notInRevision) *notInRevision = false;

    while (d && !(d->isFunction()))
        d = cache->overrideData(d);

    if (d && !cache->isAllowedInRevision(d)) {
        if (notInRevision) *notInRevision = true;
        return nullptr;
    } else if (d && d->isSignal()) {
        return d;
    }

    if (name.endsWith(QLatin1String("Changed"))) {
        QString propName = name.mid(0, name.size() - static_cast<int>(strlen("Changed")));

        d = property(propName, notInRevision);
        if (d)
            return cache->signal(d->notifyIndex());
    }

    return nullptr;
}

QT_END_NAMESPACE
