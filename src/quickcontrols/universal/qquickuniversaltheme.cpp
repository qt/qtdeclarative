// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickuniversaltheme_p.h"
#include "qquickuniversalstyle_p.h"

#include <QtCore/qmutex.h>
#include <QtGui/qfontdatabase.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

struct QQuickUniversalThemePrivate
{
    static inline void addSystemStyle(QPointer<QQuickUniversalStyle> style);
    static inline void removeSystemStyle(QPointer<QQuickUniversalStyle> style);
    static inline void updateSystemStyles();
    static inline std::vector<QPointer<QQuickUniversalStyle>> systemStyles = {};
    static inline QMutex mutex;
};

void QQuickUniversalThemePrivate::addSystemStyle(QPointer<QQuickUniversalStyle> style)
{
    QMutexLocker locker{&mutex};
    auto it = std::find(systemStyles.begin(), systemStyles.end(), style);
    if (it == systemStyles.end())
        systemStyles.push_back(style);
}
void QQuickUniversalThemePrivate::removeSystemStyle(QPointer<QQuickUniversalStyle> style)
{
    QMutexLocker locker{&mutex};
    auto it = std::find(systemStyles.begin(), systemStyles.end(), style);
    if (it != systemStyles.end())
        systemStyles.erase(it);
}
void QQuickUniversalThemePrivate::updateSystemStyles()
{
    QMutexLocker locker{&mutex};
    for (auto it = systemStyles.begin(); it != systemStyles.end(); ) {
        if (it->isNull()) {
            it = systemStyles.erase(it);
        } else {
            (*it)->setTheme(QQuickUniversalStyle::System);
            ++it;
        }
    }
}

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

void QQuickUniversalTheme::registerSystemStyle(QQuickUniversalStyle *style)
{
    QQuickUniversalThemePrivate::addSystemStyle(QPointer{style});
}

void QQuickUniversalTheme::unregisterSystemStyle(QQuickUniversalStyle *style)
{
    QQuickUniversalThemePrivate::removeSystemStyle(QPointer{style});
}

void QQuickUniversalTheme::updateTheme()
{
    QQuickUniversalThemePrivate::updateSystemStyles();
}

QT_END_NAMESPACE
