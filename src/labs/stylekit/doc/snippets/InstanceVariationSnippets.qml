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
    //! [instance variations in style]
    Style {
        StyleVariation {
            name: "mini"
            control {
                padding: 2
                background.implicitHeight: 15
                indicator.implicitWidth: 15
                indicator.implicitHeight: 15
                handle.implicitWidth: 15
                handle.implicitHeight: 15
            }
        }

        StyleVariation {
            name: "alert"
            abstractButton.background.color: "red"
        }
    }
    //! [instance variations in style]

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

            //! [apply instance variation]
            GroupBox {
                title: "Mini controls"
                StyleVariation.variations: ["mini"]

                Row {
                    spacing: 10
                    Button { text: "Save" }
                    CheckBox { text: "Option" }
                    // This button also has the "alert" variation, in addition to "mini"
                    Button {
                        text: "Delete"
                        StyleVariation.variations: ["alert"]
                    }
                }
            }
            //! [apply instance variation]
        }
    }
}
