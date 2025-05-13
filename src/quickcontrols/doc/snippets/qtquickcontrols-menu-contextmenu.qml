// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [root]
MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.RightButton
    onClicked: (mouse) => {
        if (mouse.button === Qt.RightButton)
            contextMenu.popup()
    }
    onPressAndHold: (mouse) => {
        if (mouse.source === Qt.MouseEventNotSynthesized)
            contextMenu.popup()
    }

    Menu {
        id: contextMenu
        MenuItem { text: "Cut" }
        MenuItem { text: "Copy" }
        MenuItem { text: "Paste" }
    }
}
//! [root]
