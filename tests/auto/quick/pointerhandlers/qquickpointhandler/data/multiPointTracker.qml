// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    id: root
    width: 400; height: 400

    PointHandler {
        id: ph1
        objectName: "ph1"

        target: Rectangle {
            parent: root
            visible: ph1.active
            x: ph1.point.position.x - width / 2
            y: ph1.point.position.y - height / 2
            width: 140
            height: width
            radius: width / 2
            color: "orange"
            opacity: 0.3
        }
    }

    PointHandler {
        id: ph2
        objectName: "ph2"

        target: Rectangle {
            parent: root
            visible: ph2.active
            x: ph2.point.position.x - width / 2
            y: ph2.point.position.y - height / 2
            width: 140
            height: width
            radius: width / 2
            color: "orange"
            opacity: 0.3
        }
    }

    PointHandler {
        id: ph3
        objectName: "ph3"

        target: Rectangle {
            parent: root
            visible: ph3.active
            x: ph3.point.position.x - width / 2
            y: ph3.point.position.y - height / 2
            width: 140
            height: width
            radius: width / 2
            color: "orange"
            opacity: 0.3
        }
    }
}

