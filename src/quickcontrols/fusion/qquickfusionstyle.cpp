// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfusionstyle_p.h"

#include <QtGui/qcolor.h>
#include <QtGui/qpalette.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtQuick/private/qquickpalette_p.h>

QT_BEGIN_NAMESPACE

QQuickFusionStyle::QQuickFusionStyle(QObject *parent)
    : QObject(parent)
{
}

QColor QQuickFusionStyle::lightShade()
{
    return QColor(255, 255, 255, 90);
}

QColor QQuickFusionStyle::darkShade()
{
    return QColor(0, 0, 0, 60);
}

QColor QQuickFusionStyle::topShadow()
{
    return QColor(0, 0, 0, 18);
}

QColor QQuickFusionStyle::innerContrastLine()
{
    return QColor(255, 255, 255, 30);
}

QColor QQuickFusionStyle::highlight(QQuickPalette *palette)
{
    return palette->highlight();
}

QColor QQuickFusionStyle::highlightedText(QQuickPalette *palette)
{
    return palette->highlightedText();
}

QColor QQuickFusionStyle::outline(QQuickPalette *palette)
{
    return palette->window().darker(140);
}

QColor QQuickFusionStyle::highlightedOutline(QQuickPalette *palette)
{
    QColor highlightedOutline = highlight(palette).darker(125).toHsv();
    if (highlightedOutline.value() > 160)
        highlightedOutline.setHsl(highlightedOutline.hue(), highlightedOutline.saturation(), 160);
    return highlightedOutline;
}

QColor QQuickFusionStyle::tabFrameColor(QQuickPalette *palette)
{
    return buttonColor(palette).lighter(104);
}

QColor QQuickFusionStyle::buttonColor(QQuickPalette *palette, bool highlighted, bool down, bool hovered)
{
    QColor buttonColor = palette->button();
    int val = qGray(buttonColor.rgb());
    buttonColor = buttonColor.lighter(100 + qMax(1, (180 - val)/6));
    buttonColor = buttonColor.toHsv();
    buttonColor.setHsv(buttonColor.hue(), int(buttonColor.saturation() * 0.75), buttonColor.value());
    if (highlighted)
        buttonColor = mergedColors(buttonColor, highlightedOutline(palette).lighter(130), 90);
    if (!hovered)
        buttonColor = buttonColor.darker(104);
    if (down)
        buttonColor = buttonColor.darker(110);
    return buttonColor;
}

QColor QQuickFusionStyle::buttonOutline(QQuickPalette *palette, bool highlighted, bool enabled)
{
    QColor darkOutline = enabled && highlighted ? highlightedOutline(palette) : outline(palette);
    return !enabled ? darkOutline.lighter(115) : darkOutline;
}

QColor QQuickFusionStyle::gradientStart(const QColor &baseColor)
{
    return baseColor.lighter(124);
}

QColor QQuickFusionStyle::gradientStop(const QColor &baseColor)
{
    return baseColor.lighter(102);
}

QColor QQuickFusionStyle::mergedColors(const QColor &colorA, const QColor &colorB, int factor)
{
    const int maxFactor = 100;
    const auto rgbColorB = colorB.toRgb();
    QColor tmp = colorA.toRgb();
    tmp.setRed((tmp.red() * factor) / maxFactor + (rgbColorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (rgbColorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (rgbColorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

QColor QQuickFusionStyle::grooveColor(QQuickPalette *palette)
{
    QColor color = buttonColor(palette).toHsv();
    color.setHsv(color.hue(),
                 qMin(255, color.saturation()),
                 qMin<int>(255, color.value() * 0.9));
    return color;
}

QT_END_NAMESPACE

#include "moc_qquickfusionstyle_p.cpp"
