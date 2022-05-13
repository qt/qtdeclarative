// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 480
    height: 640
    title: qsTr("Hello World")

    header: ToolBar {
        Slider {
            from: 16
            to: 48
            stepSize: 1
            onValueChanged: control.font.pointSize = value
        }
    }

    Flickable {
        anchors.fill: parent
        contentWidth: control.width
        contentHeight: control.height

        T.Control {
            id: control
            width: layout.implicitWidth + 40
            height: layout.implicitHeight + 40
            ColumnLayout {
                id: layout
                anchors.fill: parent
                anchors.margins: 20
                Button { text: "Button" }
                CheckBox { text: "CheckBox" }
                GroupBox { title: "GroupBox" }
                RadioButton { text: "RadioButton" }
                Switch { text: "Switch" }
                TabButton {
                    text: "TabButton"
                    font.pointSize: control.font.pointSize
                }
                TextField { placeholderText: "TextField" }
                TextArea { placeholderText: "TextArea" }
                ToolButton { text: "ToolButton" }
                Tumbler { model: 3 }
            }
        }

        ScrollBar.vertical: ScrollBar { }
    }
}
