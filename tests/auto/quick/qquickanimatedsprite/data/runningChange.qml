// Copyright (C) 2016 Tasuku Suzuki <stasuku@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    AnimatedSprite {
        objectName: "sprite"
        source: "squarefacesprite.png"
        frameCount: 6
        loops: 3
        frameSync: true
        running: false
        width: 160
        height: 160
    }
}
