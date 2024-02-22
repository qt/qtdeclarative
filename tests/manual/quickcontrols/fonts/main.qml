// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 640
    title: qsTr("Font and Color Inheritance Test")
    font.pointSize: pointSizeSlider.value
    font.family: fontDialog.selectedFont.family
    palette.text: textColorDialog.selectedColor
    palette.buttonText: buttonTextColorDialog.selectedColor
    SystemPalette { id: systemPalette }

    header: ToolBar {
        RowLayout {
            width: parent.width
            Slider {
                id: pointSizeSlider
                from: 6
                to: 48
                value: 12
                stepSize: 1
                Layout.fillWidth: false
            }
            Label {
                text: pointSizeSlider.value + " pt " + font.family
            }
            Button {
                text: "Font…"
                palette.buttonText: systemPalette.buttonText
                Layout.fillWidth: false
                onClicked: fontDialog.open()
                FontDialog { id: fontDialog }
                Component.onCompleted: fontDialog.selectedFont = window.font
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Text…"
                palette.buttonText: textColorDialog.selectedColor
                Layout.fillWidth: false
                onClicked: textColorDialog.open()
                ColorDialog { id: textColorDialog }
                Component.onCompleted: textColorDialog.selectedColor = systemPalette.text
                Layout.margins: 3
                Layout.alignment: Qt.AlignRight
            }
            Button {
                text: "Buttons…"
                Layout.fillWidth: false
                onClicked: buttonTextColorDialog.open()
                ColorDialog { id: buttonTextColorDialog }
                Component.onCompleted: buttonTextColorDialog.selectedColor = systemPalette.buttonText
                Layout.margins: 3
                Layout.alignment: Qt.AlignRight
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: layout.implicitWidth + 40
        contentHeight: layout.implicitHeight + 40

        ColumnLayout {
            id: layout
            anchors.fill: parent
            anchors.margins: 20
            Label {
                text: "Label with **Bold** *Italics* _Underline_ ~~Strikethrough~~ `Mono`"
                textFormat: Label.MarkdownText
            }
            Button {
                text: "Button"
                Layout.fillWidth: false
            }
            GroupBox {
                title: "GroupBox"
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    RadioButton { text: "RadioButton" }
                    CheckBox { text: "CheckBox" }
                }
            }
            Switch {
                text: "Switch"
                Layout.fillWidth: false
            }
            TabButton {
                text: "TabButton"
                Layout.fillWidth: false
            }
            TextField {
                placeholderText: "TextField"
                Layout.fillWidth: false
            }
            TextArea {
                placeholderText: "TextArea"
                Layout.fillWidth: false
                Layout.fillHeight: false
            }
            ToolButton {
                text: "ToolButton"
            }
            Tumbler {
                model: 3
                Layout.fillWidth: false
                Layout.fillHeight: false
            }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
