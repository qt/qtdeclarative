// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
Item {
    id: root
    default  property Item child
    property real amount: 2
    children: [child, hoverArea]
    width: child.width
    height: child.height
    property real bounceScale: hoverArea.containsMouse ? amount : 1
    property alias hoverEnabled: hoverArea.enabled
    Binding {
        target: child
        property: "scale"
        value: root.bounceScale
    }

    Behavior on bounceScale {
        NumberAnimation {
            duration: 300
            easing.type: Easing.InOutQuad
        }
    }
    z: hoverArea.containsMouse ? 1 : 0

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
    }
}
