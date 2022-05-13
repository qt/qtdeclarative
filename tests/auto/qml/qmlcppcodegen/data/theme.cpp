// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "theme.h"

namespace Utils {

Theme::Theme(QObject *parent) : QObject{parent} {}

int Theme::index(Area area) const
{
    switch(area) {
        case TopLeft: return 12;
        case BottomRight: return 13;
    }
    return -1;
}

QRectF Theme::area(Area area) const
{
    switch(area) {
        case TopLeft: return QRectF(0.0, 0.0, 5.0, 10.0);
        case BottomRight: return QRectF(5.0, 10.0, 1.0, 1.0);
    }
    return QRectF();
}

}
