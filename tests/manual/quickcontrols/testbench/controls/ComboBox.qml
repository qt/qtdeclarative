// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["open"],
        ["editable"],
        ["editable", "disabled"]
    ]

    property Component component: ComboBox {
        enabled: !is("disabled")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        editable: is("editable")
        model: ["ComboBox", "Apple", "Bird", "Cat", "Dog", "Elephant"]
    }
}
