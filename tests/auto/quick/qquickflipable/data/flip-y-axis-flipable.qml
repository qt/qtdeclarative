// Copyright (C) 2023 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Flipable {
    id: flipable

    property real angle: 0
    width: 3840 // wider than 1024 * 2: part of it goes behind the camera while flipping
    height: 2160

    front: Rectangle {
        width: parent.width
        height: parent.height
        color: "red"
        anchors.centerIn: parent
    }
    back: Rectangle {
        color: "yellow"
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
    }
    transform: Rotation {
        id: rotation
        origin.x: flipable.width / 2
        origin.y: flipable.height / 2
        axis.x: 0; axis.y: 1; axis.z: 0
        angle: flipable.angle
    }
}
