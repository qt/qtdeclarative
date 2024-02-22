// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    color: "white"
    width: 320
    height: 320

    SpriteSequence {
        objectName: "sprite"
        sprites: Sprite {
            name: "black"
            source: "huge.png"
            frameCount: 4096
            frameSync: true
        }
        width: 64
        height: 64
    }
}
