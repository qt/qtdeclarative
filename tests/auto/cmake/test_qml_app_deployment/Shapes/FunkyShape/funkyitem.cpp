// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "funkyitem.h"

#include <QPainter>

FunkyItem::FunkyItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

FunkyItem::~FunkyItem()
{
}

void FunkyItem::paint(QPainter *painter)
{
    const qreal halfPenWidth = qMax(painter->pen().width() / 2.0, 1.0);

    QRectF rect = boundingRect();
    rect.adjust(halfPenWidth, halfPenWidth, -halfPenWidth, -halfPenWidth);

    painter->drawEllipse(rect);
}
