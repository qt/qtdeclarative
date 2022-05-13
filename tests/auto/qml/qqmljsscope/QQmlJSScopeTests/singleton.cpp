// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singleton.h"

QFont TestApplication::createDummyFont() const
{
    QFont font;
    font.setPixelSize(42);
    return font;
}
