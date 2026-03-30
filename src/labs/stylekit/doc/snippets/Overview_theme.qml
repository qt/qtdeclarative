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
            applicationWindow.background.color: "gainsboro"
            control.text.color: "#202020"
            control.background.color: "#f0f0f0"
            control.background.border.color: "#d0d0d0"
            button.hovered.background.color: "#4a90d9"
            radioButton.indicator.foreground.color: "#d0d0d0"
        }

        dark: Theme {
            applicationWindow.background.color: "#2b2b2b"
            control.text.color: "#e0e0e0"
            control.background.color: "#404040"
            control.background.border.color: "#606060"
            button.hovered.background.color: "#6ab0f9"
            radioButton.indicator.foreground.color: "#606060"
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
        }
    }
}
