// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {

    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    Item {
        x: 20
        y: 270
        width: 200
        height: 200
        MouseArea {
            anchors.fill: parent
            onClicked: topRect.visible = !topRect.visible
        }
        Rectangle {
            x: 20
            y: 20
            width: 100
            height: 100
            color: "red"
        }
        Rectangle {
            id: topRect
            opacity: 0.5

            x: 100
            y: 100
            width: 100
            height: 100
            color: "blue"
        }
    }
}
//![0]
