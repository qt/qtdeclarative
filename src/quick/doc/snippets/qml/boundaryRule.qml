// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import Qt.labs.animation

Rectangle {
    id: root
    width: 170; height: 120
    color: "green"

    DragHandler {
        id: dragHandler
        yAxis.minimum: -1000
        xAxis.minimum: -1000
        onActiveChanged: if (!active) xbr.returnToBounds();
    }

    BoundaryRule on x {
        id: xbr
        minimum: -50
        maximum: 100
        minimumOvershoot: 40
        maximumOvershoot: 40
    }
}
//![0]
