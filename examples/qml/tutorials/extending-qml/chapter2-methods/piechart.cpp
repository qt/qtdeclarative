// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "piechart.h"
#include <QPainter>

PieChart::PieChart(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

QString PieChart::name() const
{
    return m_name;
}

void PieChart::setName(const QString &name)
{
    m_name = name;
}

QColor PieChart::color() const
{
    return m_color;
}

void PieChart::setColor(const QColor &color)
{
    m_color = color;
}

void PieChart::paint(QPainter *painter)
{
    QPen pen(m_color, 2);
    painter->setPen(pen);
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->drawPie(boundingRect().adjusted(1, 1, -1, -1), 90 * 16, 290 * 16);
}

//![0]
void PieChart::clearChart()
{
    setColor(QColor(Qt::transparent));
    update();

    emit chartCleared();
}
//![0]
