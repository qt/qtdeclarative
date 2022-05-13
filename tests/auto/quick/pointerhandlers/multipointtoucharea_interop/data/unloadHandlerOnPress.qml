// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    width: 640
    height: 480

    Loader {
        id: loader

        width: 480
        height: 480

        sourceComponent: Rectangle {
            id: item2
            anchors.fill: parent
            color: "blue"

            DragHandler{}
        }
    }

    Rectangle {
        color: "yellow"
        width: 180
        height: 180

        MultiPointTouchArea {
            anchors.fill: parent
            touchPoints: [
                TouchPoint { onPressedChanged: loader.sourceComponent = undefined }
            ]
        }
    }
}


