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

#ifndef QQMLPROPERTYRESOLVER_P_H
#define QQMLPROPERTYRESOLVER_P_H

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

#include <private/qtqmlglobal_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlrefcount_p.h>

QT_BEGIN_NAMESPACE

struct Q_QML_EXPORT QQmlPropertyResolver
{
    QQmlPropertyResolver(const QQmlRefPointer<QQmlPropertyCache> &cache)
        : cache(cache)
    {}

    QQmlPropertyData *property(int index) const
    {
        return cache->property(index);
    }

    enum RevisionCheck {
        CheckRevision,
        IgnoreRevision
    };

    QQmlPropertyData *property(const QString &name, bool *notInRevision = nullptr,
                               RevisionCheck check = CheckRevision) const;

    // This code must match the semantics of QQmlPropertyPrivate::findSignalByName
    QQmlPropertyData *signal(const QString &name, bool *notInRevision) const;

    QQmlRefPointer<QQmlPropertyCache> cache;
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYRESOLVER_P_H
