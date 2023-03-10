// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Shapes

Pane {
    width: 400
    height: 300

    Page {
        anchors.fill: parent
        anchors.margins: 40

        header: ToolBar {
            Label {
                text: "Material"
                anchors.centerIn: parent
            }
        }

        TextField {
            id: textField
            text: "TextField"
            anchors.centerIn: parent

            Component.onCompleted: forceActiveFocus()
        }
    }

    component Line: Shape {
        // Account for 1-pixel-wide lines.
        width: Math.max(1, endX - startX)
        height: Math.max(1, endY - startY)

        layer.enabled: true
        layer.samples: 4

        property alias startX: shapePath.startX
        property alias startY: shapePath.startY
        property alias endX: pathLine.x
        property alias endY: pathLine.y

        ShapePath {
            id: shapePath
            strokeWidth: 1
            strokeColor: "#444"

            PathLine {
                id: pathLine
            }
        }
    }

    Label {
        id: primaryLabel
        x: 40
        y: 3
        text: "Primary"
    }
    Line {
        id: primaryLine
        x: primaryLabel.x + primaryLabel.width / 2
        y: primaryLabel.y + primaryLabel.height
        startX: 0.5
        startY: 0
        endX: 0.5
        endY: 40
    }

    Label {
        id: foregroundLabel
        anchors.horizontalCenter: parent.horizontalCenter
        y: 3
        text: "Foreground"
    }
    Line {
        id: foregroundLine
        x: foregroundLabel.x + foregroundLabel.width / 2
        y: foregroundLabel.y + foregroundLabel.height
        // Lines are drawn at the center of the pixel.
        startX: 0.5
        startY: 0
        endX: 0.5
        endY: 34
    }

    Label {
        id: accentLabel
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.825
        text: "Accent"
    }
    Line {
        id: accentLine
        x: accentLabel.x + accentLabel.width / 2
        y: parent.height * 0.7
        startX: 0.5
        startY: 0
        endX: 0.5
        endY: 38
    }

    Label {
        id: backgroundLabel
        x: parent.width - width - 10
        y: parent.height - height - 10
        text: "Background"
    }
    Line {
        id: backgroundLine
        x: backgroundLabel.x + backgroundLabel.width / 2
        y: backgroundLabel.y - height
        startX: 0.5
        startY: 0
        endX: 0.5
        endY: 40
    }
}
