/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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
