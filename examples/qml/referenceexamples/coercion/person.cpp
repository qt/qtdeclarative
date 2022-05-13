// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "person.h"

QString Person::name() const
{
    return m_name;
}

void Person::setName(const QString &n)
{
    m_name = n;
}

int Person::shoeSize() const
{
    return m_shoeSize;
}

void Person::setShoeSize(int s)
{
    m_shoeSize = s;
}
