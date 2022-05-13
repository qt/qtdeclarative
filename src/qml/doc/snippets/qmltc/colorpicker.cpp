// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "colorpicker.h"

#include <QtCore/qlogging.h>

void MyColorPicker::setEncodedColor(double value)
{
    if (value < 0.0 || !(value < 1.0)) {
        qWarning("Bad value, %f, cannot get color from it!", value);
        return;
    }
    m_encodedColor = value;
}

QBindable<double> MyColorPicker::bindableEncodedColor()
{
    return QBindable<double>(&m_encodedColor);
}

QColor MyColorPicker::decodeColor()
{
    const double encodedValue = m_encodedColor;
    constexpr int rgbFirst = 0;
    constexpr int rgbLast = 256 * 256 * 256;
    const QRgb rgb = rgbFirst + (rgbLast - rgbFirst) * encodedValue;
    return QColor(rgb);
}
