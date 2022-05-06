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

#include "qquicktoolbutton_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolButton
    \inherits Button
//!     \instantiates QQuickToolButton
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-buttons
    \brief Button with a look suitable for a ToolBar.

    ToolButton is functionally similar to \l Button, but provides a look that
    is more suitable within a \l ToolBar.

    \image qtquickcontrols2-toolbar.png

    \snippet qtquickcontrols2-toolbar.qml 1

    ToolButton inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text}, display an \l {Icons in Qt Quick Controls}{icon},
    and react to \l {AbstractButton::clicked}{clicks} using the AbstractButton API.

    \sa ToolBar, {Customizing ToolButton}, {Button Controls}
*/

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickToolPrivate : public QQuickButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolButton)

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
