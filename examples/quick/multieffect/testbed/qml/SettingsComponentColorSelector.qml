// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Column {
    id: rootItem

    property alias text: textItem.text
    readonly property color value: Qt.rgba(colorRed, colorGreen, colorBlue, 1.0)
    readonly property real colorRed: sliderRed.value
    readonly property real colorGreen: sliderGreen.value
    readonly property real colorBlue: sliderBlue.value
    readonly property real itemWidth: (rootItem.width / 3) - itemMargin
    readonly property real itemMargin: 4
    readonly property real colorBarHeight: 4

    // Use this to set the initial values
    function setValues(r, g, b) {
        sliderRed.value = r;
        sliderGreen.value = g;
        sliderBlue.value = b;
    }

    Material.theme: Material.Dark
    Material.accent: Material.LightGreen
    spacing: -12
    width: 200

    Text {
        id: textItem
        anchors.horizontalCenter: parent.horizontalCenter
        color: "#f0f0f0"
        font.pixelSize: 14 * dp
    }
    Item {
        width: 1
        height: 36
    }

    Row {
        x: itemMargin / 2
        spacing: itemMargin
        opacity: rootItem.enabled ? 1.0 : 0.2
        Column {
            Rectangle {
                width: rootItem.itemWidth
                height: colorBarHeight
                border.width: 1
                border.color: "#202020"
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.rgba(0.0, colorGreen, colorBlue, 1.0) }
                    GradientStop { position: 1.0; color: Qt.rgba(1.0, colorGreen, colorBlue, 1.0) }
                }
                Text {
                    id: textItemRed
                    anchors.centerIn: parent
                    color: "#f0f0f0"
                    style: Text.Outline
                    styleColor: "#000000"
                    text: "R: " + Math.ceil(sliderRed.value * 255)
                    font.pixelSize: 14 * dp
                }
            }
            Slider {
                id: sliderRed
                width: rootItem.itemWidth
                value: 0
                from: 0
                to: 1
            }
        }
        Column {
            Rectangle {
                width: rootItem.itemWidth
                height: colorBarHeight
                border.width: 1
                border.color: "#202020"
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.rgba(colorRed, 0.0, colorBlue, 1.0) }
                    GradientStop { position: 1.0; color: Qt.rgba(colorRed, 1.0, colorBlue, 1.0) }
                }
                Text {
                    id: textItemGreen
                    anchors.centerIn: parent
                    color: "#f0f0f0"
                    style: Text.Outline
                    styleColor: "#000000"
                    text: "G: " + Math.ceil(sliderGreen.value * 255)
                    font.pixelSize: 14 * dp
                }
            }
            Slider {
                id: sliderGreen
                width: rootItem.itemWidth
                value: 0
                from: 0
                to: 1
            }
        }
        Column {
            Rectangle {
                width: rootItem.itemWidth
                height: colorBarHeight
                border.width: 1
                border.color: "#202020"
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Qt.rgba(colorRed, colorGreen, 0.0, 1.0) }
                    GradientStop { position: 1.0; color: Qt.rgba(colorRed, colorGreen, 1.0, 1.0) }
                }
                Text {
                    id: textItemBlue
                    anchors.centerIn: parent
                    color: "#f0f0f0"
                    style: Text.Outline
                    styleColor: "#000000"
                    text: "B: " + Math.ceil(sliderBlue.value * 255)
                    font.pixelSize: 14 * dp
                }
            }
            Slider {
                id: sliderBlue
                width: rootItem.itemWidth
                value: 0
                from: 0
                to: 1
            }
        }
    }
}
