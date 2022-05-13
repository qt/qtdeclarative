// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    SpriteSequence {
        objectName: "sprite"
        sprites: [Sprite {
            name: "firstState"
            source: "squarefacesprite.png"
            frameCount: 3
            frameSync: true
            to: {"secondState":1}
        }, Sprite {
            name: "secondState"
            source: "squarefacesprite.png"
            frameCount: 6
            frameSync: true
        } ]
        width: 160
        height: 160
    }
}
