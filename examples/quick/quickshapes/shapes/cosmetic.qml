// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    SequentialAnimation {
        running: true
        loops: Animation.Infinite

        NumberAnimation {
            id: anim
            targets: [shape1, shape2]
            property: "scale"
            from: 0.4
            to: 4
            duration: 2000
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            targets: [shape1, shape2]
            property: anim.property
            from: anim.to
            to: anim.from
            duration: anim.duration
            easing.type: anim.easing.type
        }
    }

    Shape {
        id: shape1
        preferredRendererType: root.requestedBackend
        anchors.centerIn: parent
        width: 50
        height: 50

        ShapePath {
            fillColor: "transparent"
            strokeColor: "teal"
            cosmeticStroke: false

            PathLine { relativeX: 50; relativeY: 50 }
        }
    }

    Shape {
        id: shape2
        preferredRendererType: root.requestedBackend
        anchors.centerIn: parent
        width: 50
        height: 50

        ShapePath {
            fillColor: "transparent"
            strokeColor: "teal"
            cosmeticStroke: true

            startX: 50
            PathLine { relativeX: -50; relativeY: 50 }
        }
    }
}
