// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.0

Item {
    width: 200; height: 200
    property int outerEnterEvents: 0
    property int outerExitEvents: 0
    property int innerEnterEvents: 0
    property int innerExitEvents: 0

    DropArea {
        objectName: "outerDropArea"
        x: 75; y: 75
        width: 100; height: 100
        Rectangle {
            anchors.fill: parent
            color: "green"
        }
        onEntered: ++outerEnterEvents
        onExited: ++outerExitEvents

        DropArea {
            objectName: "innerDropArea"
            width: 50; height: 50
            Rectangle {
                anchors.fill: parent
                color: "blue"
            }
            onEntered: ++innerEnterEvents
            onExited: ++innerExitEvents
        }
    }

    Rectangle {
        width: 20; height: 20
        color: dragArea.pressed ? "red" : "brown"
        Drag.active: dragArea.drag.active
        MouseArea {
            id: dragArea
            objectName: "dragArea"
            anchors.fill: parent
            drag.target: parent
        }
    }
}
