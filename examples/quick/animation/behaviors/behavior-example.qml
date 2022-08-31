// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    width: 320; height: 480
    color: "#343434"

    Rectangle {
        anchors.centerIn: parent
        width: 200; height: 200
        radius: 30
        color: "transparent"
        border.width: 4; border.color: "white"


        SideRect {
            id: leftRect
            focusItem: focusRect
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.left }
            text: "Left"
        }

        SideRect {
            id: rightRect
            focusItem: focusRect
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.right }
            text: "Right"
        }

        SideRect {
            id: topRect
            focusItem: focusRect
            anchors { verticalCenter: parent.top; horizontalCenter: parent.horizontalCenter }
            text: "Top"
        }

        SideRect {
            id: bottomRect
            focusItem: focusRect
            anchors { verticalCenter: parent.bottom; horizontalCenter: parent.horizontalCenter }
            text: "Bottom"
        }

        FocusRect {
            id: focusRect
        }
    }
}
