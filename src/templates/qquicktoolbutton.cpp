/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicktoolbutton_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolButton
    \inherits Button
    \instantiates QQuickToolButton
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-buttons
    \brief A tool button control.

    ToolButton is functionally similar to \l Button, but provides a look that
    is more suitable within a \l ToolBar.

    ### TODO: screenshot

    \code
    ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: qsTr("< %1").arg(Qt.application.name)
                enabled: stack.depth > 1
                onClicked: stack.pop()
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                text: qsTr("< %1").arg(Qt.application.name)
                enabled: stack.depth > 1
                onClicked: stack.pop()
            }
        }
    }
    \endcode

    \labs

    \sa ToolBar, {Customizing ToolButton}, {Button Controls}
*/

QQuickToolButton::QQuickToolButton(QQuickItem *parent) :
    QQuickButton(parent)
{
}

QFont QQuickToolButton::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::ToolButtonFont);
}

QT_END_NAMESPACE
