// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 640
    height: 480

    DragHandler {
        id: handler
        target: null
    }

    Text {
        color: handler.active ? "darkgreen" : "black"
        text: handler.centroid.position.x.toFixed(1) + "," + handler.centroid.position.y.toFixed(1)
        x: handler.centroid.position.x - width / 2
        y: handler.centroid.position.y - height
    }
}
//![0]
