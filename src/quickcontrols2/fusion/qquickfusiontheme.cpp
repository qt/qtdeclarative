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
