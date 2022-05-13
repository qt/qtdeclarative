// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfusiontheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

void QQuickFusionTheme::initialize(QQuickTheme *theme)
{
    const bool isDarkSystemTheme = QQuickStylePrivate::isDarkSystemTheme();
    QPalette systemPalette;
    systemPalette.setColor(QPalette::Active, QPalette::ButtonText,
        isDarkSystemTheme ? QColor::fromRgb(0xe7e7e7) : QColor::fromRgb(0x252525));
    systemPalette.setColor(QPalette::Inactive, QPalette::ButtonText,
        isDarkSystemTheme ? QColor::fromRgb(0xe7e7e7) : QColor::fromRgb(0x252525));
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText,
        isDarkSystemTheme ? QColor::fromRgb(0x777777) : QColor::fromRgb(0xb6b6b6));
    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE
