// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    width: 300; height: 300

    Rectangle {
        id: rect
        width: 150; height: 100; anchors.centerIn: parent
        color: "red"
        antialiasing: true

        states: State {
            name: "rotated"
            PropertyChanges { target: rect; rotation: 180 }
        }

        transitions: Transition {
            RotationAnimation { duration: 1000; direction: RotationAnimation.Counterclockwise }
        }
    }

    MouseArea { anchors.fill: parent; onClicked: rect.state = "rotated" }
}
//![0]

