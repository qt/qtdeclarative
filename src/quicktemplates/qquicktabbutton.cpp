// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktabbutton_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabButton
    \inherits AbstractButton
//!     \nativetype QQuickTabButton
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-navigation
    \brief Button with a look suitable for a TabBar.

    \image qtquickcontrols-tabbutton.png

    TabButton is used in conjunction with a \l TabBar.

    \snippet qtquickcontrols-tabbutton.qml 1

    TabButton inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text}, and react to \l {AbstractButton::clicked}{clicks}
    using the AbstractButton API.

    \sa TabBar, {Customizing TabButton}, {Button Controls}, {Navigation Controls}
*/

class Q_QUICKTEMPLATES2_EXPORT QQuickTabButtonPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickTabButton)

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::TabBar); }
};

QQuickTabButton::QQuickTabButton(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickTabButtonPrivate), parent)
{
    setCheckable(true);
    setAutoExclusive(true);
}

QFont QQuickTabButton::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::TabBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickTabButton::accessibleRole() const
{
    return QAccessible::PageTab;
}
#endif

QT_END_NAMESPACE

#include "moc_qquicktabbutton_p.cpp"
