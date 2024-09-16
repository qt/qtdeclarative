// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.4

AnimatedSprite {
    id: animatedSprite
    source: big ? "img100x100.png" : "img50x50.png"
    frameWidth: 100
    frameHeight: 100
    property bool big: true
    MouseArea {
        anchors.fill: parent
        onClicked: animatedSprite.big = !animatedSprite.big
    }
}
