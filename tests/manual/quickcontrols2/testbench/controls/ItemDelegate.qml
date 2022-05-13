// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["highlighted"],
        ["highlighted", "pressed"]
    ]

    property Component component: ItemDelegate {
        text: "ItemDelegate"
        enabled: !is("disabled")
        checkable: is("checkable")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        highlighted: is("highlighted")
        focusPolicy: Qt.StrongFocus
    }

    property Component exampleComponent: ListView {
        implicitWidth: 200
        implicitHeight: 200
        clip: true
        model: 20
        delegate: ItemDelegate {
            width: parent.width
            text: "ItemDelegate"
            focusPolicy: Qt.StrongFocus
        }
    }
}
