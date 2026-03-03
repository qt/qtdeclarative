// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:
    //! [propagation]
    Style {
        button.background.radius: 2
        StyleVariation {
            name: "alert"
            button.background.border.width: 4
            button.background.radius: 0
        }

        light: Theme {
            StyleVariation {
                name: "alert"
                button.background.color: "red"
            }
        }

        dark: Theme {
            button.background.radius: 6
            StyleVariation {
                name: "alert"
                button.background.color: "cyan"
            }
        }
    }
    //! [propagation]

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                StyleVariation.variations: ["alert"]
                text: "Button outside"
            }
        }
    }
}
