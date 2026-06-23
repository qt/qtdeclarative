// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Image Particle")

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
                { name: "All at once", description: "Uses all ImageParticle features", source: "allatonce.qml" },
                { name: "Colored", description: "Colorized image particles", source: "colored.qml" },
                { name: "Color Table", description: "Color-over-life rainbow particles", source: "colortable.qml" },
                { name: "Deformation", description: "Deformed particles", source: "deformation.qml" },
                { name: "Rotation", description: "Rotated particles", source: "rotation.qml" },
                { name: "Sharing", description: "Multiple ImageParticles on the same particles", source: "sharing.qml" },
                { name: "Sprites", description: "Particles rendered with sprites", source: "sprites.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
