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
        ["checked", "disabled", "mirrored"],
        ["checked", "pressed"],
        ["checked", "pressed", "mirrored"],
        ["mirrored"],
    ]

    property Component component: Switch {
        text: "Switch"
        enabled: !is("disabled")
        checked: is("checked")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined

        LayoutMirroring.enabled: is("mirrored")
    }
}
