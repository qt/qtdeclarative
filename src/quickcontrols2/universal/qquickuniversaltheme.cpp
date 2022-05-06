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

#include "qquickuniversaltheme_p.h"

#include <QtGui/qfontinfo.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

void QQuickUniversalTheme::initialize(QQuickTheme *theme)
{
    QFont systemFont;
    QFont groupBoxTitleFont;
    QFont tabButtonFont;

    const QFont font(QLatin1String("Segoe UI"));
    if (QFontInfo(font).family() == QLatin1String("Segoe UI")) {
        const QStringList families{font.family()};
        systemFont.setFamilies(families);
        groupBoxTitleFont.setFamilies(families);
        tabButtonFont.setFamilies(families);
    }

    systemFont.setPixelSize(15);
    theme->setFont(QQuickTheme::System, systemFont);

    groupBoxTitleFont.setPixelSize(15);
    groupBoxTitleFont.setWeight(QFont::DemiBold);
    theme->setFont(QQuickTheme::GroupBox, groupBoxTitleFont);

    tabButtonFont.setPixelSize(24);
    tabButtonFont.setWeight(QFont::Light);
    theme->setFont(QQuickTheme::TabBar, tabButtonFont);
}

QT_END_NAMESPACE
