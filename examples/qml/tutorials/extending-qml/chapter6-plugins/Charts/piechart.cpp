// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "piechart.h"
#include "pieslice.h"

PieChart::PieChart(QQuickItem *parent)
    : QQuickItem(parent)
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

QQmlListProperty<PieSlice> PieChart::slices()
{
    return QQmlListProperty<PieSlice>(this, nullptr, &PieChart::append_slice, nullptr,
                                      nullptr, nullptr, nullptr, nullptr);
}

void PieChart::append_slice(QQmlListProperty<PieSlice> *list, PieSlice *slice)
{
    PieChart *chart = qobject_cast<PieChart *>(list->object);
    if (chart) {
        slice->setParentItem(chart);
        chart->m_slices.append(slice);
    }
}
