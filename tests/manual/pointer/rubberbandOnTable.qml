// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

TableView {
    id: root
    objectName: "root"
    width: 480
    height: 480
    columnSpacing: 2
    rowSpacing: 2
    // TODO use TableModel when it's ready, to test with multiple columns
    model: 10

    delegate: Rectangle {
        id: tableDelegate
        objectName: "itemview delegate"
        color: delegateTap.pressed ? "wheat" : "beige"
        implicitWidth: 200
        implicitHeight: 140

        Rectangle {
            objectName: "button"
            anchors.centerIn: parent
            border.color: "tomato"
            border.width: 10
            color: buttonTap.pressed ? "goldenrod" : "beige"
            width: 100
            height: 100
            TapHandler {
                id: buttonTap
                objectName: "buttonTap"
            }
        }

        TapHandler {
            id: delegateTap
            objectName: "delegateTap"
        }
    }

    DragHandler {
        id: rubberBandDrag
        objectName: "rubberBandDrag"
        target: null
        acceptedDevices: PointerDevice.Mouse
    }
    Rectangle {
        visible: rubberBandDrag.active
        x: Math.min(rubberBandDrag.centroid.position.x, rubberBandDrag.centroid.pressPosition.x)
        y: Math.min(rubberBandDrag.centroid.position.y, rubberBandDrag.centroid.pressPosition.y)
        width: Math.abs(rubberBandDrag.centroid.position.x - rubberBandDrag.centroid.pressPosition.x)
        height: Math.abs(rubberBandDrag.centroid.position.y - rubberBandDrag.centroid.pressPosition.y)
        color: "transparent"
        border.color: "black"
        z: 1000
    }

    Component.onCompleted: contentItem.objectName = "TableView's contentItem"
}
