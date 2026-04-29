// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

//! [trace]
ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.debug.control: someButton
    StyleKit.debug.filter: "background.color"

    StyleKit.style: Style {
        button {
            background.color: "gray"
            hovered.background.color: "dimgray"
        }
        dark: Theme {
            button {
                background.color: "skyblue"
                hovered.background.color: "lightblue"
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Button {
            id: someButton
            text: "A Button"
        }
    }
}
//! [trace]
