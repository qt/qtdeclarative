// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
// Main.qml

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    // Assign the style to be used
    StyleKit.style: PlainStyle {}

    // Controls are used as normal
    Column {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Button {
            text: "Button"
        }

        Slider {
            width: 200
        }
    }
}
//! [1]
