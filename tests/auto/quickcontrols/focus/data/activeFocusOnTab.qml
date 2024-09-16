// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Item {
    id: main
    objectName: "main"
    width: 400
    height: 800
    focus: true
    Component.onCompleted: button1.focus = true
    Column {
        anchors.fill: parent
        id: column
        objectName: "column"
        BusyIndicator {
            id: busyindicator
            objectName: "busyindicator"
        }
        Button {
            id: button1
            objectName: "button1"
            text: "button1"
        }
        Button {
            id: button2
            objectName: "button2"
            text: "button2"
        }
        CheckBox {
            id: checkbox
            objectName: "checkbox"
            text: "checkbox"
        }
        GroupBox {
            id: groupbox1
            objectName: "groupbox1"
            title: "grouppox1"
            Column {
                anchors.fill: parent
                CheckBox {
                    id: checkbox1
                    objectName: "checkbox1"
                    text: "checkbox1"
                }
                CheckBox {
                    id: checkbox2
                    objectName: "checkbox2"
                    text: "checkbox2"
                }
            }
        }
        Label {
            id: label
            objectName: "label"
            text: "label"
        }
        PageIndicator {
            id: pageindicator
            objectName: "pageindicator"
        }
        ProgressBar {
            id: progressbar
            objectName: "progressbar"
            indeterminate: true
        }
        RadioButton {
            id: radiobutton
            objectName: "radiobutton"
            text: "radiobutton"
        }
        GroupBox {
            id: groupbox2
            objectName: "groupbox2"
            title: "groupbox2"
            Column {
                anchors.fill: parent
                RadioButton {
                    id: radiobutton1
                    objectName: "radiobutton1"
                    text: "radiobutton1"
                }
                RadioButton {
                    id: radiobutton2
                    objectName: "radiobutton2"
                    text: "radiobutton2"
                }
            }
        }
        RangeSlider {
            id: rangeslider
            objectName: "rangeslider"
            first.handle.objectName: "rangeslider.first"
            second.handle.objectName: "rangeslider.second"
        }
        // ScrollBar
        ScrollIndicator {
            id: scrollindicator
            objectName: "scrollindicator"
        }
        Slider {
            id: slider
            objectName: "slider"
            value: 0.5
        }
        SpinBox {
            id: spinbox
            objectName: "spinbox"
            editable: true
            value: 50
        }
        // StackView
        Switch {
            id: swtich // switch
            objectName: "switch"
            text: "switch"
        }
        TabBar {
            width: parent.width
            id: tabbar
            objectName: "tabbar"
            TabButton {
                id: tabbutton1
                objectName: "tabbutton1"
                text: "tabbutton1"
            }
            TabButton {
                id: tabbutton2
                objectName: "tabbutton2"
                text: "tabbutton2"
            }
        }
        TextField {
            id: textfield
            objectName: "textfield"
            text: "abc"
        }
        ToolBar {
            width: parent.width
            id: toolbar
            objectName: "toolbar"
            ToolButton {
                id: toolbutton
                objectName: "toolbutton"
                text: "toolbutton"
            }
        }
        TextArea {
            id: textarea
            objectName: "textarea"
            text: "abc"
        }
    }
}
