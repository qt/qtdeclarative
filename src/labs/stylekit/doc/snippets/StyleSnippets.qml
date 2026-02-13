// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [dark]
        dark: Theme {
            control.background.color: "#404040"
            button.hovered.background.color: "#6ab0f9"
        }
        //! [dark]

        //! [light]
        light: Theme {
            control.background.color: "#f0f0f0"
            button.hovered.background.color: "#4a90d9"
        }
        //! [light]

        //! [palette]
        button.background.color: palette.accent
        button.text.color: palette.buttonText
        //! [palette]
    }

    /* // we can only have one StyleKit.style set, so comment out this one
    //! [themeName]
    // Main.qml

    StyleKit.style: YourStyleKitStyle {
        // Set a theme different from "System" at start-up
        themeName: "Dark"
    }

    ComboBox {
        // Let the user select a different theme at run-time
        model: StyleKit.style.themeNames
        onCurrentTextChanged: StyleKit.style.themeName = currentText
    }
    //! [themeName]
    */
}
