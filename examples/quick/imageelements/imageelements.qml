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
            addExample("BorderImage", "An image with scaled borders",  Qt.resolvedUrl("borderimage.qml"));
            addExample("Image", "A showcase of the options available to Image", Qt.resolvedUrl("image.qml"));
            addExample("Shadows", "Rectangles with a drop-shadow effect", Qt.resolvedUrl("shadows.qml"));
            addExample("AnimatedImage", "An image which plays animated formats", Qt.resolvedUrl("animatedimage.qml"));
            addExample("AnimatedSprite", "A simple sprite-based animation", Qt.resolvedUrl("animatedsprite.qml"));
            addExample("SpriteSequence", "A sprite-based animation with complex transitions", Qt.resolvedUrl("spritesequence.qml"));
            addExample("FrameStepping", "A multi-frame non-animated image", Qt.resolvedUrl("framestepping.qml"));
            addExample("MultiBorderImage", "A multi-frame image with scaled borders", Qt.resolvedUrl("multiframeborderimage.qml"));
        }
    }
}
