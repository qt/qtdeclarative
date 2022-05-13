// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "Sliders"
    property int sliderWidth: 300
    property int sliderHeight: 140

    Row {
        spacing: 40

        Column {
            spacing: 15

            Slider {
                id: customHandle
                width: sliderWidth
                height: 20
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    id: handle
                    width: 12
                    height: customHandle.height
                    color: "white"
                    border.width: 2

                    x: customHandle.visualPosition * (customHandle.availableWidth - width)
                    y: (customHandle.availableHeight - height) / 2
                }
            }

            Slider {
                id: customBackground
                width: sliderWidth
                from: 0
                to: 10
                background: Rectangle {
                    implicitHeight: 5
                    color: "lightgray"
                    border.width: 1
                }
            }

            Slider {
                id: customAll
                width: sliderWidth
                height: 20
                from: 0
                to: 10
                background: Rectangle {
                    implicitHeight: customAll.height
                    color: "lightgray"
                    border.width: 1
                }
                handle: Rectangle {
                    width: 12
                    height: customAll.height
                    color: "white"
                    border.width: 2

                    x: customAll.visualPosition * (customAll.availableWidth - width)
                    y: (customAll.availableHeight - height) / 2
                }
            }
        }

        Row {
            spacing: 20

            Slider {
                id: customVHandle
                width: 20
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    height: 12
                    width: customVHandle.width
                    color: "white"
                    border.width: 2

                    x: (customVHandle.availableWidth - width) / 2
                    y: customVHandle.visualPosition * (customVHandle.availableHeight - height)
                }
            }

            Slider {
                id: customVBackground
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                background: Rectangle {
                    implicitWidth: 5
                    color: "lightgray"
                    border.width: 1
                }
            }

            Slider {
                id: customVAll
                width: 20
                height: sliderHeight
                orientation: Qt.Vertical
                from: 0
                to: 10
                value: 5
                handle: Rectangle {
                    height: 12
                    width: customVHandle.width
                    color: "white"
                    border.width: 2

                    x: (customVAll.availableWidth - width) / 2
                    y: customVAll.visualPosition * (customVAll.availableHeight - height)
                }
                background: Rectangle {
                    implicitWidth: 5
                    color: "lightgray"
                    border.width: 1
                }
            }
        }
    }
}
