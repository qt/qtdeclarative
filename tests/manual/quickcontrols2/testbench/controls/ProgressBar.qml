// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
