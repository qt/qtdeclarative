// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick.Controls.Basic

ApplicationWindow {
    width: 500
    height: 500
    visible: true
    font.pixelSize: 20
    palette.windowText: "steelblue"

    // This will have a pixelSize of 20 and be "steelblue" in color.
    header: Label {
        text: "ApplicationWindow Label"
        leftPadding: 20
        topPadding: 20
    }

    Pane {
        width: 400
        height: 400
        anchors.centerIn: parent
        palette.window: "#edf3f8"
        palette.windowText: "tomato"

        // This will have a pixelSize of 20 and be "tomato" in color.
        Label {
            text: "Pane Label"
        }

        Popup {
            width: 300
            height: 300
            anchors.centerIn: parent
            font.pixelSize: 10
            visible: true

            // This will have a pixelSize of 10 and "steelblue" in color.
            Label {
                text: "Popup Label"
            }

            Popup {
                width: 200
                height: 200
                anchors.centerIn: parent
                visible: true

                // This will have a pixelSize of 20 and be "steelblue" in color.
                Label {
                    text: "Child Popup Label"
                }
            }
        }
    }
}
//! [file]
