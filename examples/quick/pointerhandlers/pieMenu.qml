// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Item {
    width: 800
    height: 480

    Rectangle {
        id: rect
        anchors.fill: parent; anchors.margins: 40
        color: pieMenu.active ? "lightgrey" : "darkgrey"

        QuadPieMenu {
            id: pieMenu
            labels: [ "whiz", "bang", "fizz", "buzz" ]
            onTriggered: (text)=> feedback.text = "selected **" + text + "**"
            onCanceled: feedback.text = "canceled"
        }

        Text {
            id: feedback
            x: 6; y: 6
            textFormat: Text.MarkdownText
            text: "hold for context menu"
        }
    }
}
