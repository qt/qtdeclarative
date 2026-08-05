// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion
import QtQuick.Controls.Fusion.impl

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 2
    horizontalPadding: 4
    spacing: 6

    icon.width: 16
    icon.height: 16

    z: checked

    readonly property bool __tabBarVertical: control.TabBar.tabBar?.vertical ?? false

    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.buttonText
    }


    background: Rectangle {
        x: control.__tabBarVertical ? (control.checked || control.TabBar.position !== T.TabBar.Header ? 0 : 2) : 0
        y: control.__tabBarVertical ? 0 : (control.checked || control.TabBar.position !== T.TabBar.Header ? 0 : 2)
        implicitWidth: control.__tabBarVertical ? 21 : 0
        implicitHeight: control.__tabBarVertical ? 0 : 21
        width: control.__tabBarVertical ? control.width - (control.checked ? 0 : 2) : control.width
        height: control.__tabBarVertical ? control.height : control.height - (control.checked ? 0 : 2)

        border.color: Qt.lighter(Fusion.outline(control.palette), 1.1)

        gradient: Gradient {
            orientation: control.__tabBarVertical ? Gradient.Horizontal : Gradient.Vertical
            GradientStop {
                position: 0
                color: control.checked ? Qt.lighter(Fusion.tabFrameColor(control.palette), 1.04)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.08)
            }
            GradientStop {
                position: control.checked ? 0 : 0.85
                color: control.checked ? Qt.lighter(Fusion.tabFrameColor(control.palette), 1.04)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.08)
            }
            GradientStop {
                position: 1
                color: control.checked ? Fusion.tabFrameColor(control.palette)
                                       : Qt.darker(Fusion.tabFrameColor(control.palette), 1.16)
            }
        }
    }
}
