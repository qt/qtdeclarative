// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    topPadding: !dim && edge === Qt.BottomEdge && Material.elevation === 0
    leftPadding: !dim && edge === Qt.RightEdge && Material.elevation === 0
    rightPadding: !dim && edge === Qt.LeftEdge && Material.elevation === 0
    bottomPadding: !dim && edge === Qt.TopEdge && Material.elevation === 0

    enter: Transition { SmoothedAnimation { velocity: 5 } }
    exit: Transition { SmoothedAnimation { velocity: 5 } }

    // https://m3.material.io/components/navigation-drawer/specs#e616dc8f-d61a-4d56-a311-50c68ecda744
    Material.elevation: !interactive && !dim ? 0 : 1
    Material.roundedScale: Material.LargeScale

    background: PaddedRectangle {
        // https://m3.material.io/components/navigation-drawer/specs#ce8bfbcf-3dec-45d2-9d8b-5e10af1cf87d
        implicitWidth: 360
        color: control.Material.dialogColor
        // FullScale doesn't make sense for Drawer.
        radius: control.Material.roundedScale
        // Only the top and bottom right corners should be rounded.
        leftPadding: -radius
        clip: true

        layer.enabled: control.position > 0 && control.Material.elevation > 0
        layer.effect: RoundedElevationEffect {
            elevation: control.Material.elevation
            roundedScale: control.background.radius
        }
    }

    T.Overlay.modal: Rectangle {
        color: control.Material.backgroundDimColor
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    T.Overlay.modeless: Rectangle {
        color: control.Material.backgroundDimColor
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
