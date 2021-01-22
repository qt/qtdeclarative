/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "qqmlpropertyresolver_p.h"

QT_BEGIN_NAMESPACE

QQmlPropertyData *QQmlPropertyResolver::property(const QString &name, bool *notInRevision,
                                                 RevisionCheck check) const
{
    if (notInRevision) *notInRevision = false;

    QQmlPropertyData *d = cache->property(name, nullptr, nullptr);

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


QQmlPropertyData *QQmlPropertyResolver::signal(const QString &name, bool *notInRevision) const
{
    if (notInRevision) *notInRevision = false;

    QQmlPropertyData *d = cache->property(name, nullptr, nullptr);
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
        QString propName = name.mid(0, name.length() - static_cast<int>(strlen("Changed")));

        d = property(propName, notInRevision);
        if (d)
            return cache->signal(d->notifyIndex());
    }

    return nullptr;
}

QT_END_NAMESPACE
