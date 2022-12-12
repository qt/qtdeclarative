// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Rectangle {
    id: root
    width: 240; height: 320

    Rectangle {
        id: blackRect
        objectName: "blackrect"
        color: "black"
        y: 50
        x: 50
        width: 200
        height: 200

        PinchHandler {
            id: pinchHandler
            objectName: "pinchHandler"
            dragThreshold: 10
            minimumScale: 0.5
            maximumScale: 2.0
            minimumRotation: 0.0
            maximumRotation: 0.0
            minimumPointCount: 3
            maximumPointCount: 3
        }
    }
}
