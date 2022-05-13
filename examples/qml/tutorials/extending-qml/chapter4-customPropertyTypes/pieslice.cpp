// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "pieslice.h"

#include <QPainter>

PieSlice::PieSlice(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QColor PieSlice::color() const
{
    return m_color;
}

void PieSlice::setColor(const QColor &color)
{
    m_color = color;
}

void PieSlice::paint(QPainter *painter)
{
    QPen pen(m_color, 2);
    painter->setPen(pen);
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->drawPie(boundingRect().adjusted(1, 1, -1, -1), 90 * 16, 290 * 16);
}

