// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Rectangle {
    id: root
    width: 320
    height: 240
    objectName: "root"
    color: "#222222"

    Row {
        objectName: "row"
        anchors.fill: parent
        spacing: 10
        Rectangle {
            width: 50
            height: 50
            color: "aqua"
            objectName: "dragAndTap"
            DragHandler {
                objectName: "drag"
            }
            TapHandler {
                objectName: "tap"
                gesturePolicy: TapHandler.DragThreshold
            }
        }
        Rectangle {
            width: 50
            height: 50
            color: "aqua"
            objectName: "tapAndDrag"
            TapHandler {
                objectName: "tap"
                gesturePolicy: TapHandler.DragThreshold
            }
            DragHandler {
                objectName: "drag"
            }
        }

        Rectangle {
            color: "aqua"
            width: 50
            height: 50
            objectName: "dragAndTapNotSiblings"
            DragHandler {
                objectName: "drag"
            }
            Rectangle {
                color: "blue"
                width: 30
                height: 30
                anchors.centerIn: parent
                TapHandler {
                    objectName: "tap"
                    gesturePolicy: TapHandler.DragThreshold
                }
            }
        }
        Rectangle {
            color: "aqua"
            width: 50
            height: 50
            objectName: "tapAndDragNotSiblings"
            TapHandler {
                objectName: "tap"
                gesturePolicy: TapHandler.DragThreshold
            }
            Rectangle {
                color: "blue"
                x: 10
                y: 10
                width: 30
                height: 30
                DragHandler {
                    objectName: "drag"
                }
            }
        }
    }
}
