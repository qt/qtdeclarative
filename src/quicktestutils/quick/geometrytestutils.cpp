// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "geometrytestutils_p.h"

#include <QQuickItem>

QT_BEGIN_NAMESPACE

QSizeChangeListener::QSizeChangeListener(QQuickItem *item) :
    item(item)
{
    connect(item, &QQuickItem::widthChanged, this, &QSizeChangeListener::onSizeChanged);
    connect(item, &QQuickItem::heightChanged, this, &QSizeChangeListener::onSizeChanged);
}

void QSizeChangeListener::onSizeChanged()
{
    append(QSize(item->width(), item->height()));
}

QT_END_NAMESPACE

#include "moc_geometrytestutils_p.cpp"
