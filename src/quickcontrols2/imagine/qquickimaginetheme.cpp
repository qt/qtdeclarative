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
