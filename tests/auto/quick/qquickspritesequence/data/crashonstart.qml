// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//QTBUG-24797
import QtQuick 2.0

Rectangle {
    width: 800
    height: 800

    SpriteSequence {
        id: mysprite
        sprites: [s1,s2]
        scale: 2
        height: 200
        width: 200
        anchors.centerIn: parent
    }

    Component.onCompleted: mysprite.jumpTo("running")
    Sprite {
        id: s1
        name: "standing"
        frameCount: 12
        frameDuration: 80
        source: "squarefacesprite.png"
    }

    Sprite {
        id: s2
        name: "running"
        frameCount: 6
        frameDuration: 80
        source: "squarefacesprite.png"
    }
}
