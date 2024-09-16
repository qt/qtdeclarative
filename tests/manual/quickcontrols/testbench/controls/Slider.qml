// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        ["vertical"],
        ["vertical", "disabled"],
        ["vertical", "pressed"],
        ["horizontal"],
        ["horizontal", "disabled"],
        ["horizontal", "pressed"]
    ]

    property Component component: Slider {
        enabled: !is("disabled")
        orientation: is("horizontal") ? Qt.Horizontal : Qt.Vertical
        value: 0.5
    }
}
