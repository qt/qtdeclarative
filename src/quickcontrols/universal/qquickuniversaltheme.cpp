// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickuniversaltheme_p.h"

#include <QtGui/qfontdatabase.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

void QQuickUniversalTheme::initialize(QQuickTheme *theme)
{
    QFont systemFont;
    QFont groupBoxTitleFont;
    QFont tabButtonFont;

    const QLatin1String segoeUiFamilyName("Segoe UI");
    if (QFontDatabase::families().contains(segoeUiFamilyName)) {
        const QFont font(segoeUiFamilyName);
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
