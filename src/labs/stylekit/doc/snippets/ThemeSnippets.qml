// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:

    //! [themes]
    Style {
        light: Theme {
            control {
                text.color: "#202020"
                background.color: "#f0f0f0"
                background.border.color: "#d0d0d0"
                background.shadow.color: "#c0c0c0"
                hovered.background.color: "#e0e0e0"
            }
            applicationWindow.background.color: "gainsboro"
            button.hovered.background.color: "#4a90d9"
            radioButton.indicator.foreground.color: "#d0d0d0"
            switchControl.indicator.foreground.color: "lightslategray"
            switchControl.handle.color: "white"
        }

        dark: Theme {
            control {
                text.color: "#e0e0e0"
                background.color: "#404040"
                background.border.color: "#606060"
                background.shadow.color: "#222222"
                hovered.background.color: "#505050"
            }
            applicationWindow.background.color: "#2b2b2b"
            button.hovered.background.color: "#6ab0f9"
            radioButton.indicator.foreground.color: "#606060"
            switchControl.indicator.foreground.color: "#505050"
            switchControl.handle.color: "#808080"
        }

        CustomTheme {
            name: "HighContrast"
            theme: Theme {
                control {
                    text.color: "black"
                    text.bold: true
                    background.color: "white"
                    background.border.color: "black"
                    background.border.width: 3
                    background.shadow.visible: false
                    hovered.background.border.width: 5
                }
                applicationWindow.background.color: "white"
                itemDelegate.hovered.text.color: "white"
                itemDelegate.hovered.background.color: "black"
                itemDelegate.background.border.width: 0
                button.hovered.background.color: "black"
                button.hovered.text.color: "white"
                radioButton.indicator.foreground.color: "white"
                radioButton.checked.indicator.foreground.color: "black"
                switchControl.indicator.foreground.color: "white"
                switchControl.handle.color: "white"
                switchControl.handle.border.color: "black"
                switchControl.handle.border.width: 2
                switchControl.checked.handle.color: "black"
            }
        }
    }
    //! [themes]

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    GroupBox {
        title: "GroupBox"
        Column {
            spacing: 10

            ComboBox {
                model: StyleKit.style.themeNames
                onCurrentTextChanged: StyleKit.style.themeName = currentText
            }
            Button {
                text: "button"
            }
            Slider {
                width: 200
            }
            RadioButton {
                text: "RadioButton"
            }
            Switch {
                text: "Switch"
            }
        }
    }
}
