/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include "moc_qquicktoolbutton_p.cpp"
