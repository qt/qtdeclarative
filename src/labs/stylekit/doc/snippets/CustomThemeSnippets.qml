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

    //! [custom themes]
    Style {
        CustomTheme {
            name: "HighContrast"
            theme: Theme {
                control.background.color: "white"
                control.background.border.color: "black"
                control.background.border.width: 2
            }
        }

        CustomTheme {
            name: "Sepia"
            theme: Theme {
                control.text.color: "#5b4636"
                control.background.color: "#f4ecd8"
                control.background.border.color: "#c8b99a"
                applicationWindow.background.color: "#efe6d0"
            }
        }
    }
    //! [custom themes]

    //! [change theme]
    ComboBox {
        model: StyleKit.style.themeNames
        onCurrentTextChanged: StyleKit.style.themeName = currentText
    }
    //! [change theme]

    /*
    //! [custom theme at start-up]
    ApplicationWindow {
        width: 1024
        height: 800
        visible: true

        StyleKit.style: MyStyleKitStyle {
            themeName: "HighContrast"
        }
    }
    //! [custom theme at start-up]
    */

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
