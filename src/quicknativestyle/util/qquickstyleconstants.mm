// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

QColor QQuickStyleConstants::secondarySystemFillColor() const
{
    if (@available(macOS 14.0, *))
        return qt_mac_toQBrush([NSColor secondarySystemFillColor]).color();
    return Qt::black;
}

QColor QQuickStyleConstants::tertiarySystemFillColor() const
{
    if (@available(macOS 14.0, *))
        return qt_mac_toQBrush([NSColor tertiarySystemFillColor]).color();
    return Qt::black;
}

QT_END_NAMESPACE

#include "moc_qquickstyleconstants.cpp"
