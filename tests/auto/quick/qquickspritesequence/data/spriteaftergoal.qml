// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// QTBUG-40595
import QtQuick 2.0

Rectangle {
    width: 320
    height: 320

    SpriteSequence
    {
        anchors.centerIn: parent

        width: 300
        height: 300

        goalSprite: "foobar"

        sprites:
        [
            Sprite
            {
                name: "foobar"
                source: "squarefacesprite.png"
            }
        ]
    }
}

