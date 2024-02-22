// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["pressed"],
        ["disabled"],
        ["mirrored"],
        ["mirrored", "pressed"],
        ["mirrored", "disabled"],
        ["editable"],
        ["editable", "pressed"],
        ["editable", "disabled"],
        ["editable", "mirrored"],
        ["editable", "mirrored", "pressed"],
        ["editable", "mirrored", "disabled"]
    ]

    property Component component: SpinBox {
        value: 1
        enabled: !is("disabled")
        editable: is("editable")
        up.pressed: is("pressed")

        LayoutMirroring.enabled: is("mirrored")
    }
}
