// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickbasictheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

void QQuickBasicTheme::initialize(QQuickTheme *theme)
{
    QPalette systemPalette;

    systemPalette.setColor(QPalette::Base, QColor::fromRgba(0xFFFFFFFF));
    systemPalette.setColor(QPalette::Disabled, QPalette::Base, QColor::fromRgba(0xFFD6D6D6));

    systemPalette.setColor(QPalette::Button, QColor::fromRgba(0xFFE0E0E0));

    systemPalette.setColor(QPalette::ButtonText, QColor::fromRgba(0xFF26282A));
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgba(0x4D26282A));

    systemPalette.setColor(QPalette::BrightText, QColor::fromRgba(0xFFFFFFFF));
    systemPalette.setColor(QPalette::Disabled, QPalette::BrightText, QColor::fromRgba(0x4DFFFFFF));

    systemPalette.setColor(QPalette::Dark, QColor::fromRgba(0xFF353637));

    systemPalette.setColor(QPalette::Highlight, QColor::fromRgba(0xFF0066FF));
    systemPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor::fromRgba(0xFFF0F6FF));

    systemPalette.setColor(QPalette::HighlightedText, QColor::fromRgba(0xFF090909));

    systemPalette.setColor(QPalette::Light, QColor::fromRgba(0xFFF6F6F6));

    systemPalette.setColor(QPalette::Link, QColor::fromRgba(0xFF45A7D7));

    systemPalette.setColor(QPalette::Mid, QColor::fromRgba(0xFFBDBDBD));

    systemPalette.setColor(QPalette::Midlight, QColor::fromRgba(0xFFE4E4E4));

    systemPalette.setColor(QPalette::Text, QColor::fromRgba(0xFF353637));
    systemPalette.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgba(0x7F353637));

    systemPalette.setColor(QPalette::Shadow, QColor::fromRgba(0xFF28282A));

    systemPalette.setColor(QPalette::ToolTipBase, QColor::fromRgba(0xFFFFFFFF));
    systemPalette.setColor(QPalette::ToolTipText, QColor::fromRgba(0xFF000000));

    systemPalette.setColor(QPalette::Window, QColor::fromRgba(0xFFFFFFFF));

    systemPalette.setColor(QPalette::WindowText, QColor::fromRgba(0xFF26282A));
    systemPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgba(0xFFBDBEBF));

    systemPalette.setColor(QPalette::PlaceholderText, QColor::fromRgba(0x88353637));

    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE
