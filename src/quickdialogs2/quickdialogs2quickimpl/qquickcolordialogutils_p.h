// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKCOLORDIALOGUTILS_P_H
#define QQUICKCOLORDIALOGUTILS_P_H

#include <QtCore/QtGlobal>

std::pair<qreal, qreal> getSaturationAndValue(qreal saturation, qreal lightness);

std::pair<qreal, qreal> getSaturationAndLightness(qreal saturation, qreal value);

struct HSVA
{
    qreal h = .0;
    qreal s = .0;
    union {
        qreal v = 1.0;
        qreal l;
    };
    qreal a = 1.0;
};

#endif // QQUICKCOLORDIALOGUTILS_P_H
