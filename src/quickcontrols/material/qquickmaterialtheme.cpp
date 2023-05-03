// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialtheme_p.h"
#include "qquickmaterialstyle_p.h"

#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtGui/qfont.h>
#include <QtGui/qfontinfo.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

void QQuickMaterialTheme::initialize(QQuickTheme *theme)
{
    QFont systemFont;
    QFont buttonFont;
    QFont toolTipFont;
    QFont itemViewFont;
    QFont listViewFont;
    QFont menuItemFont;
    QFont editorFont;

    QFont font;
    font.setFamilies(QStringList{QLatin1String("Roboto")});
    QString family = QFontInfo(font).family();

    if (family != QLatin1String("Roboto")) {
        font.setFamilies(QStringList{QLatin1String("Noto")});
        family = QFontInfo(font).family();
    }

    if (family == QLatin1String("Roboto") || family == QLatin1String("Noto")) {
        const QStringList families{family};
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

QT_END_NAMESPACE
