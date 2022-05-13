// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    color: "white"
    width: 512
    height: 320

    AnimatedSprite {
        anchors.centerIn: parent
        objectName: "sprite"
        source: "image://test/largeAnimation.png"
        frameCount: 40
        loops: 3
        frameSync: true
        running: false
        width: 512
        height: 64
        frameWidth: 512
        frameHeight: 64

    }
}
