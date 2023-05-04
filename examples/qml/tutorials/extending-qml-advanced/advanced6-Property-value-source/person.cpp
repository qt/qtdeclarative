// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "person.h"

Person::Person(QObject *parent) : QObject(parent)
{
    m_shoe = new ShoeDescription(this);
}

int ShoeDescription::size() const
{
    return m_size;
}

void ShoeDescription::setSize(int size)
{
    if (m_size != size) {
        m_size = size;
        emit shoeChanged();
    }
}

QColor ShoeDescription::color() const
{
    return m_color;
}

void ShoeDescription::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit shoeChanged();
    }
}

QString ShoeDescription::brand() const
{
    return m_brand;
}

void ShoeDescription::setBrand(const QString &brand)
{
    if (m_brand != brand) {
        m_brand = brand;
        emit shoeChanged();
    }
}

qreal ShoeDescription::price() const
{
    return m_price;
}

void ShoeDescription::setPrice(qreal price)
{
    if (m_price != price) {
        m_price = price;
        emit shoeChanged();
    }
}

bool ShoeDescription::operatorEqualsImpl(const ShoeDescription &lhs, const ShoeDescription &rhs)
{
    return lhs.m_size == rhs.m_size && lhs.m_color == rhs.m_color && lhs.m_brand == rhs.m_brand
            && lhs.m_price == rhs.m_price;
}

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

ShoeDescription *Person::shoe() const
{
    return m_shoe;
}

void Person::setShoe(ShoeDescription *shoe)
{
    if (!shoe)
        return;

    if (*m_shoe != *shoe) {
        m_shoe = shoe;
        emit shoeChanged();
    }
}
