/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
    QColor highlightedOutline = highlight(palette).darker(125);
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
    QColor tmp = colorA;
    tmp.setRed((tmp.red() * factor) / maxFactor + (colorB.red() * (maxFactor - factor)) / maxFactor);
    tmp.setGreen((tmp.green() * factor) / maxFactor + (colorB.green() * (maxFactor - factor)) / maxFactor);
    tmp.setBlue((tmp.blue() * factor) / maxFactor + (colorB.blue() * (maxFactor - factor)) / maxFactor);
    return tmp;
}

QColor QQuickFusionStyle::grooveColor(QQuickPalette *palette)
{
    QColor color = buttonColor(palette);
    color.setHsv(color.hue(),
                 qMin(255, color.saturation()),
                 qMin<int>(255, color.value() * 0.9));
    return color;
}

QT_END_NAMESPACE

#include "moc_qquickfusionstyle_p.cpp"
