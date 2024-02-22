// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

    property Component component: CheckBox {
        text: "CheckBox"
        enabled: !is("disabled")
        checkState: is("checked") ? Qt.Checked : is("partially-checked") ? Qt.PartiallyChecked : Qt.Unchecked
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
    }
}
