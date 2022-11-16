// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 50; height: 200

    Rectangle {
        id: knob
        width: parent.width; height: width; radius: width / 2
        anchors.centerIn: parent
        color: "lightsteelblue"

        Rectangle {
            antialiasing: true
            width: 4; height: 20
            x: parent.width / 2 - 2
        }

        WheelHandler {
            property: "rotation"
        }
    }

    DragHandler {
        target: null
        dragThreshold: 0
        yAxis.onActiveValueChanged: (delta)=> { knob.rotation -= delta }
    }
}
//![0]
