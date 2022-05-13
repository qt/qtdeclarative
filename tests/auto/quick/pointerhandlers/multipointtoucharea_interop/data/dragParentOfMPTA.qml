// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    width: 640
    height: 480
    property alias touchpointPressed: tp1.pressed

    Rectangle {
        color: tp1.pressed ? "lightsteelblue" : drag.active ? "tomato" : "wheat"
        width: 180
        height: 180

        DragHandler { id: drag }

        MultiPointTouchArea {
            anchors.fill: parent
            touchPoints: [
                TouchPoint { id: tp1 }
            ]
        }
    }
}
