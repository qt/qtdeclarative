// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Rectangle {
    width: 400
    height: 400

    Rectangle {
        x: 100
        y: 100
        width: 200
        height: 200
        rotation: 45

        Rectangle {
            id: rect
            scale: 0.5
            color: "black"
            anchors.fill: parent

            PinchHandler {
                objectName: "pinchHandler"
            }
        }
    }
}
