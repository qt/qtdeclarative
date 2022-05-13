// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.0

Item {
    id: root
    width: 400
    height: 400

    MouseArea {
        anchors.fill: parent
    }

    Item {
        width: 400
        height: 400

        MultiPointTouchArea {
            anchors.fill: parent
            touchPoints: [ TouchPoint { id: point1 }]
            touchPoints: [ TouchPoint { id: point2 }]
        }
    }
}
