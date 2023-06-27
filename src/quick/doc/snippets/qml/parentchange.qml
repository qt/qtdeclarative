// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 200; height: 100

    Rectangle {
        id: redRect
        width: 100; height: 100
        color: "red"
    }

    Rectangle {
        id: blueRect
        x: redRect.width
        width: 50; height: 50
        color: "blue"

        states: State {
            name: "reparented"
            ParentChange { target: blueRect; parent: redRect; x: 10; y: 10 }
        }

        MouseArea { anchors.fill: parent; onClicked: blueRect.state = "reparented" }
    }
}
//![0]

