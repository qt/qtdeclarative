// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root
    width: 600
    height: 540
    objectName: "root"
    color: "#222222"

    Instantiator {
        model: 3
        // non-interfering, just for visual monitoring of points
        delegate: PointHandler {
            id: ph
            required property int index
            objectName: "ph" + index
            parent: root

            target: Rectangle {
                parent: root
                visible: ph.active
                x: ph.point.position.x - width / 2
                y: ph.point.position.y - height / 2
                width: 10; height: width; radius: width / 2
                color: Qt.rgba(1, 0.33 * ph.index, 1 - 0.3 * ph.index)
            }
        }
    }

    Grid {
        objectName: "grid"
        anchors.fill: parent
        spacing: 10
        columns: 6
        Repeater {
            id: top
            objectName: "top"
            model: 6

            delegate: Slider {
                objectName: label
                label: "Drag Knob " + index
                width: 140
            }
        }
        Repeater {
            id: bottom
            objectName: "bottom"
            model: 6

            delegate: DragAnywhereSlider {
                objectName: label
                label: "Drag Anywhere " + index
                width: 140
            }
        }
    }
}
