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
import Qt.labs.templates 1.0 as T
import Qt.labs.controls.material 1.0

T.CheckBox {
    id: control

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            (label ? label.implicitWidth : 0) +
                            (indicator ? indicator.implicitWidth : 0) +
                            (label && indicator ? spacing : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0,
                             Math.max(label ? label.implicitHeight : 0,
                                      indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)

    padding: 6
    spacing: 6

    //! [indicator]
    indicator: Rectangle {
        id: indicatorItem
        x: text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 20
        implicitHeight: 20
        border.color: control.Material.secondaryTextColor
        radius: 2

        Component.onCompleted: {
            color = control.checked || control.pressed ? control.Material.accentColor : "transparent";
            border.width = control.checked || control.pressed ? 0 : 2;
        }

        Ripple {
            width: parent.width
            height: width
            control: control
            colored: control.checked
            opacity: control.pressed ? 1 : 0
        }

        // TODO: This needs to be transparent
        Image {
            id: checkImage
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            source: "qrc:/images/check.png"
            fillMode: Image.PreserveAspectFit

            Component.onCompleted: {
                width = control.checked || control.pressed ? 16 : 0;
                height = control.checked || control.pressed ? 16 : 0;
            }
        }

        Connections {
            target: control
            onCheckedChanged: {
                pressScaleAnimation.start();
                if (control.checked)
                    checkedAnimation.start();
                else
                    uncheckedAnimation.start();
            }
        }

        SequentialAnimation {
            id: pressScaleAnimation

            NumberAnimation {
                target: indicatorItem
                property: "scale"
                // Go down 2 pixels in size.
                to: 1 - 2 / indicatorItem.width
                duration: 120
            }
            NumberAnimation {
                target: indicatorItem
                property: "scale"
                to: 1
                duration: 120
            }
        }

        SequentialAnimation {
            id: checkedAnimation

            PropertyAction {
                target: indicatorItem.border
                property: "color"
                value: control.Material.accentColor
            }
            NumberAnimation {
                target: indicatorItem.border
                property: "width"
                from: 2
                to: indicatorItem.width / 2
                easing.type: Easing.OutCubic
                duration: 100
            }
            NumberAnimation {
                target: checkImage
                properties: "width, height"
                from: 0
                to: 16
                duration: 100
            }
        }

        SequentialAnimation {
            id: uncheckedAnimation

            PropertyAction {
                target: indicatorItem.border
                property: "color"
                value: control.Material.secondaryTextColor
            }
            NumberAnimation {
                target: checkImage
                properties: "width, height"
                from: 16
                to: 0
                duration: 100
            }
            NumberAnimation {
                target: indicatorItem.border
                property: "width"
                from: indicatorItem.width / 2
                to: 2
                easing.type: Easing.OutCubic
                duration: 100
            }
            PropertyAction {
                target: indicatorItem
                property: "color"
                value: "transparent"
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
