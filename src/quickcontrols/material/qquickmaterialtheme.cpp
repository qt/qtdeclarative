// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialtheme_p.h"
#include "qquickmaterialstyle_p.h"

#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtGui/qfont.h>
#include <QtGui/qfontdatabase.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtCore/qmutex.h>
QT_BEGIN_NAMESPACE

struct QQuickMaterialThemePrivate
{
    static inline void addSystemStyle(QPointer<QQuickMaterialStyle> style);
    static inline void removeSystemStyle(QPointer<QQuickMaterialStyle> style);
    static inline void updateSystemStyles();

    static inline std::vector<QPointer<QQuickMaterialStyle>> systemStyles = {};
    static inline QMutex mutex;
};

void QQuickMaterialThemePrivate::addSystemStyle(QPointer<QQuickMaterialStyle> style)
{
    QMutexLocker locker{&mutex};
    auto it = std::find(systemStyles.begin(), systemStyles.end(), style);
    if (it == systemStyles.end())
        systemStyles.push_back(style);
}

void QQuickMaterialThemePrivate::removeSystemStyle(QPointer<QQuickMaterialStyle> style)
{
    QMutexLocker locker{&mutex};
    auto it = std::find(systemStyles.begin(), systemStyles.end(), style);
    if (it != systemStyles.end())
        systemStyles.erase(it);
}

void QQuickMaterialThemePrivate::updateSystemStyles()
{
    QMutexLocker locker{&mutex};
    for (auto it = systemStyles.begin(); it != systemStyles.end(); ) {
        if (it->isNull()) {
            it = systemStyles.erase(it);
        } else {
            (*it)->setTheme(QQuickMaterialStyle::System);
            ++it;
        }
    }
}

void QQuickMaterialTheme::initialize(QQuickTheme *theme)
{
    QFont systemFont;
    QFont buttonFont;
    QFont toolTipFont;
    QFont itemViewFont;
    QFont listViewFont;
    QFont menuItemFont;
    QFont editorFont;

    auto defaultFontFamily = QLatin1String("Roboto");
    if (!QFontDatabase::hasFamily(defaultFontFamily)) {
        defaultFontFamily = QLatin1String("Noto"); // fallback
        if (!QFontDatabase::hasFamily(defaultFontFamily))
            defaultFontFamily = {};
    }

    if (!defaultFontFamily.isEmpty()) {
        const QStringList families{defaultFontFamily};
        systemFont.setFamilies(families);
        buttonFont.setFamilies(families);
        toolTipFont.setFamilies(families);
        itemViewFont.setFamilies(families);
        listViewFont.setFamilies(families);
        menuItemFont.setFamilies(families);
        editorFont.setFamilies(families);
    }

    const bool dense = QQuickMaterialStyle::variant() == QQuickMaterialStyle::Dense;
    systemFont.setPixelSize(dense ? 13 : 14);
    theme->setFont(QQuickTheme::System, systemFont);

    // https://material.io/guidelines/components/buttons.html#buttons-style
    buttonFont.setPixelSize(dense ? 13 : 14);
    buttonFont.setWeight(QFont::Medium);
    theme->setFont(QQuickTheme::Button, buttonFont);
    theme->setFont(QQuickTheme::TabBar, buttonFont);
    theme->setFont(QQuickTheme::ToolBar, buttonFont);

    // https://material.io/guidelines/components/tooltips.html
    toolTipFont.setPixelSize(dense ? 10 : 14);
    toolTipFont.setWeight(QFont::Medium);
    theme->setFont(QQuickTheme::ToolTip, toolTipFont);

    itemViewFont.setPixelSize(dense ? 13 : 14);
    itemViewFont.setWeight(QFont::Medium);
    theme->setFont(QQuickTheme::ItemView, itemViewFont);

    // https://material.io/guidelines/components/lists.html#lists-specs
    listViewFont.setPixelSize(dense ? 13 : 16);
    theme->setFont(QQuickTheme::ListView, listViewFont);

    menuItemFont.setPixelSize(dense ? 13 : 16);
    theme->setFont(QQuickTheme::Menu, menuItemFont);
    theme->setFont(QQuickTheme::MenuBar, menuItemFont);
    theme->setFont(QQuickTheme::ComboBox, menuItemFont);

    editorFont.setPixelSize(dense ? 13 : 16);
    theme->setFont(QQuickTheme::TextArea, editorFont);
    theme->setFont(QQuickTheme::TextField, editorFont);
    theme->setFont(QQuickTheme::SpinBox, editorFont);
}

void QQuickMaterialTheme::registerSystemStyle(QQuickMaterialStyle *style)
{
    QQuickMaterialThemePrivate::addSystemStyle(QPointer{style});
}

void QQuickMaterialTheme::unregisterSystemStyle(QQuickMaterialStyle *style)
{
    QQuickMaterialThemePrivate::removeSystemStyle(QPointer{style});
}

void QQuickMaterialTheme::updateTheme()
{
    QQuickMaterialThemePrivate::updateSystemStyles();
}

QT_END_NAMESPACE
