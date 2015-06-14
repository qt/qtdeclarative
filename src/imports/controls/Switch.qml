/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

import QtQuick 2.6
import QtQuick.Controls 2.0

AbstractSwitch {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            (label ? label.implicitWidth : 0) +
                            (indicator ? indicator.implicitWidth : 0) +
                            (label && indicator ? Theme.spacing : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(label ? label.implicitHeight : 0,
                                      indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)

    Accessible.name: text
    Accessible.checkable: true
    Accessible.checked: checked
    Accessible.pressed: pressed
    Accessible.role: Accessible.Button // TODO: Switch?

    padding: Theme.padding

    //! [indicator]
    indicator: Rectangle {
        implicitWidth: 36
        implicitHeight: 20
        x: text ? (control.mirrored ? parent.width - width - control.rightPadding : control.leftPadding) : (parent.width - width) / 2
        y: (parent.height - height) / 2

        radius: 10
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? control.Theme.focusColor : control.Theme.frameColor
        color: control.Theme.backgroundColor

        Rectangle {
            width: 12
            height: 12
            radius: 6

            color: Qt.tint(control.checked && !control.enabled ? control.Theme.disabledColor :
                           control.checked && control.activeFocus ? control.Theme.focusColor :
                           control.checked ? control.Theme.accentColor : control.Theme.baseColor,
                           control.pressed ? control.Theme.pressColor : "transparent")
            border.width: control.checked || control.pressed ? 0 : 1
            border.color: control.Theme.frameColor

            x: Math.max(4, Math.min(parent.width - width - 4,
                                    control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2

            Behavior on x {
                enabled: !control.pressed
                SmoothedAnimation { velocity: 200 }
            }
        }
    }
    //! [indicator]

    //! [label]
    label: Text {
        x: control.mirrored ? control.leftPadding : (indicator.x + indicator.width + control.Theme.spacing)
        y: control.topPadding
        width: control.contentWidth - indicator.width - control.Theme.spacing
        height: control.contentHeight

        text: control.text
        color: control.enabled ? control.Theme.textColor : control.Theme.disabledColor
        elide: Text.ElideRight
        visible: control.text
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
    //! [label]
}
