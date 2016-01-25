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

#include "qquicktoolbar_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolBar
    \inherits Frame
    \instantiates QQuickToolBar
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-containers
    \brief A tool bar control.

    ToolBar is a container of application-wide and context sensitive
    actions and controls, such as navigation buttons and search fields.
    ToolBar is commonly used as a \l {ApplicationWindow::header}{header}
    or a \l {ApplicationWindow::footer}{footer} of an \l ApplicationWindow.

    ToolBar does not provide a layout of its own, but requires you to
    position its contents, for instance by creating a \l RowLayout. If only
    a single item is used within the ToolBar, it will resize to fit the
    implicit size of its contained item. This makes it particularly suitable
    for use together with layouts.

    \image qtlabscontrols-toolbar.png

    \code
    ApplicationWindow {
        visible:true

        header: ToolBar {
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    text: qsTr("\u25C0 %1").arg(Qt.application.name)
                    enabled: stack.depth > 1
                    onClicked: stack.pop()
                }
                Item { Layout.fillWidth: true }
                Switch {
                    checked: true
                    text: qsTr("Notifications")
                }
            }
        }

        StackView {
            id: stack
            anchors.fill: parent
        }
    }
    \endcode

    \labs

    \sa ApplicationWindow, ToolButton, {Customizing ToolBar}, {Container Controls}
*/

QQuickToolBar::QQuickToolBar(QQuickItem *parent) :
    QQuickFrame(parent)
{
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickToolBar::accessibleRole() const
{
    return QAccessible::ToolBar;
}
#endif

QT_END_NAMESPACE
