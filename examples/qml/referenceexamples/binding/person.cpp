// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "person.h"

int ShoeDescription::size() const
{
    return m_size;
}

void ShoeDescription::setSize(int s)
{
    if (m_size == s)
        return;

    m_size = s;
    emit shoeChanged();
}

QColor ShoeDescription::color() const
{
    return m_color;
}

void ShoeDescription::setColor(const QColor &c)
{
    if (m_color == c)
        return;

    m_color = c;
    emit shoeChanged();
}

QString ShoeDescription::brand() const
{
    return m_brand;
}

void ShoeDescription::setBrand(const QString &b)
{
    if (m_brand == b)
        return;

    m_brand = b;
    emit shoeChanged();
}

qreal ShoeDescription::price() const
{
    return m_price;
}

void ShoeDescription::setPrice(qreal p)
{
    if (m_price == p)
        return;

    m_price = p;
    emit shoeChanged();
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    if (m_name == n)
        return;

    m_name = n;
    emit nameChanged();
}

ShoeDescription *Person::shoe()
{
    return &m_shoe;
}
