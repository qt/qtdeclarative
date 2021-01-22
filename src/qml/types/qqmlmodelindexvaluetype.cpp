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
