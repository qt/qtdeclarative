// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickstyleconstants.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQuickStyleConstants::QQuickStyleConstants()
{
}

bool QQuickStyleConstants::runningWithLiquidGlass() const
{
    return qt_apple_runningWithLiquidGlass();
}

QColor QQuickStyleConstants::accentColor() const
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000)
    if (@available(macOS 14.0, *))
        return qt_mac_toQBrush([NSColor controlAccentColor]).color();
#endif
    return Qt::black;
}
QColor QQuickStyleConstants::secondarySystemFillColor() const
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000)
    if (@available(macOS 14.0, *))
        return qt_mac_toQBrush([NSColor secondarySystemFillColor]).color();
#endif
    return Qt::black;
}

QColor QQuickStyleConstants::tertiarySystemFillColor() const
{
#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(140000)
    if (@available(macOS 14.0, *))
        return qt_mac_toQBrush([NSColor tertiarySystemFillColor]).color();
#endif
    return Qt::black;
}

QT_END_NAMESPACE

#include "moc_qquickstyleconstants.cpp"
