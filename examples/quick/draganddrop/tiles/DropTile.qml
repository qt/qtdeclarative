// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//! [0]
DropArea {
    id: dragTarget

    property string colorKey
    property alias dropProxy: dragTarget

    width: 64; height: 64
    keys: [ colorKey ]

    Rectangle {
        id: dropRectangle

        anchors.fill: parent
        color: dragTarget.colorKey

        states: [
            State {
                when: dragTarget.containsDrag
                PropertyChanges {
                    dropRectangle.color: "grey"
                }
            }
        ]
    }
}
//! [0]
