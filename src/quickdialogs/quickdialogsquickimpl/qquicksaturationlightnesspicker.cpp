// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksaturationlightnesspicker_p.h"
#include "qquickabstractcolorpicker_p_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickdeferredexecute_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickSaturationLightnessPickerPrivate : public QQuickAbstractColorPickerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSaturationLightnessPicker)

public:
    explicit QQuickSaturationLightnessPickerPrivate();
};
QQuickSaturationLightnessPickerPrivate::QQuickSaturationLightnessPickerPrivate()
{
    m_hsl = true;
}

QQuickSaturationLightnessPicker::QQuickSaturationLightnessPicker(QQuickItem *parent)
    : QQuickAbstractColorPicker(*(new QQuickSaturationLightnessPickerPrivate), parent)
{
}

QColor QQuickSaturationLightnessPicker::colorAt(const QPointF &pos)
{
    const qreal w = width();
    const qreal h = height();
    if (w <= 0 || h <= 0)
        return color();
    const qreal x = qBound(.0, pos.x(), w);
    const qreal y = qBound(.0, pos.y(), h);
    const qreal saturation = 1.0 - (y / h);
    const qreal lightness = x / w;

    return QColor::fromHslF(hue(), saturation, lightness);
}

QT_END_NAMESPACE
