// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["indeterminate"]
    ]

    property Component component: ProgressBar {
        enabled: !is("disabled")
        indeterminate: is("indeterminate")
        value: 0.25
    }
}
