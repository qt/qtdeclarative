// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbasicstyle_p.h"

QT_BEGIN_NAMESPACE

QQuickBasicStyle::QQuickBasicStyle(QObject *parent) :
    QObject(parent)
{
}

QColor QQuickBasicStyle::backgroundColor() const
{
    return QColor::fromRgba(0xFFFFFFFF);
}

QColor QQuickBasicStyle::overlayModalColor() const
{
    return QColor::fromRgba(0x7F28282A);
}

QColor QQuickBasicStyle::overlayDimColor() const
{
    return QColor::fromRgba(0x1F28282A);
}

QColor QQuickBasicStyle::textColor() const
{
    return QColor::fromRgba(0xFF353637);
}

QColor QQuickBasicStyle::textDarkColor() const
{
    return QColor::fromRgba(0xFF26282A);
}

QColor QQuickBasicStyle::textLightColor() const
{
    return QColor::fromRgba(0xFFFFFFFF);
}

QColor QQuickBasicStyle::textLinkColor() const
{
    return QColor::fromRgba(0xFF45A7D7);
}

QColor QQuickBasicStyle::textSelectionColor() const
{
    return QColor::fromRgba(0xFFFDDD5C);
}

QColor QQuickBasicStyle::textDisabledColor() const
{
    return QColor::fromRgba(0xFFBDBEBF);
}

QColor QQuickBasicStyle::textDisabledLightColor() const
{
    return QColor::fromRgba(0xFFC2C2C2);
}

QColor QQuickBasicStyle::textPlaceholderColor() const
{
    return QColor::fromRgba(0xFF777777);
}

QColor QQuickBasicStyle::focusColor() const
{
    return QColor::fromRgba(0xFF0066FF);
}

QColor QQuickBasicStyle::focusLightColor() const
{
    return QColor::fromRgba(0xFFF0F6FF);
}

QColor QQuickBasicStyle::focusPressedColor() const
{
    return QColor::fromRgba(0xFFCCE0FF);
}

QColor QQuickBasicStyle::buttonColor() const
{
    return QColor::fromRgba(0xFFE0E0E0);
}

QColor QQuickBasicStyle::buttonPressedColor() const
{
    return QColor::fromRgba(0xFFD0D0D0);
}

QColor QQuickBasicStyle::buttonCheckedColor() const
{
    return QColor::fromRgba(0xFF353637);
}

QColor QQuickBasicStyle::buttonCheckedPressedColor() const
{
    return QColor::fromRgba(0xFF585A5C);
}

QColor QQuickBasicStyle::buttonCheckedFocusColor() const
{
    return QColor::fromRgba(0xFF599BFF);
}

QColor QQuickBasicStyle::toolButtonColor() const
{
    return QColor::fromRgba(0x33333333);
}

QColor QQuickBasicStyle::tabButtonColor() const
{
    return QColor::fromRgba(0xFF353637);
}

QColor QQuickBasicStyle::tabButtonPressedColor() const
{
    return QColor::fromRgba(0xFF585A5C);
}

QColor QQuickBasicStyle::tabButtonCheckedPressedColor() const
{
    return QColor::fromRgba(0xFFE4E4E4);
}

QColor QQuickBasicStyle::delegateColor() const
{
    return QColor::fromRgba(0xFFEEEEEE);
}

QColor QQuickBasicStyle::delegatePressedColor() const
{
    return QColor::fromRgba(0xFFBDBEBF);
}

QColor QQuickBasicStyle::delegateFocusColor() const
{
    return QColor::fromRgba(0xFFE5EFFF);
}

QColor QQuickBasicStyle::indicatorPressedColor() const
{
    return QColor::fromRgba(0xFFF6F6F6);
}

QColor QQuickBasicStyle::indicatorDisabledColor() const
{
    return QColor::fromRgba(0xFFFDFDFD);
}

QColor QQuickBasicStyle::indicatorFrameColor() const
{
    return QColor::fromRgba(0xFF909090);
}

QColor QQuickBasicStyle::indicatorFramePressedColor() const
{
    return QColor::fromRgba(0xFF808080);
}

QColor QQuickBasicStyle::indicatorFrameDisabledColor() const
{
    return QColor::fromRgba(0xFFD6D6D6);
}

QColor QQuickBasicStyle::frameDarkColor() const
{
    return QColor::fromRgba(0xFF353637);
}

QColor QQuickBasicStyle::frameLightColor() const
{
    return QColor::fromRgba(0xFFBDBEBF);
}

QColor QQuickBasicStyle::scrollBarColor() const
{
    return QColor::fromRgba(0xFFBDBEBF);
}

QColor QQuickBasicStyle::scrollBarPressedColor() const
{
    return QColor::fromRgba(0xFF28282A);
}

QColor QQuickBasicStyle::progressBarColor() const
{
    return QColor::fromRgba(0xFFE4E4E4);
}

QColor QQuickBasicStyle::pageIndicatorColor() const
{
    return QColor::fromRgba(0xFF28282A);
}

QColor QQuickBasicStyle::separatorColor() const
{
    return QColor::fromRgba(0xFFCCCCCC);
}

QColor QQuickBasicStyle::disabledDarkColor() const
{
    return QColor::fromRgba(0xFF353637);
}

QColor QQuickBasicStyle::disabledLightColor() const
{
    return QColor::fromRgba(0xFFBDBEBF);
}

QT_END_NAMESPACE

#include "moc_qquickbasicstyle_p.cpp"
