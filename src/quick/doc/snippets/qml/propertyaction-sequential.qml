// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

Row {

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

            //! [sequential]
            transitions: Transition {
                SequentialAnimation {
                    PropertyAction { target: rect; property: "transformOrigin" }
                    RotationAnimation { duration: 1000; direction: RotationAnimation.Counterclockwise }
                }
            }
            //! [sequential]

            MouseArea {
                anchors.fill: parent
                onClicked: rect.state = "rotated"
            }
        }
    }
}
