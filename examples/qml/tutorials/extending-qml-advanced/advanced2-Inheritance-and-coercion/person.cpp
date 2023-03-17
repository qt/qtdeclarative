// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "person.h"

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

int Person::shoeSize() const
{
    return m_shoeSize;
}

void Person::setShoeSize(int shoeSize)
{
    if (m_shoeSize != shoeSize) {
        m_shoeSize = shoeSize;
        emit shoeSizeChanged();
    }
}
