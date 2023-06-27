// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: page
    color: "white"
    width: 182; height: 182

//! [normal image]
Image {
    source: "pics/borderframe.png"
    anchors.centerIn: parent
}
//! [normal image]

    Rectangle {
        width: 120; height: 120
        color: "transparent"
        border.color: "gray"
        anchors.centerIn: parent

        Rectangle {
            x: 30; y: 0
            width: 1; height: 120
            color: "gray"
        }

        Rectangle {
            x: 90; y: 0
            width: 1; height: 120
            color: "gray"
        }

        Rectangle {
            x: 0; y: 30
            width: 120; height: 1
            color: "gray"
        }

        Rectangle {
            x: 0; y: 90
            width: 120; height: 1
            color: "gray"
        }
    }
}
