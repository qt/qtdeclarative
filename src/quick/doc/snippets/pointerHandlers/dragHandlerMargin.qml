// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [entire]
Item {
    width: 320
    height: 240
    //![draggable]
    Rectangle {
        width: 24
        height: 24
        border.color: "steelblue"
        Text {
            text: "it's\ntiny"
            font.pixelSize: 7
            rotation: -45
            anchors.centerIn: parent
        }

        DragHandler {
            margin: 12
        }
    }
    //![draggable]
}
//! [entire]
