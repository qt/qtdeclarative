// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 170; height: 120
    color: "green"; antialiasing: true

    WheelHandler {
        property: "rotation"
        onWheel: (event)=> console.log("rotation", event.angleDelta.y,
                                       "scaled", rotation, "@", point.position,
                                       "=>", parent.rotation)
    }
}
//![0]
