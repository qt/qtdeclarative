// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 320
    height: 480
    visible: true
    title: qsTr("Image Elements")

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
                { name: "BorderImage", description: "An image with scaled borders", source: "borderimage.qml" },
                { name: "Image", description: "A showcase of the options available to Image", source: "image.qml" },
                { name: "Shadows", description: "Rectangles with a drop-shadow effect", source: "shadows.qml" },
                { name: "AnimatedImage", description: "An image which plays animated formats", source: "animatedimage.qml" },
                { name: "AnimatedSprite", description: "A simple sprite-based animation", source: "animatedsprite.qml" },
                { name: "SpriteSequence", description: "A sprite-based animation with complex transitions", source: "spritesequence.qml" },
                { name: "FrameStepping", description: "A multi-frame non-animated image", source: "framestepping.qml" },
                { name: "MultiBorderImage", description: "A multi-frame image with scaled borders", source: "multiframeborderimage.qml" },
            ]
            onExampleClicked: (source) => stack.push(exampleLoader, { source })
        }

        Component {
            id: exampleLoader
            Loader {}
        }
    }
}
