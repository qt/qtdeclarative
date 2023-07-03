// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: window
    width: 120; height: 120
    color: "black"

    Rectangle { id: myRect; width: 50; height: 50; color: "red" }

    states: State {
        name: "reanchored"

        AnchorChanges {
            target: myRect
            anchors.top: window.top
            anchors.bottom: window.bottom
        }
        PropertyChanges {
            target: myRect
            anchors.topMargin: 10
            anchors.bottomMargin: 10
        }
    }

    MouseArea { anchors.fill: parent; onClicked: window.state = "reanchored" }
}
//![0]

