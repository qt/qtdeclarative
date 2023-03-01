// Copyright (C) 2023 Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    width: 480; height: 320

    Rectangle {
        color: handler.active ? "tomato" : "wheat"
        x: handler.point.position.x - width / 2
        y: handler.point.position.y - height / 2
        width: 20; height: width; radius: width / 2
    }

    PointHandler {
        id: handler
        acceptedButtons: Qt.MiddleButton | Qt.RightButton
    }
}
//![0]
