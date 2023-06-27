// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: myRect
    width: 100; height: 100
    color: "black"

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: myRect.state == 'clicked' ? myRect.state = "" : myRect.state = 'clicked';
    }

    states: [
        State {
            name: "clicked"
            PropertyChanges { target: myRect; color: "red" }
        }
    ]
}
//![0]
