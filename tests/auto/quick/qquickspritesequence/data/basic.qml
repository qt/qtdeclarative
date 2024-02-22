// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    SpriteSequence {
        objectName: "sprite"
        sprites: Sprite {
            name: "happy"
            source: "squarefacesprite.png"
            frameCount: 6
            frameDuration: 120
        }
        width: 160
        height: 160
    }
}
