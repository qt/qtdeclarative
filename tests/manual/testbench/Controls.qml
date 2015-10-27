import QtQuick 2.6
import QtQuick.Layouts 1.2
import Qt.labs.controls 1.0

Rectangle {
    id: root

    property alias themeSwitch: themeSwitch

    property int margins: 30
    property int spacing: 10

    Switch {
        id: themeSwitch
        text: "Light/Dark"
        anchors.right: parent.right
    }

    Flow {
        id: flow
        anchors.fill: parent
        anchors.margins: 30
        spacing: 30

        RowLayout {
            BusyIndicator {
            }
            BusyIndicator {
                enabled: false
            }
        }

        RowLayout {
            spacing: root.spacing

            Button {
                text: "Normal"
            }
            Button {
                text: "Pressed"
                pressed: true
            }
            Button {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            Frame {
                Label {
                    text: "Normal\nLabel"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            Frame {
                enabled: false

                Label {
                    text: "Disabled\nLabel"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        RowLayout {
            CheckBox {
                text: "Normal"
            }
            CheckBox {
                text: "Pressed"
                pressed: true
            }
            CheckBox {
                text: "Checked"
                checked: true
            }
            CheckBox {
                text: "Checked + Pressed"
                checked: true
                pressed: true
            }
            CheckBox {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            Dial {
            }
            Dial {
                enabled: false
            }
        }

        RowLayout {
            GroupBox {
                title: "Normal"

                Item {
                    implicitWidth: 100
                    implicitHeight: 100
                }
            }
            GroupBox {
                enabled: false
                title: "Disabled"

                Item {
                    implicitWidth: 100
                    implicitHeight: 100
                }
            }
        }

        RowLayout {
            PageIndicator {
                count: 5
            }
            PageIndicator {
                count: 5
                enabled: false
            }
        }

        RowLayout {
            ProgressBar {
                value: 0.5
            }
            ProgressBar {
                value: 0.5
                enabled: false
            }
        }

        RowLayout {
            RadioButton {
                text: "Normal"
            }
            RadioButton {
                text: "Pressed"
                pressed: true
            }
            RadioButton {
                text: "Checked"
                checked: true
            }
            RadioButton {
                text: "Checked + Pressed"
                checked: true
                pressed: true
            }
            RadioButton {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            Frame {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100

                ScrollBar {
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

            Frame {
                Layout.preferredWidth: 100
                Layout.preferredHeight: 100

                ScrollBar {
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
            Rectangle {
                width: 100
                height: 100
                color: "transparent"
                border.color: "#cccccc"

                ScrollIndicator {
                    size: 0.3
                    position: 0.2
                    active: true
                    orientation: Qt.Vertical
                    height: parent.height
                    anchors.right: parent.right
                }
            }

            Rectangle {
                width: 100
                height: 100
                color: "transparent"
                border.color: "#cccccc"

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
            Switch {
                text: "Normal"
            }
            Switch {
                text: "Pressed"
                pressed: true
            }
            Switch {
                text: "Checked"
                checked: true
            }
            Switch {
                text: "Checked + Pressed"
                checked: true
                pressed: true
            }
            Switch {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            TabBar {
                TabButton {
                    text: "Normal"
                }
                TabButton {
                    text: "Pressed"
                    pressed: true
                }
                TabButton {
                    text: "Disabled"
                    enabled: false
                }
            }
        }

        RowLayout {
            TextArea {
                text: "Normal"
            }
            TextArea {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            TextField {
                text: "Normal"
            }
            TextField {
                text: "Disabled"
                enabled: false
            }
        }

        RowLayout {
            ToolBar {
                Row {
                    ToolButton {
                        text: "Normal!"
                    }
                    ToolButton {
                        text: "Pressed!"
                        pressed: true
                    }
                    ToolButton {
                        text: "Disabled!"
                        enabled: false
                    }
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
    }
}
