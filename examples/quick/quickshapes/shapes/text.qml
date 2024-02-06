// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Rectangle {
    color: "lightGray"
    width: 256
    height: 256

    Shape {
        anchors.centerIn: parent
        width: 200
        height: 100

        ShapePath {
            id: capTest
            strokeColor: "black"
            strokeWidth: 1
            fillColor: "black"

            PathText {
                x: 0
                y: 0
                text: qsTr("Qt!")
                font.family: "Arial"
                font.pixelSize: 150
            }
        }
    }
}
