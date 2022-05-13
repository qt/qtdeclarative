// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import "content"

Rectangle {
    id: root
    width: 400
    height: 400
    objectName: "root"
    color: "steelblue"

    component FlickableStuff: Flickable {
        width: root.width / 2 - 2
        height: 400
        contentWidth: width
        contentHeight: 800
        pressDelay: pressDelayCB.checked ? 1000 : 0

        Rectangle {
            anchors.fill: parent
            color: "#222222"
        }

        Column {
            spacing: 6
            anchors.fill: parent
            anchors.margins: 6
            Rectangle {
                radius: 5
                width: parent.width - 12
                height: pressDelayCB.implicitHeight + 12
                x: 6
                color: "lightgray"
                CheckBox {
                    x: 6; y: 6
                    id: pressDelayCB
                    label: "press delay"
                }
            }

            Row {
                spacing: 6
                Slider {
                    label: "DragHandler"
                    value: 49; width: 100; height: 400
                }
                MouseAreaSlider {
                    label: "MouseArea"
                    value: 49; width: 100; height: 400
                }
                Column {
                    spacing: 6
                    MouseAreaButton {
                        text: "MouseArea"
                    }
                    MptaButton {
                        text: "MultiPointTouchArea"
                    }
                    MptaButton {
                        text: "MultiPointTouchArea"
                    }
                    TapHandlerButton {
                        text: "TapHandler"
                    }
                    TapHandlerButton {
                        text: "TapHandler"
                    }
                }
            }
        }
    }

    Row {
        anchors.fill: parent
        spacing: 2
        FlickableStuff { }
        FlickableStuff { }
    }
}
