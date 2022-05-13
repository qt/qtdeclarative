// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["checked"],
        ["checked", "disabled"],
        ["checked", "pressed"],
        ["partially-checked"],
        ["partially-checked", "disabled"],
        ["partially-checked", "pressed"],
    ]

    property Component component: CheckDelegate {
        text: "CheckDelegate"
        enabled: !is("disabled")
        checkState: is("checked") ? Qt.Checked : is("partially-checked") ? Qt.PartiallyChecked : Qt.Unchecked
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        focusPolicy: Qt.StrongFocus
    }

    property Component exampleComponent: ListView {
        implicitWidth: 200
        implicitHeight: 200
        clip: true
        model: 20
        delegate: CheckDelegate {
            width: parent.width
            text: "CheckDelegate"
            focusPolicy: Qt.StrongFocus
        }
    }
}
