// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    id: root
    width: 1024
    height: 768

    readonly property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Qt.tint(root.col, "#20FFFFFF")
        }
        GradientStop {
            position: 0.1
            color: Qt.tint(root.col, "#20AAAAAA")
        }
        GradientStop {
            position: 0.9
            color: Qt.tint(root.col, "#20666666")
        }
        GradientStop {
            position: 1.0
            color: Qt.tint(root.col, "#20000000")
        }
    }

    Rectangle {
        id: scissorRect
        width: 200
        height: 200
        x: 150
        readonly property real centerY: parent.height / 2 - height / 2
        property real dy: 0
        y: centerY + dy
        clip: true

        Loader {
            id: loader1
            width: parent.width
            height: parent.height
            y: 25 - scissorRect.dy
            source: "tiger.qml"
            asynchronous: true
            visible: status === Loader.Ready
        }

        SequentialAnimation {
            loops: Animation.Infinite
            running: loader1.status === Loader.Ready && (loader1.item as Shape)?.status === Shape.Ready
            NumberAnimation {
                target: scissorRect
                property: "dy"
                from: 0
                to: -scissorRect.centerY
                duration: 2000
            }
            NumberAnimation {
                target: scissorRect
                property: "dy"
                from: -scissorRect.centerY
                to: scissorRect.centerY
                duration: 4000
            }
            NumberAnimation {
                target: scissorRect
                property: "dy"
                from: scissorRect.centerY
                to: 0
                duration: 2000
            }
        }
    }

    // With a more complex transformation (like rotation), stenciling is used
    // instead of scissoring, this is more expensive. It may also trigger a
    // slower code path for Shapes, depending on the path rendering backend
    // in use, and may affect rendering quality as well.
    Rectangle {
        id: stencilRect
        width: 300
        height: 200
        anchors {
            right: parent.right
            rightMargin: 100
            verticalCenter: parent.verticalCenter
        }
        clip: true // NB! still clips to bounding rect (not shape)

        Loader {
            id: loader2
            width: parent.width
            height: parent.height
            source: "tiger.qml"
            asynchronous: true
            visible: status === Loader.Ready
        }

        NumberAnimation on rotation {
            from: 0
            to: 360
            duration: 5000
            loops: Animation.Infinite
        }
    }
}
