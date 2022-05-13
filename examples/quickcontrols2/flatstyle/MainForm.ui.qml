// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme

Item {
    id: form

    width: 320
    height: 480
    property alias slider: slider
    property alias checkBoxUnderline: checkBoxUnderline
    property alias checkBoxBold: checkBoxBold
    property alias sizeSwitch: sizeSwitch
    property alias button: button

    Slider {
        id: slider
        width: 297
        height: 38
        stepSize: 1
        to: 18
        from: 10
        value: 14
        anchors.topMargin: Theme.baseSize
        anchors.top: gridLayout.bottom
        anchors.right: gridLayout.right
        anchors.left: gridLayout.left
        handle: Rectangle {
            id: sliderHandle
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 26
            implicitHeight: 26
            radius: 13
            color: slider.pressed ? Theme.mainColorDarker : Theme.mainColor
            border.color: Theme.gray
        }
    }

    GridLayout {
        id: gridLayout
        anchors.top: parent.top
        anchors.topMargin: 64

        anchors.horizontalCenter: parent.horizontalCenter
        columnSpacing: Theme.baseSize * 0.5
        rowSpacing: Theme.baseSize * 0.5
        rows: 4
        columns: 2

        Label {
            text: qsTr("Toggle Size")
            font: Theme.font
        }

        Switch {
            id: sizeSwitch
            Layout.fillWidth: true
        }

        CheckBox {
            id: checkBoxBold
            text: qsTr("Bold")
            checked: true
            Layout.fillWidth: true
        }

        CheckBox {
            id: checkBoxUnderline
            text: qsTr("Underline")
            Layout.fillWidth: true
        }

        Rectangle {
            id: rectangle
            color: Theme.mainColor
            Layout.fillWidth: true
            Layout.columnSpan: 2
            Layout.preferredHeight: 38
            Layout.preferredWidth: 297
        }

        Label {
            id: label
            text: qsTr("Customization")
            font: Theme.font
        }

        Button {
            id: button
            text: qsTr("Change Color")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
    }
}
