// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Window {
    width: 480
    height: 320
    visible: true

    Item {
        id: glassPane
        z: 10000
        anchors.fill: parent

        //![1]
        PointHandler {
            id: handler
            acceptedDevices: PointerDevice.TouchScreen | PointerDevice.TouchPad
            target: Rectangle {
                parent: glassPane
                color: "red"
                visible: handler.active
                x: handler.point.position.x - width / 2
                y: handler.point.position.y - height / 2
                width: 20; height: width; radius: width / 2
            }
        }
        //![1]
    }
}
//![0]
