// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "foreign.h"

int Foreign::things() const
{
    return m_things;
}

void Foreign::setThings(int things)
{
    if (m_things == things)
        return;

    m_things = things;
    emit thingsChanged(m_things);
}
