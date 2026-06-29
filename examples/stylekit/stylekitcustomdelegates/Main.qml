// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 800
    height: 500
    visible: true
    title: qsTr("StyleKit")

    palette.accent: "darkseagreen"
    StyleKit.style: CustomDelegatesStyle {}

    GridLayout {
        id: controls
        anchors.fill: parent
        anchors.margins: 20
        columns: 2
        columnSpacing: 20
        rowSpacing: 20

        GroupBox {
            title: "Buttons with Overlay"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                spacing: 10
                Button { text: "Pizza" }
                Button { text: "Calzone" }
                Button { text: "Focaccia" }
            }
        }

        GroupBox {
            title: "RadioButtons with Underlay"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                spacing: 10
                RadioButton { text: "Pasta" }
                RadioButton { text: "Lasagna"; checked: true }
                RadioButton { text: "Burrita" }
            }
        }

        GroupBox {
            title: "CheckBoxes with Shader Effect"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                spacing: 10
                CheckBox { text: "Mango"; checked: true }
                CheckBox { text: "Avocado" }
                CheckBox { text: "Banano"; checked: true }
            }
        }

        GroupBox {
            title: "Sliders with Custom Handles"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                spacing: 10
                Slider { from: 0; to: 10; value: 5 }
                RangeSlider { from: 0; to: 10; first.value: 2; second.value: 8 }
            }
        }

        GroupBox {
            title: "Switch with Noise Effect"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                Switch { checked: true; text: "Jalapeño" }
            }
        }

        GroupBox {
            title: "TextField with Custom Shadow"
            Layout.fillWidth: true
            clip: true
            RowLayout {
                TextField { placeholderText: "Potato" }
            }
        }
    }
}
