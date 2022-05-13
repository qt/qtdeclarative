// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcontentitem_p.h"

#include <QtQml/private/qqmlmetatype_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Helper class that aids debugging by producing more useful debugging output.
*/

QQuickContentItem::QQuickContentItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setObjectName(QQmlMetaType::prettyTypeName(parent));
}

QQuickContentItem::QQuickContentItem(const QObject *scope, QQuickItem *parent)
    : QQuickItem(parent)
{
    setObjectName(QQmlMetaType::prettyTypeName(scope));
}

QT_END_NAMESPACE

#include "moc_qquickcontentitem_p.cpp"
