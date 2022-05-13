// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "piechart.h"
#include "pieslice.h"

PieChart::PieChart(QQuickItem *parent)
    : QQuickItem(parent), m_pieSlice(nullptr)
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

PieSlice *PieChart::pieSlice() const
{
    return m_pieSlice;
}

//![0]
void PieChart::setPieSlice(PieSlice *pieSlice)
{
    m_pieSlice = pieSlice;
    pieSlice->setParentItem(this);
}
//![0]

