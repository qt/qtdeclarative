// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktoolbutton_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolButton
    \inherits Button
//!     \nativetype QQuickToolButton
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-buttons
    \brief Button with a look suitable for a ToolBar.

    ToolButton is functionally similar to \l Button, but provides a look that
    is more suitable within a \l ToolBar.

    \image qtquickcontrols-toolbar.png

    \snippet qtquickcontrols-toolbar.qml 1

    ToolButton inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text}, display an \l {Icons in Qt Quick Controls}{icon},
    and react to \l {AbstractButton::clicked}{clicks} using the AbstractButton API.

    \sa ToolBar, {Customizing ToolButton}, {Button Controls}
*/

class Q_QUICKTEMPLATES2_EXPORT QQuickToolPrivate : public QQuickButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolButton)

    QQuickToolPrivate() { setSizePolicy(QLayoutPolicy::Fixed, QLayoutPolicy::Fixed); }

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ToolBar); }
};

QQuickToolButton::QQuickToolButton(QQuickItem *parent)
    : QQuickButton(*(new QQuickToolPrivate), parent)
{
}

QFont QQuickToolButton::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ToolBar);
}

QT_END_NAMESPACE

#include "moc_qquicktoolbutton_p.cpp"
