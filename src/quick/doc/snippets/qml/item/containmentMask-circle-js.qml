// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

//![0]
Rectangle {
    id: circle
    width: 100; height: width
    radius: width / 2
    color: tapHandler.pressed ? "tomato" : hoverHandler.hovered ? "darkgray" : "lightgray"

    TapHandler { id: tapHandler }
    HoverHandler { id: hoverHandler }

    containmentMask: QtObject {
        property alias radius: circle.radius
        function contains(point: point) : bool {
            return (Math.pow(point.x - radius, 2) + Math.pow(point.y - radius, 2)) < Math.pow(radius, 2)
        }
    }
}
//![0]
