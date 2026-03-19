// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitstyleandthemebase_p.h"
#include "qqstylekitvariation_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype AbstractStyle
    \inqmlmodule Qt.labs.StyleKit
    \inherits AbstractStylableControls
    \brief Abstract base type with properties common to both \l Style and \l Theme.

    AbstractStyle contains properties, such and fonts and palettes, that are
    common to both a \l Style and a \l Theme.

    \labs
*/

QQStyleKitStyleAndThemeBase::QQStyleKitStyleAndThemeBase(QObject *parent)
    : QQStyleKitControls(parent)
{
}

/*!
    \qmlproperty StyleFont AbstractStyle::fonts

    Grouped property for configuring per-control \l [QML]{font}{fonts}.
    Fonts can be set for individual control types such as button,
    textField, or label.

    \qml
    Style {
        fonts {
            system.pixelSize: 14
            button.bold: true
            textField.family: "Monospace"
        }
    }
    \endqml
*/
QQStyleKitFont *QQStyleKitStyleAndThemeBase::fonts()
{
    return &m_fonts;
}

/*!
    \qmlproperty StylePalette AbstractStyle::palettes

    Grouped property for configuring per-control \l [QML]{Palette}{palettes}.
    Palettes can be set system-wide or for individual control types.

    \qml
    light: Theme {
        palettes {
            system {
                window: "gainsboro"
                windowText: "black"
            }
            button.buttonText: "black"
            textField.text: "#4e4e4e"
        }
    }
    \endqml
*/
QQStyleKitPalette *QQStyleKitStyleAndThemeBase::palettes()
{
    return &m_palettes;
}

QT_END_NAMESPACE

#include "moc_qqstylekitstyleandthemebase_p.cpp"


