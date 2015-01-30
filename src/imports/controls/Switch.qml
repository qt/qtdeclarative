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

import QtQuick 2.4
import QtQuick.Controls 2.0

AbstractSwitch {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            (label ? label.implicitWidth : 0) +
                            (indicator ? indicator.implicitWidth : 0) +
                            (label && indicator ? style.spacing : 0) + padding.left + padding.right)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(label ? label.implicitHeight : 0,
                                      indicator ? indicator.implicitHeight : 0) + padding.top + padding.bottom)

    Accessible.name: text
    Accessible.checkable: true
    Accessible.checked: checked
    Accessible.pressed: pressed
    Accessible.role: Accessible.Button // TODO: Switch?

    padding { top: style.padding; left: style.padding; right: style.padding; bottom: style.padding }

    indicator: Rectangle {
        readonly property bool mirror: control.effectiveLayoutDirection == Qt.RightToLeft

        implicitWidth: 36
        implicitHeight: 20
        x: text ? (mirror ? parent.width - width - padding.right : padding.left) : (parent.width - width) / 2
        y: (parent.height - height) / 2

        radius: 10
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? style.focusColor : style.frameColor
        opacity: enabled ? 1.0 : style.disabledOpacity
        color: style.backgroundColor

        Rectangle {
            width: 12
            height: 12
            radius: 6

            color: Qt.tint(Qt.tint(control.checked ? style.accentColor : style.baseColor,
                                   control.checked && control.activeFocus ? style.focusColor : "transparent"),
                                   control.pressed ? style.pressColor : "transparent")
            border.width: control.checked || control.pressed ? 0 : 1
            border.color: style.frameColor

            x: Math.max(4, Math.min(parent.width - width - 4,
                                    control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2

            Behavior on x {
                enabled: !control.pressed
                SmoothedAnimation { velocity: 200 }
            }
        }
    }

    label: Text {
        readonly property bool mirror: control.effectiveLayoutDirection == Qt.RightToLeft

        x: mirror ? padding.left : (indicator.x + indicator.width + control.style.spacing)
        y: padding.top
        width: parent.width - indicator.width - control.style.spacing - padding.left - padding.right
        height: parent.height - padding.top - padding.bottom

        text: control.text
        color: control.style.textColor
        elide: Text.ElideRight
        visible: control.text
        opacity: enabled ? 1.0 : control.style.disabledOpacity
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
}
