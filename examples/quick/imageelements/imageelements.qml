// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Shared

Item {
    height: 480
    width: 320
    Shared.LauncherList {
        id: ll

        anchors.fill: parent
        Component.onCompleted: {
            addExample(qsTr("BorderImage"), qsTr("An image with scaled borders"),  Qt.resolvedUrl("borderimage.qml"))
            addExample(qsTr("Image"), qsTr("A showcase of the options available to Image"), Qt.resolvedUrl("image.qml"))
            addExample(qsTr("Shadows"), qsTr("Rectangles with a drop-shadow effect"), Qt.resolvedUrl("shadows.qml"))
            addExample(qsTr("AnimatedImage"), qsTr("An image which plays animated formats"), Qt.resolvedUrl("animatedimage.qml"))
            addExample(qsTr("AnimatedSprite"), qsTr("A simple sprite-based animation"), Qt.resolvedUrl("animatedsprite.qml"))
            addExample(qsTr("SpriteSequence"), qsTr("A sprite-based animation with complex transitions"), Qt.resolvedUrl("spritesequence.qml"))
            addExample(qsTr("FrameStepping"), qsTr("A multi-frame non-animated image"), Qt.resolvedUrl("framestepping.qml"))
            addExample(qsTr("MultiBorderImage"), qsTr("A multi-frame image with scaled borders"), Qt.resolvedUrl("multiframeborderimage.qml"))
        }
    }
}
