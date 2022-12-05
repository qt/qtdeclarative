// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimaginetheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

void QQuickImagineTheme::initialize(QQuickTheme *theme)
{
    QFont systemFont;
    systemFont.setFamilies(QStringList{QLatin1String("Open Sans")});
    theme->setFont(QQuickTheme::System, systemFont);

    const QColor accentColor = QColor::fromRgb(0x4fc1e9);
    const QColor windowTextColor = QColor::fromRgb(0x434a54);
    const QColor disabledWindowTextColor = QColor::fromRgb(0xccd1d9);

    QPalette systemPalette;
    systemPalette.setColor(QPalette::ButtonText, Qt::white);
    systemPalette.setColor(QPalette::BrightText, Qt::white);
    systemPalette.setColor(QPalette::Highlight, accentColor);
    systemPalette.setColor(QPalette::HighlightedText, Qt::white);
    systemPalette.setColor(QPalette::Text, windowTextColor);
    systemPalette.setColor(QPalette::ToolTipText, Qt::white);
    systemPalette.setColor(QPalette::WindowText, windowTextColor);
    systemPalette.setColor(QPalette::Disabled, QPalette::Text, disabledWindowTextColor);
    systemPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledWindowTextColor);
    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE
