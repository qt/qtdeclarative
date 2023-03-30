// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls

Item {
    width: 320
    height: 480
    Rectangle {
        anchors.fill: parent
        color: "white"
    }

//! [sprite]
    AnimatedSprite {
        id: sprite

        anchors.centerIn: parent
        source: "pics/speaker.png"
        frameCount: 60
        frameSync: true
        frameWidth: 170
        frameHeight: 170
        loops: 3
    }
//! [sprite]

    Label {
        text: qsTr("Left click to resume\nMiddle click to advance backward\nRight click to advance forward")
        visible: sprite.paused
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onClicked: (mouse) => {
            if (!sprite.running) {
                sprite.start()
            } else if (!sprite.paused) {
                sprite.pause()
            } else {
                if (mouse.button === Qt.LeftButton)
                    sprite.resume()
                else if (mouse.button === Qt.MiddleButton)
                    sprite.advance(-1)
                else if (mouse.button === Qt.RightButton)
                    sprite.advance(1)
            }
        }
    }

    Component.onCompleted: console.log(qsTr("Press Space to toggle visibility. Click with mouse to pause/resume."))
    focus: true
    Keys.onSpacePressed: sprite.visible = !sprite.visible
}
