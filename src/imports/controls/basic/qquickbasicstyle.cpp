/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
