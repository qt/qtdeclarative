// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 640
    height: 480

    Rectangle {
        id: feedback
        border.color: "red"
        width: Math.max(10, handler.centroid.ellipseDiameters.width)
        height: Math.max(10, handler.centroid.ellipseDiameters.height)
        radius: Math.max(width, height) / 2
        visible: handler.active
    }

    DragHandler {
        id: handler
        target: feedback
    }
}
//![0]
