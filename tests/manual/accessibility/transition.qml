// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: scene
    width: 300; height: 300

    Rectangle {
        id: rect
        x : 100
        y : 100
        width: 100; height: 100
        color: "red"

        MouseArea {
            id: mouseArea
            anchors.fill: parent
        }

        Text {
            text : "Transition"
        }

        states: State {
            name: "moved"; when: mouseArea.pressed
            PropertyChanges { target: rect; x: 150; y: 150 }
        }

        transitions: Transition {
            NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 1000 }
        }
    }
}
