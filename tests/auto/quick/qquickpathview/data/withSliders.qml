// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root
    width: 240
    height: 320

    component AdHocSlider : Rectangle {
        width: 20
        color: "gray"
        property real value: (height - handle.height - handle.y) / handler.yAxis.maximum

        Rectangle {
            id: handle
            width: parent.width
            height: 20
            border.color: "black"
            radius: 2
            y: handler.yAxis.maximum
        }

        DragHandler {
            id: handler
            target: handle
            xAxis.enabled: false
            yAxis.minimum: 0
            yAxis.maximum: parent.height - handle.height
        }
    }

    // PathView can substitute for SwipeView in case you need wrap-around (QTBUG-56422)
    PathView {
        id: view
        anchors.fill: parent
        clip: true
        snapMode: PathView.SnapOneItem
        highlightRangeMode: PathView.StrictlyEnforceRange
        currentIndex: 0
        maximumFlickVelocity: 2 * width

        model: 3
        delegate: Rectangle {
            width: root.width
            height: root.height
            border.color: "green"

            Text {
                x: 10; y: 10
                font.pointSize: 72
                color: "lightgrey"
                text: index
            }

            AdHocSlider {
                id: ahslider
                objectName: "adHocSlider"
                x: 100
                y: 10
                height: 260
            }

            Text {
                x: ahslider.x
                anchors.top: ahslider.bottom
                text: ahslider.value.toFixed(2)
            }

            Slider {
                id: slider
                objectName: "controlsSlider"
                x: 160
                y: 10
                height: 260
                orientation: Qt.Vertical
            }

            Text {
                x: slider.x
                anchors.top: slider.bottom
                text: slider.value.toFixed(2)
            }
        }

        path: Path {
            // a horizontal line going halfway outside on both sides
            startX: -view.width / 2
            startY: view.height / 2

            PathLine {
                relativeX: view.width * 3
                relativeY: 0
            }
        }
    }
}
