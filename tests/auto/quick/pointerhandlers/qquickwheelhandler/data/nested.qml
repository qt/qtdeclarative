// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14

Rectangle {
    width: 320; height: 240
    color: "lightsteelblue"; antialiasing: true
    border.color: outerWheelHandler.active ? "red" : "white"

    WheelHandler {
        id: outerWheelHandler
        objectName: "outerWheelHandler"
        property: "x"
    }

    Rectangle {
        width: 120; height: 120; x: 100; y: 60
        color: "beige"; antialiasing: true
        border.color: innerWheelHandler.active ? "red" : "white"

        WheelHandler {
            id: innerWheelHandler
            objectName: "innerWheelHandler"
            // TODO should ideally deactivate because events go to the outer handler, not because of timeout
            activeTimeout: 0.5
            property: "x"
        }
    }
}
