// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcolor_p.h"

QT_BEGIN_NAMESPACE

QQuickColor::QQuickColor(QObject *parent) :
    QObject(parent)
{
}

QColor QQuickColor::transparent(const QColor &color, qreal opacity) const
{
    const auto rgbColor = color.toRgb();
    return QColor(rgbColor.red(), rgbColor.green(), rgbColor.blue(),
                  int(qreal(255) * qBound(qreal(0), opacity, qreal(1))));
}

QColor QQuickColor::blend(const QColor &a, const QColor &b, qreal factor) const
{
    if (factor <= 0.0)
        return a;
    if (factor >= 1.0)
        return b;

    const auto rgbA = a.toRgb();
    const auto rgbB = b.toRgb();
    QColor color;
    color.setRedF(rgbA.redF() * (1.0 - factor) + rgbB.redF() * factor);
    color.setGreenF(rgbA.greenF() * (1.0 - factor) + rgbB.greenF() * factor);
    color.setBlueF(rgbA.blueF() * (1.0 - factor) + rgbB.blueF() * factor);
    return color;
}

QT_END_NAMESPACE

#include "moc_qquickcolor_p.cpp"
