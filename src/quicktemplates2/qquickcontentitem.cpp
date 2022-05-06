/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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
******************************************************************************/

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
