// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlmodelindexvaluetype_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal
*/
QString QQmlModelIndexValueType::propertiesString(const QModelIndex &idx)
{
    if (!idx.isValid())
        return QLatin1String("()");
    return QString(QLatin1String("(%1,%2,0x%3,%4(0x%5))"))
            .arg(idx.row()).arg(idx.column()).arg(idx.internalId(), 0, 16)
            .arg(QLatin1String(idx.model()->metaObject()->className())).arg(quintptr(idx.model()), 0, 16);
}

/*!
    \internal
*/
QString QQmlItemSelectionRangeValueType::toString() const
{
    return QString(QLatin1String("QItemSelectionRange(%1,%2)"))
            .arg(reinterpret_cast<const QQmlPersistentModelIndexValueType *>(&v.topLeft())->toString())
            .arg(reinterpret_cast<const QQmlPersistentModelIndexValueType *>(&v.bottomRight())->toString());
}

QT_END_NAMESPACE

#include "moc_qqmlmodelindexvaluetype_p.cpp"
