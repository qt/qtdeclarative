// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Row {

    //![transition]
    Item {
        width: 400; height: 400

        Rectangle {
            id: rect
            width: 200; height: 100
            color: "red"

            states: State {
                name: "rotated"
                PropertyChanges { target: rect; rotation: 180; transformOrigin: Item.BottomRight }
            }

            transitions: Transition {
                RotationAnimation { duration: 1000; direction: RotationAnimation.Counterclockwise }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: rect.state = "rotated"
            }
        }
    }
    //![transition]

    Item {
        width: 300; height: 300

        Image { id: img; source: "pics/qt.png" }

        //![standalone]
        SequentialAnimation {
            PropertyAction { target: img; property: "opacity"; value: .5 }
            NumberAnimation { target: img; property: "width"; to: 300; duration: 1000 }
            PropertyAction { target: img; property: "opacity"; value: 1 }
        }
        //![standalone]
    }
}
