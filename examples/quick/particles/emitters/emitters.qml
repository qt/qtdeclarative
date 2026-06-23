// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Emitters")

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
                { name: "Velocity from Motion", description: "Particle motion just by moving emitters", source: "velocityfrommotion.qml" },
                { name: "Burst and Pulse", description: "Emit imperatively", source: "burstandpulse.qml" },
                { name: "Custom Emitter", description: "Custom starting state", source: "customemitter.qml" },
                { name: "Emit Mask", description: "Emit arbitrary shapes", source: "emitmask.qml" },
                { name: "Maximum Emitted", description: "Put a limit on emissions", source: "maximumemitted.qml" },
                { name: "Shape and Direction", description: "Creates a portal effect", source: "shapeanddirection.qml" },
                { name: "TrailEmitter", description: "Emit from other particles", source: "trailemitter.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
