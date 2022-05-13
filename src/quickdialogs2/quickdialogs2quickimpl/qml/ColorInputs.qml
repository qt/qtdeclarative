// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    required property color currentColor
    required property real red
    required property real green
    required property real blue
    required property real hue
    required property real saturation
    required property real value
    required property real lightness
    required property real alpha
    property bool showAlpha
    property alias currentIndex: colorSystemComboBox.currentIndex
    signal emitHex(string hex)
    signal emitRed(int r)
    signal emitGreen(int g)
    signal emitBlue(int b)
    signal emitHue(real h)
    signal emitSaturation(real s)
    signal emitValue(real v)
    signal emitLightness(real l)
    signal emitAlpha(real a)

    ComboBox {
        id: colorSystemComboBox
        objectName: "colorSystemComboBox"
        editable: false
        flat: true
        background.implicitWidth: 0
        implicitContentWidthPolicy: ComboBox.WidestText
        model: ListModel {
            ListElement {
                name: qsTr("Hex")
            }
            ListElement {
                name: qsTr("RGB")
            }
            ListElement {
                name: qsTr("HSV")
            }
            ListElement {
                name: qsTr("HSL")
            }
        }
    }
    StackLayout {
        objectName: "colorParameters"
        currentIndex: colorSystemComboBox.currentIndex

        Layout.fillWidth: true
        Layout.fillHeight: false

        TextField {
            horizontalAlignment: Qt.AlignHCenter
            text: root.currentColor
            selectByMouse: true
            maximumLength: 9
            validator: RegularExpressionValidator {
                regularExpression: /^#[0-9A-f]{6}(?:[0-9A-f]{2})?$/
            }
            onEditingFinished: function() {
                root.emitHex(text)
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: root.red
                maximumLength: 3
                validator: IntValidator {
                    bottom: 0
                    top: 999
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitRed(Math.max(Math.min(parseInt(text), 255), 0))
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: root.green
                maximumLength: 3
                validator: IntValidator {
                    bottom: 0
                    top: 999
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitGreen(Math.max(Math.min(parseInt(text), 255), 0))
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: root.blue
                maximumLength: 3
                validator: IntValidator {
                    bottom: 0
                    top: 999
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitBlue(Math.max(Math.min(parseInt(text), 255), 0))
                }
            }
            TextField {
                visible: root.showAlpha
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.alpha * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitAlpha(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.hue === -1.0 ? 0 : control.hue * 360).toString() + "°"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}°?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitHue(Math.max(Math.min(text.match(/^(\d+)°?$/)[1], 360), 0) / 360.0)
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.saturation * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitSaturation(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.value * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitValue(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
            TextField {
                visible: root.showAlpha
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.alpha * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitAlpha(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.hue === -1.0 ? 0 : control.hue * 360).toString() + "°"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}°?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitHue(Math.max(Math.min(text.match(/^(\d+)°?$/)[1], 360), 0) / 360.0)
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.saturation * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitSaturation(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
            TextField {
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.lightness * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitLightness(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
            TextField {
                visible: root.showAlpha
                horizontalAlignment: Qt.AlignHCenter
                text: Math.round(root.alpha * 100).toString() + "%"
                maximumLength: 4
                validator: RegularExpressionValidator {
                    regularExpression: /^[0-9]{0,3}%?$/
                }

                Layout.preferredWidth: 1
                Layout.fillWidth: true

                onEditingFinished: function() {
                    root.emitAlpha(Math.max(Math.min(text.match(/^(\d+)%?$/)[1], 100), 0) / 100.0)
                }
            }
        }
    }
}
