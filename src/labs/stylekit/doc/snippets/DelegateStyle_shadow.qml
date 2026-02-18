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

        //! [drop shadow]
        button {
            background {
                shadow {
                    color: "lightgray"
                    opacity: 0.6
                    verticalOffset: 2
                    horizontalOffset: 2
                }
            }
        }
        //! [drop shadow]

        //! [glow]
        dark: Theme {
            applicationWindow.background.color: "#1e1e1e"
            switchControl {
                handle.color: "white"
                handle.shadow {
                    color: "ghostwhite"
                    scale: 1.2
                    opacity: 0.6
                }
                checked.handle.shadow.scale: 1.5
            }
        }
        //! [glow]

        themeName: "dark"
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Button {
                text: "Button"
            }

            Switch {
                text: "Switch"
            }
        }
    }
}
