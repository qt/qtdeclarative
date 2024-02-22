// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Item {
    id: root
    width: 600
    height: 400

    component ResettablePropertySlider: RowLayout {
        property alias setCheckBox: checkBox
        property alias valueSlider: slider

        CheckBox {
            id: checkBox
            text: "Set"
        }
        Slider {
            id: slider
            from: 0
            value: 25
            to: 200
            enabled: checkBox.checked

            Layout.fillWidth: true
        }
    }

    RowLayout {
        anchors.fill: parent

        GridLayout {
            columns: 2
            rowSpacing: 10

            Label {
                text: "radius"
            }
            Slider {
                id: radiusSlider
                from: 0
                value: 25
                to: 200
            }

            Label {
                text: "topLeftRadius"
            }
            ResettablePropertySlider {
                id: topLeftRadiusSlider
            }

            Label {
                text: "topRightRadius"
            }
            ResettablePropertySlider {
                id: topRightRadiusSlider
            }

            Label {
                text: "bottomLeftRadius"
            }
            ResettablePropertySlider {
                id: bottomLeftRadiusSlider
            }

            Label {
                text: "bottomRightRadius"
            }
            ResettablePropertySlider {
                id: bottomRightRadiusSlider
            }

            Label {
                text: "border.width"
            }
            Slider {
                id: borderWidthSlider
                from: 0
                value: 15
                to: 100

                Layout.fillWidth: true
            }

            Label {
                text: "border alpha"
            }
            Slider {
                id: borderAlphaSlider
                from: 0
                value: 1
                to: 1

                Layout.fillWidth: true
            }

            Label {
                text: "opacity"
            }
            Slider {
                id: opacitySlider
                from: 0
                value: 1
                to: 1

                Layout.fillWidth: true
            }

            Label {
                text: "gradient"
            }
            ComboBox {
                id: gradientComboBox
                model: ListModel {
                    id: model
                    ListElement { text: "NoGradient" }
                    ListElement { text: "LowFreq" }
                    ListElement { text: "HighFreq" }
                }

                Layout.fillWidth: true

                readonly property var gradients: [undefined, lowFGrad, highFGrad]

                property var lowFGrad: Gradient {
                    orientation: Qt.Horizontal
                    GradientStop { position: 0.0; color: "green" }
                    GradientStop { position: 0.33; color: "blue" }
                    GradientStop { position: 0.66; color: "red" }
                    GradientStop { position: 1.0; color: "yellow" }
                }

                property var highFGrad: Gradient {
                    orientation: Qt.Horizontal
                    GradientStop { position: 0.00; color: "yellow" }
                    GradientStop { position: 0.05; color: "red" }
                    GradientStop { position: 0.10; color: "yellow" }
                    GradientStop { position: 0.15; color: "red" }
                    GradientStop { position: 0.20; color: "yellow" }
                    GradientStop { position: 0.25; color: "red" }
                    GradientStop { position: 0.30; color: "yellow" }
                    GradientStop { position: 0.35; color: "red" }
                    GradientStop { position: 0.40; color: "yellow" }
                    GradientStop { position: 0.45; color: "red" }
                    GradientStop { position: 0.50; color: "yellow" }
                    GradientStop { position: 0.55; color: "red" }
                    GradientStop { position: 0.60; color: "yellow" }
                    GradientStop { position: 0.65; color: "red" }
                    GradientStop { position: 0.70; color: "yellow" }
                    GradientStop { position: 0.75; color: "red" }
                    GradientStop { position: 0.80; color: "yellow" }
                    GradientStop { position: 0.85; color: "red" }
                    GradientStop { position: 0.90; color: "yellow" }
                    GradientStop { position: 0.95; color: "red" }
                    GradientStop { position: 1.00; color: "yellow" }
                }
            }

            Label {
                text: "rotation"
            }
            Slider {
                id: rotationSlider
                from: 0
                value: 0
                to: 10

                Layout.fillWidth: true
            }
        }

        Rectangle {
            id: rect1
            color: colorDialog.selectedColor
            border.color: borderColorDialog.alphaAdjusted
            border.width: borderWidthSlider.value
            radius: radiusSlider.value
            topLeftRadius: topLeftRadiusSlider.setCheckBox.checked ? topLeftRadiusSlider.valueSlider.value : undefined
            topRightRadius: topRightRadiusSlider.setCheckBox.checked ? topRightRadiusSlider.valueSlider.value : undefined
            bottomLeftRadius: bottomLeftRadiusSlider.setCheckBox.checked ? bottomLeftRadiusSlider.valueSlider.value : undefined
            bottomRightRadius: bottomRightRadiusSlider.setCheckBox.checked ? bottomRightRadiusSlider.valueSlider.value : undefined
            antialiasing: true
            opacity: opacitySlider.value
            rotation: rotationSlider.value
            gradient: gradientComboBox.gradients[gradientComboBox.currentIndex]

            Layout.preferredWidth: root.width / 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 32

            TapHandler {
                onTapped: colorDialog.open()
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: borderColorDialog.open()
            }
        }

        ColorDialog {
            id: colorDialog
            selectedColor: "khaki"
        }

        ColorDialog {
            id: borderColorDialog
            selectedColor: "sienna"
            property color alphaAdjusted: Qt.rgba(selectedColor.r,
                                                  selectedColor.g,
                                                  selectedColor.b,
                                                  borderAlphaSlider.value)
        }
    }
}
