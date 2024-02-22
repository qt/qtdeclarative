// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    AnimatedSprite {
        objectName: "sprite"
        loops: 30
        source: "squarefacesprite.png"
        frameCount: 6
        frameDuration: 240
        width: 160
        height: 160
    }
}
