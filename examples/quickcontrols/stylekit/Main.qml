// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import "styles"

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true
    title: qsTr("StyleKit")

    // Set the initial style:
    StyleKit.style: hazeStyle

    // Instantiate the available styles. The user can switch between them
    // at runtime, and each style provides its own set of themes.
    Haze { id: hazeStyle }
    Vitrum { id: vitrumStyle }
    CustomDelegates { id: delegateStyle }
    Plain { id: plainStyle }

    property real spacing: 10
    StyleKit.transitionsEnabled: transitionsEnabled.checked

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentHeight: content.height + 40

        ColumnLayout {
            id: content
            x: 10
            y: app.spacing * 2
            transformOrigin: Item.TopLeft
            spacing: app.spacing * 2

            GroupBox {
                title: "Buttons"
                RowLayout {
                    spacing: app.spacing
                    Button {
                        text: "Normal"
                    }

                    Button {
                        text: "Checkable"
                        checkable: true
                    }

                    Button {
                        text: "Disabled"
                        enabled: false
                    }

                    Button {
                        text: "Flat"
                        flat: true
                        checkable: true
                    }
                }
            }

            GroupBox {
                title: "CheckBoxes and RadioButtons"
                GridLayout {
                    rowSpacing: app.spacing
                    columnSpacing: app.spacing
                    columns: 3

                    CheckBox {
                        text: "Mango"
                        checked: true
                    }

                    CheckBox {
                        text: "Avocado"
                    }

                    CheckBox {
                        text: "Banano"
                        checked: true
                    }

                    RadioButton {
                        text: "Pasta"
                    }

                    RadioButton {
                        text: "Lasagna"
                        checked: true
                    }

                    RadioButton {
                        text: "Burrita"
                    }
                }
            }

            GroupBox {
                title: "Text inputs"
                RowLayout {
                    spacing: app.spacing

                    TextField {
                        id: tf1
                        placeholderText: "Potato"
                    }

                    TextField {
                        id: tf2
                        placeholderText: "Tomato"
                    }
                }
            }

            GroupBox {
                title: "Misc"
                GridLayout {
                    rowSpacing: app.spacing
                    columnSpacing: app.spacing
                    columns: 3

                    Switch {
                        checked: true
                        text: "Switch 1"
                    }

                    SpinBox {
                        id: spinBox1
                        value: 42
                    }

                    ComboBox {
                        id: comboBox1
                        model: ["One", "February", "Aramis", "Winter", "Friday"]
                    }
                }
            }

            GroupBox {
                title: "Sliders"
                RowLayout {
                    spacing: app.spacing

                    ColumnLayout {
                        Slider {
                            id: slider1
                            from: 0
                            to: 10
                            value: 5
                        }

                        RangeSlider {
                            id: rangeSlider1
                            from: 0
                            to: 10
                            first.value: 2
                            second.value: 8
                        }
                    }

                    Slider {
                        id: slider2
                        from: 0
                        to: 10
                        value: 2
                        orientation: Qt.Vertical
                    }

                    RangeSlider {
                        id: rangeSlider2
                        from: 0
                        to: 10
                        first.value: 2
                        second.value: 8
                        orientation: Qt.Vertical
                    }
                }
            }

            GroupBox {
                title: "Popups"
                RowLayout {
                    spacing: app.spacing

                    Button {
                        text: "Open Popup"
                        onClicked: popup.open()
                    }
                }
            }

            GroupBox {
                title: "Variations"
                StyleVariation.variations: ["mini"]
                ColumnLayout {
                    spacing: app.spacing * 2
                    Text {
                        visible: StyleKit.style === hazeStyle
                        text: "These controls are affected by an Instance Variation named 'mini'"
                    }
                    RowLayout {
                        spacing: app.spacing

                        TextField {
                            placeholderText: "Mini zucchini"
                        }

                        Switch {
                            checked: true
                        }

                        Button {
                            // This button will be affected by both an "alert" and a "mini" variation
                            StyleVariation.variations: ["alert"]
                            text: "Alert!"
                        }

                        CheckBox {
                            text: "Baninis"
                            checked: true
                        }

                        Slider {
                            value: 0.5
                        }
                    }
                    Frame {
                        Layout.preferredHeight: 120
                        Layout.fillWidth: true
                        Column {
                            spacing: 20
                            anchors.fill: parent
                            Text {
                                visible: StyleKit.style === hazeStyle
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Frame also has a Type Variation that affects Button"
                            }
                            Button {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Button"
                            }
                        }
                     }
                }
            }

            GroupBox {
                title: "Custom controls"
                RowLayout {
                    spacing: app.spacing

                    CustomButtonImplementation {}
                    CustomButtonImplementation {}
                }
            }
        }

        // Settings menu

        GroupBox {
            id: menu
            anchors.right: parent.right
            anchors.rightMargin: 10
            contentWidth: menuContents.implicitWidth
            contentHeight: menuContents.implicitHeight
            title: "Settings"
            y: app.spacing * 2

            GridLayout {
                id: menuContents
                columns: 2
                rowSpacing: app.spacing
                columnSpacing: app.spacing

                Label { text: "Style" }
                ComboBox {
                    id: styleSelector
                    textRole: "text"
                    valueRole: "value"
                    currentValue: StyleKit.style
                    model: [
                        { value: hazeStyle, text: "Haze" },
                        { value: plainStyle, text: "Plain" },
                        { value: vitrumStyle, text: "Vitrum" },
                        { value: delegateStyle, text: "CustomDelegates" }
                    ]
                    onCurrentTextChanged: {
                        StyleKit.style = model[currentIndex].value;
                        themeSelector.currentValue = StyleKit.style.themeName
                        themeSelector.model = StyleKit.style.themeNames
                    }
                    Component.onCompleted: {
                        themeSelector.currentValue = StyleKit.style.themeName
                        themeSelector.model = StyleKit.style.themeNames
                    }
                }

                Label { text: "Theme" }
                ComboBox {
                    id: themeSelector
                    onCurrentTextChanged: StyleKit.style.themeName = currentText
                }

                Label { text: "Radius" }
                Slider {
                    Layout.maximumWidth: 150
                    from: 0
                    to: 20
                    value: StyleKit.style.control.background.radius
                    onValueChanged: {
                        // Ensure we don't set the value if the style already has the same value
                        // set, or if that value is out-of-range WRT the slider. In both cases,
                        // this would lead to a binding loop.
                        let styleValue = StyleKit.style.control.background.radius
                        if (styleValue === value || styleValue < from || styleValue > to)
                            return
                        StyleKit.style.abstractButton.background.radius = value
                        StyleKit.style.groupBox.background.radius = value
                    }
                }

                Label { text: "Transitions enabled" }
                Switch {
                    id: transitionsEnabled
                    checked: true
                }

                Label { text: "Accent color" }
                ComboBox {
                    id: accentColor
                    model: ["darkseagreen", "plum", "sandybrown", "slateblue"]
                    onCurrentTextChanged: app.palette.accent = currentText
                }
            }
        }
    }

    Popup {
        id: popup
        anchors.centerIn: parent
        closePolicy: Popup.NoAutoClose
        popupType: Popup.Window

        ColumnLayout {
            anchors.centerIn: parent
            spacing: app.spacing * 2

            Label {
                text: qsTr("A Label in a Popup")
                Layout.alignment: Qt.AlignHCenter
            }

            Button {
                text: qsTr("Close Popup")
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: false
                onClicked: popup.close()
            }
        }
    }

    // In addition to Qt Quick Controls, it's also possible to
    // define and style your own custom controls.

    component CustomButtonImplementation : Rectangle {
        implicitWidth: fancyButton.background.implicitWidth + fancyButton.leftPadding + fancyButton.rightPadding
        implicitHeight: fancyButton.background.implicitHeight + fancyButton.topPadding + fancyButton.bottomPadding
        radius: fancyButton.background.radius
        border.color: fancyButton.background.border.color
        border.width: fancyButton.background.border.width
        color: fancyButton.background.color
        scale: fancyButton.background.scale

        StyleReader {
            id: fancyButton
            controlType: hazeStyle.fancyButton
            hovered: hoverHandler.hovered
            pressed: tapHandler.pressed
            palette: app.palette
        }

        Text {
            anchors.centerIn: parent
            font.pixelSize: 15
            text: "Custom Button"
        }

        HoverHandler {
            id: hoverHandler
        }

        TapHandler {
            id: tapHandler
            onTapped: {
                // Change the background color of all controls whose
                // controlType matches fancyButton.type.
                let fancyButtons = StyleKit.style.theme.getControl(fancyButton.type)
                if (fancyButtons) // Only the Haze style defines a fancyButton
                    fancyButtons.background.color = "yellowgreen"
            }
        }
    }
}
