// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcustomtheme_p.h"
#include "qqstylekittheme_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype CustomTheme
    \inqmlmodule Qt.labs.StyleKit
    \brief Defines a named custom theme.

    CustomTheme lets you add additional themes beyond
    \l {Style::light}{light} and \l {Style::dark}{dark}.
    While the light and dark themes are applied \l {Style::themeName}{automatically} based on the
    operating system's color scheme, a CustomTheme must be activated
    explicitly by the application. Apart from this difference, all themes work the same way.

    You can define any number of custom themes inside a \l Style.

    \snippet CustomThemeSnippets.qml custom themes

    To activate a CustomTheme, set \l Style::themeName to its name
    from the application:

    \snippet CustomThemeSnippets.qml change theme

    You can also set a CustomTheme as the default theme at start-up:

    \snippet CustomThemeSnippets.qml custom theme at start-up

    The custom themes defined in a Style can be queried at runtime
    from \l {Style::customThemeNames} or \l {Style::themeNames}.

    \labs

    \sa Theme, Style::themeName, Style::customThemeNames
*/

/*!
    \qmlproperty string CustomTheme::name

    The name of this theme. This is the value you assign to
    \l {Style::themeName} to activate it.
*/

/*!
    \qmlproperty Component CustomTheme::theme

    The \l Theme component that defines the theme. It will
    only be instantiated when the theme is activated.

    Properties not set in the theme fall back to those defined in the \l Style.
*/

QQStyleKitCustomTheme::QQStyleKitCustomTheme(QObject *parent)
    : QObject(parent)
{
}

QString QQStyleKitCustomTheme::name() const
{
    return m_name;
}

void QQStyleKitCustomTheme::setName(const QString &newName)
{
    if (m_name == newName)
        return;
    m_name = newName;
    emit nameChanged();
}

QQmlComponent *QQStyleKitCustomTheme::theme() const
{
    return m_theme;
}

void QQStyleKitCustomTheme::setTheme(QQmlComponent *newTheme)
{
    if (m_theme == newTheme)
        return;
    m_theme = newTheme;
    emit themeChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitcustomtheme_p.cpp"
