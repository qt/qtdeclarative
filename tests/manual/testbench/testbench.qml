/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Controls.Universal 2.1

ApplicationWindow {
    id: window
    visible: true
    width: 750
    height: 1000

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }

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
                text: "ToolButton"
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
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

            ToolSeparator {}

            ToolButton {
                text: "1"
            }
            ToolButton {
                text: "2"
            }

            ToolSeparator {}

            ToolButton {
                id: menuButton
                text: "Menu"
                hoverEnabled: true
                ToolTip.text: text
                ToolTip.delay: 1000
                ToolTip.visible: hovered
                checked: menu.visible
                checkable: true

                Menu {
                    id: menu
                    x: 1
                    y: 1 + parent.height
                    visible: menuButton.checked
                    closePolicy: Popup.CloseOnPressOutsideParent

                    MenuItem {
                        text: "MenuItem"
                    }
                    MenuItem {
                        text: "Pressed"
                        down: true
                    }
                    MenuItem {
                        text: "Disabled"
                        enabled: false
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: "Checked"
                        checked: true
                    }
                    MenuItem {
                        text: "CH+PR"
                        checked: true
                        down: true
                    }
                    MenuItem {
                        text: "CH+DIS"
                        checked: true
                        enabled: false
                    }
                }
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
            text: "TabButton"
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
                        text: "Button"
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
                    spacing: window.controlSpacing * 2

                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            down: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI + PR"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            checked: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI + CH"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            down: true
                            checked: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI+CH+PR"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            enabled: false
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI + DIS"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                    ColumnLayout {
                        RoundButton {
                            highlighted: true
                            enabled: false
                            checked: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "HI+CH+DIS"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                RowLayout {
                    CheckBox {
                        text: "CheckBox"
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
                    CheckBox {
                        text: "Tri-state\nCheckBox"
                        tristate: true
                    }
                    CheckBox {
                        text: "Pressed"
                        down: true
                        tristate: true
                    }
                    CheckBox {
                        text: "Partially\nChecked"
                        tristate: true
                        checkState: Qt.PartiallyChecked
                    }
                    CheckBox {
                        text: "CH + PR"
                        tristate: true
                        checkState: Qt.PartiallyChecked
                        down: true
                    }
                    CheckBox {
                        text: "Disabled"
                        tristate: true
                        enabled: false
                    }
                    CheckBox {
                        text: "CH + DIS"
                        tristate: true
                        checkState: Qt.PartiallyChecked
                        enabled: false
                    }
                }

                RowLayout {
                    RadioButton {
                        text: "RadioButton"
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
                        text: "Switch"
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
                    Switch {
                        text: "CH + DIS"
                        checked: true
                        enabled: false
                    }
                }

                RowLayout {
                    ProgressBar {
                        value: slider.value
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
                        id: slider
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
                    spacing: window.controlSpacing * 2

                    TextArea {
                        text: "TextArea"
                        Layout.preferredWidth: normalGroupBox.implicitWidth
                    }

                    TextArea {
                        placeholderText: "Placeholder"
                        Layout.preferredWidth: normalGroupBox.implicitWidth
                    }

                    TextArea {
                        text: "Disabled"
                        enabled: false
                        Layout.preferredWidth: normalGroupBox.implicitWidth
                    }
                }

                RowLayout {
                    spacing: window.controlSpacing * 2

                    TextField {
                        text: "TextField"
                    }
                    TextField {
                        placeholderText: "Placeholder"
                    }
                    TextField {
                        text: "Disabled"
                        enabled: false
                    }
                }

                RowLayout {
                    spacing: window.controlSpacing * 2

                    SpinBox {
                        id: normalSpinBox
                    }
                    SpinBox {
                        up.pressed: true
                    }
                    SpinBox {
                        editable: true
                    }
                    SpinBox {
                        enabled: false
                    }
                }

                RowLayout {
                    spacing: window.controlSpacing * 2

                    ComboBox {
                        model: 5
                    }

                    ComboBox {
                        pressed: true
                        model: ["Pressed"]
                    }

                    ComboBox {
                        editable: true
                        model: ListModel {
                            id: fruitModel
                            ListElement { text: "Banana" }
                            ListElement { text: "Apple" }
                            ListElement { text: "Coconut" }
                        }
                        onAccepted: {
                            if (find(editText) === -1)
                                fruitModel.append({text: editText})
                        }
                    }

                    ComboBox {
                        enabled: false
                        model: ["Disabled"]
                    }
                }

                RowLayout {
                    GroupBox {
                        id: normalGroupBox
                        title: "GroupBox"

                        contentWidth: 200
                        contentHeight: 100

                        BusyIndicator {
                            anchors.centerIn: parent
                        }
                    }
                    GroupBox {
                        enabled: false
                        title: "Disabled"

                        contentWidth: 200
                        contentHeight: 100

                        BusyIndicator {
                            anchors.centerIn: parent
                        }
                    }
                    GroupBox {
                        enabled: false
                        title: "."
                        label.visible: false

                        contentWidth: 200
                        contentHeight: 100

                        PageIndicator {
                            count: 5
                            enabled: false
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                RowLayout {
                    Frame {
                        id: scrollBarFrame

                        padding: 0
                        contentWidth: 200
                        contentHeight: 100

                        Label {
                            text: "ScrollBar"
                            anchors.centerIn: parent
                        }

                        ScrollBar {
                            size: 0.6
                            position: 0.1
                            policy: ScrollBar.AlwaysOn
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                        }
                    }

                    Frame {
                        padding: 0
                        contentWidth: 200
                        contentHeight: 100

                        Label {
                            text: "Pressed"
                            anchors.centerIn: parent
                        }

                        ScrollBar {
                            size: 0.6
                            position: 0.1
                            policy: ScrollBar.AlwaysOn
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                            pressed: true
                        }
                    }

                    Frame {
                        padding: 0
                        contentWidth: 200
                        contentHeight: 100
                        enabled: false

                        Label {
                            text: "Disabled"
                            anchors.centerIn: parent
                        }

                        ScrollBar {
                            size: 0.6
                            position: 0.1
                            policy: ScrollBar.AlwaysOn
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                        }
                    }
                }

                RowLayout {
                    Frame {
                        padding: 0
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100

                        ScrollIndicator {
                            size: 0.6
                            position: 0.1
                            active: true
                            orientation: Qt.Vertical
                            height: parent.height
                            anchors.right: parent.right
                        }
                    }

                    Frame {
                        padding: 0
                        Layout.preferredWidth: 100
                        Layout.preferredHeight: 100

                        ScrollIndicator {
                            size: 0.6
                            position: 0.1
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
                            implicitWidth: 80
                            implicitHeight: 100
                        }
                    }
                    Frame {
                        Tumbler {
                            model: 5
                            implicitWidth: 80
                            implicitHeight: 100
                            enabled: false
                        }
                    }
                }

                RowLayout {
                    Dial {
                        value: 0.5
                        implicitWidth: 100
                        implicitHeight: 100
                    }
                    Dial {
                        value: 0.5
                        implicitWidth: 100
                        implicitHeight: 100
                        enabled: false
                    }
                }

                ListModel {
                    id: checkableDelegateModel
                    ListElement { label: "Pressed"; press: true }
                    ListElement { label: "Checked"; check: true }
                    ListElement { label: "CH + PR"; check: true; press: true }
                    ListElement { label: "Disabled"; disabled: true }
                    ListElement { label: "CH + DIS"; check: true; disabled: true }
                }

                RowLayout {
                    Frame {
                        Column {
                            width: 200

                            CheckDelegate {
                                text: "CheckDelegate"
                                width: parent.width
                                focusPolicy: Qt.StrongFocus
                            }

                            Repeater {
                                model: checkableDelegateModel
                                delegate: CheckDelegate {
                                    text: label
                                    width: parent.width
                                    down: press
                                    checked: check
                                    enabled: !disabled
                                    focusPolicy: Qt.StrongFocus
                                }
                            }
                        }
                    }

                    Frame {
                        Column {
                            width: 200

                            RadioDelegate {
                                text: "RadioDelegate"
                                width: parent.width
                                focusPolicy: Qt.StrongFocus
                            }

                            Repeater {
                                model: checkableDelegateModel
                                delegate: RadioDelegate {
                                    text: label
                                    down: press
                                    width: parent.width
                                    checked: check
                                    enabled: !disabled
                                    focusPolicy: Qt.StrongFocus
                                }
                            }
                        }
                    }

                    Frame {
                        Column {
                            width: 200

                            SwitchDelegate {
                                text: "SwitchDelegate"
                                width: parent.width
                                focusPolicy: Qt.StrongFocus
                            }

                            Repeater {
                                model: checkableDelegateModel
                                delegate: SwitchDelegate {
                                    text: label
                                    width: parent.width
                                    checked: check
                                    down: press
                                    enabled: !disabled
                                    focusPolicy: Qt.StrongFocus
                                }
                            }
                        }
                    }
                }

                ListModel {
                    id: regularDelegateModel
                    ListElement { label: "Pressed"; press: true }
                    ListElement { label: "Disabled"; disabled: true }
                }

                RowLayout {
                    Frame {
                        Column {
                            width: 200

                            ItemDelegate {
                                text: "ItemDelegate"
                                width: parent.width
                                focusPolicy: Qt.StrongFocus
                            }

                            Repeater {
                                model: regularDelegateModel
                                delegate: ItemDelegate {
                                    text: label
                                    width: parent.width
                                    down: press
                                    enabled: !disabled
                                    focusPolicy: Qt.StrongFocus
                                }
                            }
                        }
                    }
                    Frame {
                        Column {
                            id: listView
                            width: 200
                            clip: true

                            SwipeDelegate {
                                text: "SwipeDelegate"
                                width: parent.width
                                swipe.left: removeComponent
                                swipe.right: removeComponent
                                focusPolicy: Qt.StrongFocus
                            }

                            Repeater {
                                model: regularDelegateModel
                                delegate: SwipeDelegate {
                                    text: label
                                    width: parent.width
                                    down: press
                                    enabled: !disabled
                                    focusPolicy: Qt.StrongFocus

                                    swipe.left: removeComponent
                                    swipe.right: removeComponent
                                }
                            }

                            Component {
                                id: removeComponent

                                Rectangle {
                                    color: parent.swipe.complete && parent.pressed ? "#333" : "#444"
                                    width: parent.width
                                    height: parent.height
                                    clip: true

                                    Label {
                                        font.pixelSize: parent.parent.font.pixelSize
                                        text: "Boop"
                                        color: "white"
                                        anchors.centerIn: parent
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                parent: window.contentItem
                x: parent.width - width
                height: parent.height
            }
        }
    }
}

