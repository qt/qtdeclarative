// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        // TODO: no down property to test this with
//        ["pressed"]
    ]

    property Component component: Dial {
        enabled: !is("disabled")
    }
}
