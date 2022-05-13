// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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

    property Component component: RangeSlider {
        enabled: !is("disabled")
        orientation: is("horizontal") ? Qt.Horizontal : Qt.Vertical
        second.value: 0.5
    }
}
