// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleconstants_p.h"

#include <private/qcore_mac_p.h>
#include <private/qcoregraphics_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>

#include <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQuickStyleConstants::QQuickStyleConstants()
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        emit secondarySystemFillColorChanged();
        emit tertiarySystemFillColorChanged();
    });
}

bool QQuickStyleConstants::runningWithLiquidGlass() const
{
    return qt_apple_runningWithLiquidGlass();
}

static QColor systemColor(std::function<NSColor *()> block)
{
    __block QColor color;
    [NSApp.effectiveAppearance performAsCurrentDrawingAppearance:^{
        color = qt_mac_toQBrush(block()).color();
    }];
    return color;
}

QColor QQuickStyleConstants::secondarySystemFillColor() const
{
    if (@available(macOS 14.0, *))
        return systemColor([] { return NSColor.secondarySystemFillColor; });
    return Qt::black;
}

QColor QQuickStyleConstants::tertiarySystemFillColor() const
{
    if (@available(macOS 14.0, *))
        return systemColor([] { return NSColor.tertiarySystemFillColor; });
    return Qt::black;
}

QT_END_NAMESPACE

#include "moc_qquickstyleconstants_p.cpp"
