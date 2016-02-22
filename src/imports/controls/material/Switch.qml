/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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
import Qt.labs.controls.material 1.0
import Qt.labs.templates 1.0 as T
import QtGraphicalEffects 1.0

T.Switch {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            (label ? label.implicitWidth : 0) +
                            (indicator ? indicator.implicitWidth : 0) +
                            (label && indicator ? spacing : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(label ? label.implicitHeight : 0,
                                      indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    baselineOffset: label ? label.y + label.baselineOffset : 0

    padding: 8
    spacing: 8

    //! [indicator]
    indicator: Item {
        x: text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 38
        implicitHeight: 32

        Ripple {
            x: handle.x + handle.width / 2 - width / 2
            y: handle.y + handle.height / 2 - height / 2
            width: handle.width
            height: width
            control: control
            colored: control.checked
            opacity: control.pressed || control.activeFocus ? 1 : 0
        }

        Rectangle {
            width: parent.width
            height: 14
            radius: height / 2
            y: parent.height / 2 - height / 2
            color: control.enabled ? (control.checked ? control.Material.switchCheckedTrackColor : control.Material.switchUncheckedTrackColor)
                                   : control.Material.switchDisabledTrackColor
        }

        Rectangle {
            id: handle
            x: Math.max(0, Math.min(parent.width - width, control.visualPosition * parent.width - (width / 2)))
            y: (parent.height - height) / 2
            width: 20
            height: 20
            radius: width / 2
            color: control.enabled ? (control.checked ? control.Material.switchCheckedHandleColor : control.Material.switchUncheckedHandleColor)
                                   : control.Material.switchDisabledHandleColor

            Behavior on x {
                enabled: !control.pressed
                SmoothedAnimation {
                    duration: 300
                }
            }

            layer.enabled: true
            layer.effect: DropShadow {
                verticalOffset: 1
                color: control.Material.dropShadowColor
                spread: 0.3
            }
        }
    }
    //! [indicator]

    //! [label]
    label: Text {
        x: control.mirrored ? control.leftPadding : (indicator.x + indicator.width + control.spacing)
        y: control.topPadding
        width: control.availableWidth - indicator.width - control.spacing
        height: control.availableHeight

        text: control.text
        font: control.font
        color: control.enabled ? control.Material.primaryTextColor : control.Material.hintTextColor
        elide: Text.ElideRight
        visible: control.text
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
    }
    //! [label]
}
