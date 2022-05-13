// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "person.h"

int ShoeDescription::size() const
{
    return m_size;
}

void ShoeDescription::setSize(int s)
{
    m_size = s;
}

QColor ShoeDescription::color() const
{
    return m_color;
}

void ShoeDescription::setColor(const QColor &c)
{
    m_color = c;
}

QString ShoeDescription::brand() const
{
    return m_brand;
}

void ShoeDescription::setBrand(const QString &b)
{
    m_brand = b;
}

qreal ShoeDescription::price() const
{
    return m_price;
}

void ShoeDescription::setPrice(qreal p)
{
    m_price = p;
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    m_name = n;
}

ShoeDescription *Person::shoe()
{
    return &m_shoe;
}
