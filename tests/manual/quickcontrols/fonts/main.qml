// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
            }
            Label {
                text: pointSizeSlider.value + " pt " + font.family
            }
            Button {
                text: "Font…"
                palette.buttonText: systemPalette.buttonText
                onClicked: fontDialog.open()
                FontDialog { id: fontDialog }
                Component.onCompleted: fontDialog.selectedFont = window.font
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "Text…"
                palette.buttonText: textColorDialog.selectedColor
                onClicked: textColorDialog.open()
                ColorDialog { id: textColorDialog }
                Component.onCompleted: textColorDialog.selectedColor = systemPalette.text
                Layout.margins: 3
                Layout.alignment: Qt.AlignRight
            }
            Button {
                text: "Buttons…"
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
            Button { text: "Button" }
            GroupBox {
                title: "GroupBox"
                ColumnLayout {
                    RadioButton { text: "RadioButton" }
                    CheckBox { text: "CheckBox" }
                }
            }
            Switch { text: "Switch" }
            TabButton { text: "TabButton" }
            TextField { placeholderText: "TextField" }
            TextArea { placeholderText: "TextArea" }
            ToolButton { text: "ToolButton" }
            Tumbler { model: 3 }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
