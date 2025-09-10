// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmenuseparator_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuSeparator
    \inherits Control
//!     \nativetype QQuickMenuSeparator
    \inqmlmodule QtQuick.Controls
    \since 5.8
    \ingroup qtquickcontrols-separators
    \brief Separates a group of items in a menu from adjacent items.

    MenuSeparator is used to visually distinguish between groups of items in a
    menu by separating them with a line.

    \image qtquickcontrols-menuseparator.png

    \quotefromfile qtquickcontrols-menuseparator-custom.qml
    \skipto import QtQuick
    \printuntil import QtQuick.Controls
    \skipto Menu
    \printto contentItem.parent: window
    \skipline contentItem.parent: window
    \printuntil text: qsTr("Exit")
    \printuntil }
    \printuntil }

    \sa {Customizing Menu}, Menu, {Separator Controls}
*/

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuSeparatorPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuSeparator)

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::Menu); }
};

QQuickMenuSeparator::QQuickMenuSeparator(QQuickItem *parent)
    : QQuickControl(*(new QQuickMenuSeparatorPrivate), parent)
{
}

QFont QQuickMenuSeparator::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::Menu);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenuSeparator::accessibleRole() const
{
    return QAccessible::Separator;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenuseparator_p.cpp"
