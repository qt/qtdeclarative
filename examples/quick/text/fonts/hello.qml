// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: screen

    width: 320
    height: 480
    color: "black"

    Item {
        id: container
        x: screen.width / 2
        y: screen.height / 2

        Text {
            id: text
            anchors.centerIn: parent
            color: "white"
            text: qsTr("Hello world!")
            font.pixelSize: 32

//! [letterspacing]
            SequentialAnimation on font.letterSpacing {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 0
                    to: 50
                    easing.type: Easing.InQuad
                    duration: 3000
                }
                ScriptAction {
                    script: {
                        container.y = (screen.height / 4) + (Math.random() * screen.height / 2)
                        container.x = (screen.width / 4) + (Math.random() * screen.width / 2)
                    }
                }
            }
//! [letterspacing]

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation {
                    from: 1
                    to: 0
                    duration: 2600
                }
                PauseAnimation {
                    duration: 400
                }
            }
        }
    }
}
