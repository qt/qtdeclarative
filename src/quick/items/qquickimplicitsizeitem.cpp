// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimplicitsizeitem_p.h"
#include "qquickimplicitsizeitem_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    QQuickImplicitSizeItem redefines the implicitWidth and implicitHeight
    properties as readonly, as some items (e.g. Image, where the implicit size
    represents the real size of the image) should not be able to have their
    implicit size modified.
*/

QQuickImplicitSizeItem::QQuickImplicitSizeItem(QQuickImplicitSizeItemPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
}

QT_END_NAMESPACE

#include "moc_qquickimplicitsizeitem_p.cpp"
