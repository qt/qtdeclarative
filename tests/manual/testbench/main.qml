/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Controls.Universal 2.0

ApplicationWindow {
    id: window
    visible: true
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2
    width: 750
    height: 1000

    Material.theme: themeSwitch.checked ? Material.Dark : Material.Light
    Universal.theme: themeSwitch.checked ? Universal.Dark : Universal.Light

    property int controlSpacing: 10

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    header: ToolBar {
        Material.theme: Material.Dark

        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: "Normal"
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
                onClicked: menu.visible ? menu.close() : menu.open()

                Menu {
                    id: menu
                    x: 1
                    y: 1 + parent.height

                    MenuItem {
                        text: "Option 1"
                        checkable: true
                    }
                    MenuItem {
                        text: "Option 2"
                        checkable: true
                    }
                    MenuItem {
                        text: "Option 3"
                        checkable: true
                    }
                }
            }
            ToolButton {
                text: "Pressed"
                down: true
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
            }
            ToolButton {
                text: "Checked"
                checkable: true
                checked: true
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
            }
            ToolButton {
                text: "Highlighted"
                highlighted: true
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
            }
            ToolButton {
                text: "Disabled"
                enabled: false
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                text: "Light/Dark"
            }
            Switch {
                id: themeSwitch
            }
        }
    }

    footer: TabBar {
        TabButton {
            text: "Normal"
        }
        TabButton {
            text: "Pressed"
            down: true
        }
        TabButton {
            text: "Disabled"
            enabled: false
        }
    }

    Pane {
        anchors.fill: parent

        Flickable {
            anchors.fill: parent
            contentHeight: flow.height

            Flow {
                id: flow
                width: parent.width
                spacing: 30

                RowLayout {
                    spacing: window.controlSpacing

                    Button {
                        text: "Normal"
                    }
                    Button {
                        text: "Pressed"
                        down: true
                    }
                    Button {
                        text: "Checked"
                        checked: true
                    }
                    Button {
                        text: "CH + PR"
                        checked: true
                        down: true
                    }
                    Button {
                        text: "Disabled"
                        enabled: false
                    }
                    Button {
                        text: "CH + DIS"
                        enabled: false
                        checked: true
                    }
                }

                RowLayout {
                    spacing: window.controlSpacing

                    Button {
                        text: "HI"
                        highlighted: true
                    }
                    Button {
                        text: "HI + PR"
                        highlighted: true
                        down: true
                    }
                    Button {
                        text: "HI + CH"
                        highlighted: true
                        checked: true
                    }
                    Button {
                        text: "HI+CH+PR"
                        highlighted: true
                        down: true
                        checked: true
                    }
                    Button {
                        text: "HI + DIS"
                        highlighted: true
                        enabled: false
                    }
                    Button {
                        text: "HI+CH+DIS"
                        highlighted: true
                        enabled: false
                        checked: true
                    }
                }

                RowLayout {
                    CheckBox {
                        text: "Normal"
                    }
                    CheckBox {
                        text: "Pressed"
                        down: true
                    }
                    CheckBox {
                        text: "Checked"
                        checked: true
                    }
                    CheckBox {
                        text: "CH + PR"
                        checked: true
                        down: true
                    }
                    CheckBox {
                        text: "Disabled"
                        enabled: false
                    }
                    CheckBox {
                        text: "CH + DIS"
                        checked: true
                        enabled: false
                    }
                }

                RowLayout {
                    RadioButton {
                        text: "Normal"
                    }
                    RadioButton {
                        text: "Pressed"
                        down: true
                    }
                    RadioButton {
                        text: "Checked"
                        checked: true
                    }
                    RadioButton {
                        text: "CH + PR"
                        checked: true
                        down: true
                    }
                    RadioButton {
                        text: "Disabled"
                        enabled: false
                    }
                    RadioButton {
                        text: "CH + DIS"
                        checked: true
                        enabled: false
                    }
                }

                RowLayout {
                    Switch {
                        text: "Normal"
                    }
                    Switch {
                        text: "Pressed"
                        down: true
                    }
                    Switch {
                        text: "Checked"
                        checked: true
                    }
                    Switch {
                        text: "CH + PR"
                        checked: true
                        down: true
                    }
                    Switch {
                        text: "Disabled"
                        enabled: false
                    }
                }

                RowLayout {
                    ProgressBar {
                        value: 0.5
                    }
                    ProgressBar {
                        value: 0.5
                        indeterminate: true
                    }
                    ProgressBar {
                        value: 0.5
                        enabled: false
                    }
                }

                RowLayout {
                    Slider {
                        value: 0.5
                    }
                    Slider {
                        value: 0.5
                        pressed: true
                    }
                    Slider {
                        value: 0.5
                        enabled: false
                    }
                }

                RowLayout {
                    RangeSlider {
                        first.value: 0.25
                        second.value: 0.75
                    }
                    RangeSlider {
                        first.value: 0.25
                        first.pressed: true
                        second.value: 0.75
                    }
                    RangeSlider {
                        first.value: 0.25
                        second.value: 0.75
                        enabled: false
                    }
                }

                RowLayout {
                    Item {
                        implicitWidth: normalGroupBox.width
                        implicitHeight: normalTextArea.implicitHeight

                        TextArea {
                            id: normalTextArea
                            text: "Normal"
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.width
                        implicitHeight: normalTextArea.implicitHeight

                        TextArea {
                            placeholderText: "Placeholder"
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.width
                        implicitHeight: normalTextArea.implicitHeight

                        TextArea {
                            text: "Disabled"
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalTextField.implicitHeight

                        TextField {
                            id: normalTextField
                            text: "Normal"
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalTextField.implicitHeight

                        TextField {
                            placeholderText: "Placeholder"
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalTextField.implicitHeight

                        TextField {
                            text: "Disabled"
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalSpinBox.implicitHeight

                        SpinBox {
                            id: normalSpinBox
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalSpinBox.implicitHeight

                        SpinBox {
                            up.pressed: true
                        }
                    }
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalSpinBox.implicitHeight

                        SpinBox {
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalComboBox.implicitHeight

                        ComboBox {
                            id: normalComboBox
                            model: 5
                        }
                    }

                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalComboBox.implicitHeight

                        ComboBox {
                            pressed: true
                            model: ListModel {
                                ListElement { text: "Pressed" }
                            }
                        }
                    }

                    Item {
                        implicitWidth: normalGroupBox.implicitWidth
                        implicitHeight: normalComboBox.implicitHeight

                        ComboBox {
                            enabled: false
                            model: ["Disabled"]
                        }
                    }
                }

                RowLayout {
                    GroupBox {
                        id: normalGroupBox
                        title: "Normal"

                        Item {
                            implicitWidth: 200
                            implicitHeight: 100

                            BusyIndicator {
                                anchors.centerIn: parent
                            }
                        }
                    }
                    GroupBox {
                        enabled: false
                        title: "Disabled"

                        Item {
                            implicitWidth: 200
                            implicitHeight: 100

                            BusyIndicator {
                                anchors.centerIn: parent
                            }
                        }
                    }
                    GroupBox {
                        enabled: false
                        title: "."
                        label.visible: false

                        Item {
                            implicitWidth: 200
                            implicitHeight: 100

                            PageIndicator {
                                count: 5
                                enabled: false
                                anchors.bottom: parent.bottom
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }
                }

                RowLayout {
                    Frame {
                        id: scrollBarFrame

                        Item {
                            implicitWidth: 200
                            implicitHeight: 100

                            Label {
                                text: "Normal"
                                anchors.centerIn: parent
                            }

                            ScrollBar {
                                size: 0.3
                                position: 0.2
                                active: true
                                orientation: Qt.Vertical
                                height: parent.height
                                anchors.right: parent.right
                            }
                        }
                    }

                    Frame {
                        Item {
                            implicitWidth: 200
                            implicitHeight: 100

                            Label {
                                text: "Pressed"
                                anchors.centerIn: parent
                            }

                            ScrollBar {
                                size: 0.3
                                position: 0.2
                                active: true
                                orientation: Qt.Vertical
                                height: parent.height
                                anchors.right: parent.right
                                pressed: true
                            }
                        }
                    }

                    Frame {
                        Item {
                            implicitWidth: 200
                            implicitHeight: 100
                            enabled: false

                            Label {
                                text: "Disabled"
                                anchors.centerIn: parent
                            }

                            ScrollBar {
                                size: 0.3
                                position: 0.2
                                active: true
                                orientation: Qt.Vertical
                                height: parent.height
                                anchors.right: parent.right
                            }
                        }
                    }
                }

                RowLayout {
                    Frame {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100

                        ScrollIndicator {
                            size: 0.3
                            position: 0.2
                            active: true
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                        }
                    }

                    Frame {
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100

                        ScrollIndicator {
                            size: 0.3
                            position: 0.2
                            active: true
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Frame {
                        Tumbler {
                            model: 5
                            implicitWidth: 100
                            implicitHeight: 100
                        }
                    }
                    Frame {
                        Tumbler {
                            model: 5
                            implicitWidth: 100
                            implicitHeight: 100
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Dial {
                    }
                    Dial {
                        enabled: false
                    }
                }
            }
        }
    }
}

