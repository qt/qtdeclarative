// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["disabled", "checked"],
        ["pressed"],
        ["checked"],
    ]

    property Component component: Component {
        DelayButton {
            text: "DelayButton"
            enabled: !is("disabled")
            // Only set it if it's pressed, or the non-pressed examples will have no press effects
            down: is("pressed") ? true : undefined
        }
    }
}
