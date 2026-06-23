// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Canvas")

    header: ToolBar {
        visible: stack.depth > 1
        height: visible ? implicitHeight : 0
        ToolButton {
            icon.source: "images/back.png"
            Accessible.name: qsTr("Back")
            onClicked: stack.pop()
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        focus: true
        initialItem: LauncherList {
            model: [
                { name: "Red heart", description: "Draws a red heart with bezier curves", source: "bezierCurve/bezierCurve.qml" },
                { name: "Talk bubble", description: "Draws a talk bubble with quadratic curves", source: "quadraticCurveTo/quadraticCurveTo.qml" },
                { name: "Squircle", description: "Draws a smooth squircle with simple lines", source: "squircle/squircle.qml" },
                { name: "Rounded rectangle", description: "Draws a rounded rectangle with lines and arcs", source: "roundedrect/roundedrect.qml" },
                { name: "Smile face", description: "Draws a smile face with complex paths", source: "smile/smile.qml" },
                { name: "Clip", description: "Shows the canvas clip feature", source: "clip/clip.qml" },
                { name: "Tiger", description: "Draw a tiger with a collection of SVG paths", source: "tiger/tiger.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
