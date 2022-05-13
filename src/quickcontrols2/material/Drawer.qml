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

    Material.elevation: !interactive && !dim ? 0 : 16

    background: Rectangle {
        color: control.Material.dialogColor

        Rectangle {
            readonly property bool horizontal: control.edge === Qt.LeftEdge || control.edge === Qt.RightEdge
            width: horizontal ? 1 : parent.width
            height: horizontal ? parent.height : 1
            color: control.Material.dividerColor
            x: control.edge === Qt.LeftEdge ? parent.width - 1 : 0
            y: control.edge === Qt.TopEdge ? parent.height - 1 : 0
            visible: !control.dim && control.Material.elevation === 0
        }

        layer.enabled: control.position > 0
        layer.effect: ElevationEffect {
            elevation: control.Material.elevation
            fullHeight: true
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
