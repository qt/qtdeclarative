/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
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

#include "qquickmenuseparator_p.h"
#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuSeparator
    \inherits Control
//!     \instantiates QQuickMenuSeparator
    \inqmlmodule QtQuick.Controls
    \since 5.8
    \ingroup qtquickcontrols2-separators
    \brief Separates a group of items in a menu from adjacent items.

    MenuSeparator is used to visually distinguish between groups of items in a
    menu by separating them with a line.

    \image qtquickcontrols2-menuseparator.png

    \quotefromfile qtquickcontrols2-menuseparator-custom.qml
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

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickMenuSeparatorPrivate : public QQuickControlPrivate
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
