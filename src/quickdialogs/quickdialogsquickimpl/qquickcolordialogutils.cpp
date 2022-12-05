// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcolordialogutils_p.h"

std::pair<qreal, qreal> getSaturationAndValue(qreal saturation, qreal lightness)
{
    const qreal v = lightness + saturation * qMin(lightness, 1 - lightness);
    if (v == .0)
        return { .0, .0 };
    const qreal s = 2 * (1 - lightness / v);
    return { s, v };
}

std::pair<qreal, qreal> getSaturationAndLightness(qreal saturation, qreal value)
{
    const qreal l = value * (1 - saturation / 2);
    if (l == .0)
        return { .0, .0 };
    if (l == 1 && value == 1)
        return { saturation, l };
    const qreal s = (value - l) / qMin(l, 1 - l);
    return { s, l };
}
