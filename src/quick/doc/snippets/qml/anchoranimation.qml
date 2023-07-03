// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {
    id: container
    width: 200; height: 200

    Rectangle {
        id: myRect
        width: 100; height: 100
        color: "red"
    }

    states: State {
        name: "reanchored"
        AnchorChanges { target: myRect; anchors.right: container.right }
    }

    transitions: Transition {
        // smoothly reanchor myRect and move into new position
        AnchorAnimation { duration: 1000 }
    }

    Component.onCompleted: container.state = "reanchored"
}
//![0]
